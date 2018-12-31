#include <pybind11/pybind11.h>

#include <ki/serialization/JsonSerializer.h>
#include <ki/pclass/TypeSystem.h>
#include <ki/pclass/types/Type.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace ki
{
namespace serialization
{

PYBIND11_MODULE(serialization, m)
{
    // Classes
    // py::class_<BinarySerializer> binary_serializer_cls(m, "BinarySerializer");
    py::class_<JsonSerializer> json_serializer_cls(m, "JsonSerializer");

    /*
    py::enum_<SerializerBinary::flags>(m, "SerializerBinaryFlags")
        .value("NONE", SerializerBinary::flags::NONE)
        .value("WRITE_SERIALIZER_FLAGS", SerializerBinary::flags::WRITE_SERIALIZER_FLAGS)
        .value("COMPRESSED", SerializerBinary::flags::COMPRESSED);
    */

    // BinarySerializer Definitions
    /*
    binary_serializer_cls.def(py::init<const pclass::TypeSystem &, const bool, const SerializerBinary::flags>(),
        "type_system"_a, "is_file"_a, "flags"_a);
    binary_serializer_cls.def("save", &SerializerBinary::save, "object"_a, "stream"_a);
    binary_serializer_cls.def("load", [](SerializerBinary &self, BitStream &stream, std::size_t size = 0)
    {
        // Use the size of the buffer as default.
        if (size == 0)
            size = stream.buffer().size();

        // Load the PropertyClass into our destination pointer.
        pclass::PropertyClass *dest = nullptr;
        self.load(dest, stream, size);

        // Cast it to its Python representation.
        py::object instance = py::cast(dest);

        // This object was just instantiated, and as such now has a
        // reference count >= 2; decrement the reference count to avoid
        // memory leaks.
        instance.dec_ref();

        return instance;
    }, py::return_value_policy::take_ownership, "stream"_a, "size"_a = 0);
    */

    // JsonSerializer Definitions
    json_serializer_cls.def(py::init<pclass::TypeSystem &, bool>(),
        "type_system"_a, "is_file"_a);
    json_serializer_cls.def("save", &JsonSerializer::save, "object"_a);
}

}
}
