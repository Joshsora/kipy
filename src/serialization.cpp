#include <pybind11/pybind11.h>

#include <ki/serialization/BinarySerializer.h>
#include <ki/serialization/JsonSerializer.h>
#include <ki/pclass/Type.h>
#include <ki/pclass/TypeSystem.h>
#include <ki/pclass/PropertyClass.h>

namespace py = pybind11;

using namespace pybind11::literals;

using ki::BitStream;

using ki::pclass::TypeSystem;
using ki::pclass::PropertyClass;

using ki::serialization::BinarySerializer;
using ki::serialization::JsonSerializer;

namespace
{
    /**
    * A Python alias class for BinarySerializer.
    */
    class PyBinarySerializer : public BinarySerializer
    {
    public:
        using BinarySerializer::BinarySerializer;

        bool presave_object(const PropertyClass *object, BitStream &stream) const override
        {
            PYBIND11_OVERLOAD(bool, BinarySerializer, presave_object, object, stream);
        }
    };

    /**
    * Allows for public access to the BinarySerializer::presave_object method.
    */
    class BinarySerializerPublicist : public BinarySerializer
    {
    public:
        using BinarySerializer::presave_object;
    };
}

void bind_serialization(py::module &m)
{
    // Classes
    py::class_<BinarySerializer, PyBinarySerializer> binary_serializer_cls(m, "BinarySerializer");
    py::class_<JsonSerializer> json_serializer_cls(m, "JsonSerializer");

    py::enum_<BinarySerializer::flags>(m, "BinarySerializerFlags", py::arithmetic())
        .value("NONE", BinarySerializer::flags::NONE)
        .value("WRITE_SERIALIZER_FLAGS", BinarySerializer::flags::WRITE_SERIALIZER_FLAGS)
        .value("COMPRESSED", BinarySerializer::flags::COMPRESSED);

    // BinarySerializer Definitions
    binary_serializer_cls.def(py::init<const TypeSystem &, bool, BinarySerializer::flags>(),
        "type_system"_a, "is_file"_a, "flags"_a);
    binary_serializer_cls.def("presave_object", &BinarySerializerPublicist::presave_object);
    binary_serializer_cls.def("save", &BinarySerializer::save, "object"_a, "stream"_a);
    binary_serializer_cls.def("load", [](BinarySerializer &self, BitStream &stream, std::size_t size = 0)
    {
        // Use the size of the buffer as default.
        if (size == 0)
            size = stream.buffer().size();

        // Load the PropertyClass into our destination pointer.
        std::unique_ptr<PropertyClass> dest = nullptr;
        self.load(dest, stream, size);

        // Cast it to its Python representation.
        py::object instance = py::cast(dest.release());

        // This object was just instantiated, and as such now has a
        // reference count >= 2; decrement the reference count to avoid
        // memory leaks.
        instance.dec_ref();

        return instance;
    }, py::return_value_policy::take_ownership, "stream"_a, "size"_a = 0);

    // JsonSerializer Definitions
    json_serializer_cls.def(py::init<TypeSystem &, bool>(),
        "type_system"_a, "is_file"_a);
    json_serializer_cls.def("save", &JsonSerializer::save, "object"_a);
    json_serializer_cls.def("load", [](JsonSerializer &self, const std::string &json_string)
    {
        // Load the PropertyClass into our destination pointer.
        std::unique_ptr<PropertyClass> dest = nullptr;
        self.load(dest, json_string);

        // Cast it to its Python representation.
        py::object instance = py::cast(dest.release());

        // This object was just instantiated, and as such now has a
        // reference count >= 2; decrement the reference count to avoid
        // memory leaks.
        instance.dec_ref();

        return instance;
    }, py::return_value_policy::take_ownership, "json_string"_a);
}
