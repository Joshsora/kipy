#include <pybind11/pybind11.h>

#include <ki/serialization/SerializerBinary.h>
#include <ki/pclass/TypeSystem.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace ki
{
namespace serialization
{

PYBIND11_MODULE(serialization, m)
{
    // Classes
    py::class_<SerializerBinary> serializer_binary_cls(m, "SerializerBinary");

    py::enum_<SerializerBinary::flags>(m, "SerializerBinaryFlags")
        .value("NONE", SerializerBinary::flags::NONE)
        .value("WRITE_SERIALIZER_FLAGS", SerializerBinary::flags::WRITE_SERIALIZER_FLAGS)
        .value("COMPRESSED", SerializerBinary::flags::COMPRESSED);

    // SerializerBinary Definitions
    serializer_binary_cls.def(py::init<const pclass::TypeSystem *, bool, SerializerBinary::flags>(),
        "type_system"_a, "is_file"_a, "flags"_a);
    serializer_binary_cls.def("save", &SerializerBinary::save, "object"_a, "stream"_a);
}

}
}
