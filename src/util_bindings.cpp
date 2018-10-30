#include <string>
#include <iostream>
#include <sstream>
#include <cmath>

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

#include <ki/util/BitTypes.h>
#include <ki/util/BitStream.h>

namespace py = pybind11;

template <uint8_t N, bool Unsigned>
void def_bit_integer_class(py::module &m)
{
    using Class = ki::BitInteger<N, Unsigned>;

    // Make the class name
    std::string pyclass_name;
    if (Unsigned)
        pyclass_name = "bui" + std::to_string(N);
    else
        pyclass_name = "bi" + std::to_string(N);

    // The type used to internally store the N-bit integer.
    using type = typename std::conditional<
        Unsigned,
        typename ki::bits<N>::uint_type,
        typename ki::bits<N>::int_type
    >::type;

    // Class: BitInteger<N, Unsigned>
    py::class_<Class>(m, pyclass_name.c_str())
        // Constructors
        .def(py::init<>())
        .def(py::init<const ki::BitInteger<N, Unsigned> &>(), py::arg("cp"))
        .def(py::init<const type>(), py::arg("value"))

        // Overloaded operators
        .def(py::self += type())
        .def(py::self -= type())
        .def(py::self *= type())
        .def(py::self /= type())
        .def(py::self |= type())
        .def(py::self &= type())

        // Descriptor: __repr__
        .def("__repr__",
            [](py::object self)
            {
                std::ostringstream oss;
                oss << self.attr("__class__").attr("__name__").cast<std::string>();
                oss << "(" << std::to_string(self.attr("__int__")().cast<type>()) << ")";
                return oss.str();
            })

        // Descriptor: __int__
        // This will allow int() casts in Python.
        .def("__int__",
            [](ki::BitInteger<N, Unsigned> &self)
            {
                return (type)self;
            })
        ;
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

template <
    typename IntegerT,
    typename = std::enable_if<ki::is_integral<IntegerT>::value>
>
void def_bit_stream_read_method(py::class_<ki::BitStream> &c, const char *name)
{
    c.def(name,
        &ki::BitStream::read<IntegerT>,
        py::arg("bits") = ki::bitsizeof<IntegerT>::value);
}

template <
    typename IntegerT,
    typename = std::enable_if<ki::is_integral<IntegerT>::value>
>
void def_bit_stream_write_method(py::class_<ki::BitStream> &c, const char *name)
{
    c.def(name,
        &ki::BitStream::write<IntegerT>,
        py::arg("value"),
        py::arg("bits") = ki::bitsizeof<IntegerT>::value);
}

template <uint8_t N>
void def_bit_integer_read_methods(py::class_<ki::BitStream> &c)
{
    // Method: read_bi*
    def_bit_stream_read_method<ki::bi<N>>(c, ("read_bi" + std::to_string(N)).c_str());
    // Method: read_bui*
    def_bit_stream_read_method<ki::bui<N>>(c, ("read_bui" + std::to_string(N)).c_str());

    def_bit_integer_read_methods<N - 1>(c);
}

template <>
void def_bit_integer_read_methods<0>(py::class_<ki::BitStream> &c) {}

template <uint8_t N>
void def_bit_integer_write_methods(py::class_<ki::BitStream> &c)
{
    // Method: write_bi*
    def_bit_stream_write_method<ki::bi<N>>(c, ("write_bi" + std::to_string(N)).c_str());
    // Method: write_bui*
    def_bit_stream_write_method<ki::bui<N>>(c, ("write_bui" + std::to_string(N)).c_str());

    def_bit_integer_write_methods<N - 1>(c);
}

template <>
void def_bit_integer_write_methods<0>(py::class_<ki::BitStream> &c) {}

PYBIND11_MODULE(util, m)
{
    // Submodule: bit_types

    py::module m_bit_types = m.def_submodule("bit_types");

    // Classes: bi*/bui*
    def_bit_integer_classes<64>(m_bit_types);

    // Submodule: bit_types (end)

    using namespace ki;

    // Class: BitStream
    auto c = py::class_<BitStream>(m, "BitStream")
        // Constructors
        .def(py::init<const size_t>(), py::arg("buffer_size") = KI_BITSTREAM_DEFAULT_BUFFER_SIZE)

        // Property: capacity (read-only)
        .def_property_readonly("capacity", &BitStream::capacity)
        // Property: data (read-only)
        .def_property_readonly("data", &BitStream::data, py::return_value_policy::copy)

        // Method: tell()
        .def("tell", &BitStream::tell)
        // Method: seek()
        .def("seek", &BitStream::seek, py::arg("position"))
        ;

    // Methods: read_*
    def_bit_stream_read_method<bool>(c, "read_bool");
    def_bit_stream_read_method<int8_t>(c, "read_int8");
    def_bit_stream_read_method<int16_t>(c, "read_int16");
    def_bit_stream_read_method<int32_t>(c, "read_int32");
    def_bit_stream_read_method<int64_t>(c, "read_int64");
    def_bit_stream_read_method<uint8_t>(c, "read_uint8");
    def_bit_stream_read_method<uint16_t>(c, "read_uint16");
    def_bit_stream_read_method<uint32_t>(c, "read_uint32");
    def_bit_stream_read_method<uint64_t>(c, "read_uint64");

    // Methods: write_*
    def_bit_stream_write_method<bool>(c, "write_bool");
    def_bit_stream_write_method<int8_t>(c, "write_int8");
    def_bit_stream_write_method<int16_t>(c, "write_int16");
    def_bit_stream_write_method<int32_t>(c, "write_int32");
    def_bit_stream_write_method<int64_t>(c, "write_int64");
    def_bit_stream_write_method<uint8_t>(c, "write_uint8");
    def_bit_stream_write_method<uint16_t>(c, "write_uint16");
    def_bit_stream_write_method<uint32_t>(c, "write_uint32");
    def_bit_stream_write_method<uint64_t>(c, "write_uint64");

    // Methods: read_bi*/read_bui*
    def_bit_integer_read_methods<64>(c);
    // Methods: write_bi*/write_bui*
    def_bit_integer_write_methods<64>(c);

    // Class: BitStreamPos
    using stream_pos = BitStream::stream_pos;

    py::class_<stream_pos>(m, "BitStreamPos")
        // Constructors
        .def(py::init<const intmax_t, const int>(), py::arg("byte") = 0, py::arg("bit") = 0)
        .def(py::init<const BitStream::stream_pos &>(), py::arg("cp"))

        // Overloaded operators
        .def(py::self + py::self)
        .def(py::self + int())
        .def(py::self - py::self)
        .def(py::self - int())
        .def(py::self += py::self)
        .def(py::self += int())
        .def(py::self -= py::self)
        .def(py::self -= int())

        // Descriptor: __repr__
        .def("__repr__",
            [](stream_pos &self)
            {
                std::ostringstream oss;
                oss << "BitStreamPos(" << self.get_byte() << ", " << std::to_string(self.get_bit()) << ")";
                return oss.str();
            })

        // Property: byte (read-only)
        .def_property_readonly("byte", &stream_pos::get_byte)
        // Property: bit (read-only)
        .def_property_readonly("bit", &stream_pos::get_bit)
        ;
}
