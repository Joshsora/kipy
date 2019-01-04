#include <string>
#include <iostream>
#include <sstream>
#include <cmath>
#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

#include <ki/util/BitTypes.h>
#include <ki/util/BitStream.h>

namespace py = pybind11;

using namespace pybind11::literals;

using ki::IBitBuffer;
using ki::BitBuffer;
using ki::BitBufferSegment;
using ki::BitStream;

namespace
{
template <uint8_t N, bool Unsigned>
void def_bit_integer_class(py::module &m)
{
    using Class = ki::BitInteger<N, Unsigned>;

    std::string name;
    if (Unsigned)
        name = "bui" + std::to_string(N);
    else
        name = "bi" + std::to_string(N);

    // The type used to internally store the N-bit integer.
    using type = typename std::conditional<
        Unsigned,
        typename ki::bits<N>::uint_type,
        typename ki::bits<N>::int_type
    >::type;

    py::class_<Class>(m, name.c_str())
        .def(py::init<>())
        .def(py::init<const ki::BitInteger<N, Unsigned> &>(), "cp"_a)
        .def(py::init<const type>(), "value"_a)

        .def(py::self += type())
        .def(py::self -= type())
        .def(py::self *= type())
        .def(py::self /= type())
        .def(py::self |= type())
        .def(py::self &= type())

        .def("__repr__", [](const py::object &self)
        {
            std::ostringstream oss;
            oss << self.attr("__class__").attr("__name__").cast<std::string>();
            oss << "(" << std::to_string(self.attr("__int__")().cast<type>()) << ")";
            return oss.str();
        })
        .def("__int__", [](ki::BitInteger<N, Unsigned> &self)
        {
            return static_cast<type>(self);
        });
}

template <uint8_t N>
void def_bit_integer_classes(py::module &m)
{
    def_bit_integer_class<N, false>(m);
    def_bit_integer_class<N, true>(m);
    def_bit_integer_classes<N - 1>(m);
}

template <>
void def_bit_integer_classes<0>(py::module &m) {}


class PyIBitBuffer : public IBitBuffer
{
public:
    using IBitBuffer::IBitBuffer;

    uint8_t *data() const override
    {
        PYBIND11_OVERLOAD_PURE(uint8_t *, IBitBuffer, data, );
    }

    std::size_t size() const override
    {
        PYBIND11_OVERLOAD_PURE(std::size_t, IBitBuffer, size, );
    }

    void resize(std::size_t new_size) override
    {
        PYBIND11_OVERLOAD_PURE(void, IBitBuffer, resize, new_size);
    }

    uint64_t read(IBitBuffer::buffer_pos position, uint8_t bits) const override
    {
        PYBIND11_OVERLOAD_PURE(uint64_t, IBitBuffer, read, position, bits);
    }

    void write(uint64_t value, IBitBuffer::buffer_pos position, uint8_t bits) override
    {
        PYBIND11_OVERLOAD_PURE(void, IBitBuffer, write, value, position, bits);
    }
};
}

void bind_util(py::module &m)
{
    // Submodules
    py::module bit_types_submodule = m.def_submodule("bit_types");

    // Classes
    py::class_<IBitBuffer::buffer_pos> buffer_pos_cls(m, "BufferPos");
    py::class_<IBitBuffer, PyIBitBuffer> i_bit_buffer_cls(m, "IBitBuffer");
    py::class_<BitBuffer, IBitBuffer> bit_buffer_cls(m, "BitBuffer");
    py::class_<BitBufferSegment, IBitBuffer> bit_buffer_segment_cls(m, "BitBufferSegment");
    py::class_<BitStream> bit_stream_cls(m, "BitStream");

    // BitInteger Definitions
    def_bit_integer_classes<64>(bit_types_submodule);

    // BufferPos Definitions
    buffer_pos_cls.def(py::init<uint32_t, int>(), "byte"_a = 0, "bit"_a = 0);
    buffer_pos_cls.def("__repr__", [](IBitBuffer::buffer_pos &self)
    {
        std::ostringstream oss;
        oss << "BufferPos(" << self.get_byte() << ", " << std::to_string(self.get_bit()) << ")";
        return oss.str();
    });
    buffer_pos_cls.def("as_bytes", &IBitBuffer::buffer_pos::as_bytes);
    buffer_pos_cls.def("as_bits", &IBitBuffer::buffer_pos::as_bits);
    buffer_pos_cls.def_property_readonly("byte", &IBitBuffer::buffer_pos::get_byte);
    buffer_pos_cls.def_property_readonly("bit", &IBitBuffer::buffer_pos::get_bit);
    buffer_pos_cls.def(py::self + py::self);
    buffer_pos_cls.def(py::self - py::self);
    buffer_pos_cls.def(py::self + int());
    buffer_pos_cls.def(py::self - int());
    buffer_pos_cls.def(py::self += py::self);
    buffer_pos_cls.def(py::self -= py::self);
    buffer_pos_cls.def(py::self += int());
    buffer_pos_cls.def(py::self -= int());

    // IBitBuffer Definitions
    i_bit_buffer_cls.def(py::init<>());
    i_bit_buffer_cls.def_property("data", [](const IBitBuffer &self)
    {
        return py::bytes(std::string(reinterpret_cast<char *>(self.data()), self.size()));
    }, [](IBitBuffer &self, char *buffer)
    {
        uint8_t *c_buffer = reinterpret_cast<uint8_t *>(buffer);
        std::memcpy(self.data(), c_buffer, self.size());
    });
    i_bit_buffer_cls.def_property_readonly("size", &IBitBuffer::size);
    i_bit_buffer_cls.def("resize", &IBitBuffer::resize, "new_size"_a);
    i_bit_buffer_cls.def("segment", &IBitBuffer::segment,
        py::return_value_policy::take_ownership, "from"_a, "bitsize"_a);
    i_bit_buffer_cls.def("read_signed", [](const IBitBuffer &self, IBitBuffer::buffer_pos position, const uint8_t bits = ki::bitsizeof<int64_t>::value)
    {
        int64_t value = self.read<int64_t>(position, bits);
        if (bits < 64)
        {
            int64_t positive = value & ((1 << (bits-1)) - 1);
            int64_t signed_place_value = 1 << (bits-1);
            value = positive - (value & signed_place_value);
        }
        return value;
    }, "position"_a, "bits"_a);
    i_bit_buffer_cls.def("read_unsigned", &IBitBuffer::read<uint64_t>,
        "position"_a, "bits"_a = ki::bitsizeof<uint64_t>::value);
    i_bit_buffer_cls.def("write_signed", &IBitBuffer::write<int64_t>,
        "value"_a, "position"_a, "bits"_a = ki::bitsizeof<int64_t>::value);
    i_bit_buffer_cls.def("write_unsigned", &IBitBuffer::write<uint64_t>,
        "value"_a, "position"_a, "bits"_a = ki::bitsizeof<uint64_t>::value);

    // BitBuffer Definitions
    bit_buffer_cls.def(py::init<std::size_t>(), "buffer_size"_a = KI_BITBUFFER_DEFAULT_SIZE);
    bit_buffer_cls.def(py::init<const BitBuffer &>(), "that"_a);
    bit_buffer_cls.def(py::init([](char *buffer, const std::size_t buffer_size)
    {
        auto *bit_buffer = new BitBuffer(buffer_size);
        uint8_t *c_buffer = reinterpret_cast<uint8_t *>(buffer);
        std::memcpy(bit_buffer->data(), c_buffer, bit_buffer->size());
        return std::unique_ptr<BitBuffer>(bit_buffer);
    }), "buffer"_a, "buffer_size"_a);

    // BitBufferSegment Definitions
    bit_buffer_segment_cls.def(py::init<IBitBuffer &, IBitBuffer::buffer_pos, std::size_t>(),
        "buffer"_a, "from"_a, "bitsize"_a);

    // BitStream Definitions
    bit_stream_cls.def(py::init<IBitBuffer &>(), py::keep_alive<1, 2>(), "buffer"_a);
    bit_stream_cls.def(py::init<const BitStream &>(), "that"_a);
    bit_stream_cls.def("tell", &BitStream::tell);
    bit_stream_cls.def("seek", &BitStream::seek, "position"_a, "expand"_a);
    bit_stream_cls.def_property_readonly("capacity", &BitStream::capacity);
    bit_stream_cls.def_property_readonly("buffer", &BitStream::buffer);
    bit_stream_cls.def("read_signed", [](BitStream &self, const uint8_t bits = ki::bitsizeof<int64_t>::value)
    {
        int64_t value = self.read<int64_t>(bits);
        if (bits < 64)
        {
            int64_t positive = value & ((1 << (bits-1)) - 1);
            int64_t signed_place_value = 1 << (bits-1);
            value = positive - (value & signed_place_value);
        }
        return value;
    }, "bits"_a);
    bit_stream_cls.def("read_unsigned", &BitStream::read<uint64_t>,
        "bits"_a = ki::bitsizeof<uint64_t>::value);
    bit_stream_cls.def("write_signed", &BitStream::write<int64_t>,
        "value"_a, "bits"_a = ki::bitsizeof<int64_t>::value);
    bit_stream_cls.def("write_unsigned", &BitStream::write<uint64_t>,
        "value"_a, "bits"_a = ki::bitsizeof<uint64_t>::value);
}
