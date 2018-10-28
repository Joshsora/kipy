#include <string>
#include <iostream>
#include <sstream>

#include <pybind11/pybind11.h>

#include <ki/util/BitTypes.h>
#include <ki/util/BitStream.h>

namespace py = pybind11;

template <uint8_t N, bool Unsigned>
void def_bit_integer(py::module &m)
{
    using Class = ki::BitInteger<N, Unsigned>;

    // Make the class name
    std::ostringstream oss;
    if (Unsigned)
        oss << "bui" << (uint16_t)N;
    else
        oss << "bi" << (uint16_t)N;
    std::string pyclass_name = oss.str();

    // The type used to internally store the N-bit integer.
    using type = typename std::conditional<
        Unsigned,
        typename ki::bits<N>::uint_type,
        typename ki::bits<N>::int_type
    >::type;

    // Class: BitInteger<N, Unsigned>
    py::class_<Class>(m, pyclass_name.c_str())
        .def(py::init<>())
        .def(py::init<const ki::BitInteger<N, Unsigned> &>())
        .def(py::init<const type>())

        // Descriptor: __int__
        // This will allow int() casts in Python.
        .def("__int__",
            [](ki::BitInteger<N, Unsigned> &self)
            {
                return (type)self;
            },
            py::return_value_policy::take_ownership)
        
        // Overloaded operator: +=
        .def("__iadd__",
            [](ki::BitInteger<N, Unsigned> &self, const type rhs)
            {
                self += rhs;
                return self;
            },
            py::arg("rhs"),
            py::return_value_policy::reference)
        // Overloaded operator: -=
        .def("__isub__",
            [](ki::BitInteger<N, Unsigned> &self, const type rhs)
            {
                self -= rhs;
                return self;
            },
            py::arg("rhs"),
            py::return_value_policy::reference)
        // Overloaded operator: *=
        .def("__imul__",
            [](ki::BitInteger<N, Unsigned> &self, const type rhs)
            {
                self *= rhs;
                return self;
            },
            py::arg("rhs"),
            py::return_value_policy::reference)
        // Overloaded operator: /=
        .def("__idiv__",
            [](ki::BitInteger<N, Unsigned> &self, const type rhs)
            {
                self /= rhs;
                return self;
            },
            py::arg("rhs"),
            py::return_value_policy::reference)
        // Overloaded operator: |=
        .def("__ior__",
            [](ki::BitInteger<N, Unsigned> &self, const type rhs)
            {
                self |= rhs;
                return self;
            },
            py::arg("rhs"),
            py::return_value_policy::reference)
        // Overloaded operator: &=
        .def("__ixor__",
            [](ki::BitInteger<N, Unsigned> &self, const type rhs)
            {
                self &= rhs;
                return self;
            },
            py::arg("rhs"),
            py::return_value_policy::reference);
}

template <uint8_t N>
void def_n_bit_integers(py::module &m)
{
    def_bit_integer<N, false>(m);
    def_bit_integer<N, true>(m);
    def_n_bit_integers<N - 1>(m);
}

template <>
void def_n_bit_integers<0>(py::module &m) {}

PYBIND11_MODULE(util, m)
{
    // Submodule: bit_types
    py::module m_bit_types = m.def_submodule("bit_types");

    // Classes: bi*/bui*
    def_n_bit_integers<64>(m_bit_types);

    /*
    using namespace ki;

    // Class: BitStream
    py::class_<BitStream>(m, "BitStream")

        // Initializer
        .def(py::init<const size_t>(), py::arg("buffer_size") = KI_BITSTREAM_DEFAULT_BUFFER_SIZE)

        // Property: capacity (read-only)
        .def_property_readonly("capacity", &BitStream::capacity, py::return_value_policy::copy)
        // Property: data (read-only)
        .def_property_readonly("data", &BitStream::data, py::return_value_policy::reference)

        // Method: tell()
        .def("tell", &BitStream::tell, py::return_value_policy::copy)
        // Method: seek()
        .def("seek", &BitStream::seek, py::arg("position"));*/
}
