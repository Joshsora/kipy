#include <pybind11/pybind11.h>

#include <string>
#include <iostream>
#include <sstream>

#include <ki/util/BitTypes.h>
#include <ki/pclass/types/Type.h>
#include <ki/pclass/types/PrimitiveType.h>
#include <ki/pclass/TypeSystem.h>
#include <ki/pclass/HashCalculator.h>
#include <ki/pclass/PropertyClass.h>
#include <ki/pclass/PropertyList.h>
#include <ki/pclass/Property.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace ki
{
namespace pclass
{

namespace
{
template <typename ValueT>
void def_primitive_type_class(py::module &m, const char *name)
{
    using Class = PrimitiveType<ValueT>;
    using BaseClass = Type;

    py::class_<Class, BaseClass>(m, name)
        .def(py::init<const std::string, const TypeSystem &>(), "name"_a, "type_system"_a)
        .def("write_to", [](const Type &self, ki::BitStream &stream, ValueT &value)
        {
            self.write_to(stream, Value(value));
        }, "stream"_a, "value"_a)
        .def("read_from", [](const Class &self, ki::BitStream &stream)
        {
            ValueT return_value;
            Value value(return_value);
            self.read_from(stream, value);
            return return_value;
        }, "stream"_a);
}


class PyHashCalculator : public HashCalculator
{
public:
    using HashCalculator::HashCalculator;

    hash_t calculate_type_hash(const std::string &name) const override
    {
        PYBIND11_OVERLOAD_PURE(hash_t, HashCalculator, calculate_type_hash, name);
    }

    hash_t calculate_property_hash(const std::string &name) const override
    {
        PYBIND11_OVERLOAD_PURE(hash_t, HashCalculator, calculate_property_hash, name);
    }
};

class PyType : public Type
{
public:
    using Type::Type;

    PropertyClass *instantiate() const override
    {
        PYBIND11_OVERLOAD(PropertyClass *, Type, instantiate, );
    }
};

class PyPropertyBase : public PropertyBase
{
public:
    using PropertyBase::PropertyBase;

    bool is_pointer() const override
    {
        PYBIND11_OVERLOAD(bool, PropertyBase, is_pointer, );
    }

    bool is_dynamic() const override
    {
        PYBIND11_OVERLOAD(bool, PropertyBase, is_dynamic, );
    }

    Value get_value() const override
    {
        PYBIND11_OVERLOAD_PURE(Value, PropertyBase, get_value, );
    }

    const PropertyClass *get_object() const override
    {
        PYBIND11_OVERLOAD_PURE(const PropertyClass *, PropertyBase, get_object, );
    }

    void write_value_to(BitStream &stream) const override
    {
        PYBIND11_OVERLOAD_PURE(void, PropertyBase, write_value_to, stream);
    }

    void read_value_from(BitStream &stream) override
    {
        PYBIND11_OVERLOAD_PURE(void, PropertyBase, read_value_from, stream);
    }
};

class PyDynamicPropertyBase : public DynamicPropertyBase
{
public:
    using DynamicPropertyBase::DynamicPropertyBase;

    bool is_pointer() const override
    {
        PYBIND11_OVERLOAD(bool, DynamicPropertyBase, is_pointer, );
    }

    std::size_t get_element_count() const override
    {
        PYBIND11_OVERLOAD_PURE(std::size_t, DynamicPropertyBase, get_element_count, );
    }

    Value get_value(int index) const override
    {
        PYBIND11_OVERLOAD_PURE(Value, DynamicPropertyBase, get_value, index);
    }

    const PropertyClass *get_object(int index) const override
    {
        PYBIND11_OVERLOAD_PURE(const PropertyClass *, DynamicPropertyBase, get_object, index);
    }

    void write_value_to(BitStream &stream, int index) const override
    {
        PYBIND11_OVERLOAD_PURE(void, DynamicPropertyBase, write_value_to, stream, index);
    }

    void read_value_from(BitStream &stream, int index) override
    {
        PYBIND11_OVERLOAD_PURE(void, DynamicPropertyBase, read_value_from, stream, index);
    }
};

class TypePublicist : public Type
{
public:
    using Type::m_kind;
};

class TypeSystemPublicist : public TypeSystem
{
public:
    using TypeSystem::define_type;
};

class PropertyListPublicist : public PropertyList
{
public:
    using PropertyList::add_property;
};
}

PYBIND11_MODULE(pclass, m)
{
    // Submodules
    py::module primitive_types_submodule = m.def_submodule("primitive_types");

    // Classes
    py::class_<Type, PyType> type_cls(m, "Type");
    py::class_<TypeSystem> type_system_cls(m, "TypeSystem");
    py::class_<PropertyClass> property_class_cls(m, "PropertyClass", py::metaclass((PyObject *)&PyType_Type));
    py::class_<PropertyList> property_list_cls(m, "PropertyList");
    py::class_<PropertyBase, PyPropertyBase> property_base_cls(m, "PropertyBase");
    py::class_<DynamicPropertyBase, PropertyBase, PyDynamicPropertyBase> dynamic_property_base_cls(m, "DynamicPropertyBase");

    py::class_<HashCalculator, PyHashCalculator>(m, "HashCalculator")
        .def(py::init<>())
        .def("calculate_type_hash", &HashCalculator::calculate_type_hash, "name"_a)
        .def("calculate_property_hash", &HashCalculator::calculate_property_hash, "name"_a);

    py::class_<WizardHashCalculator, HashCalculator>(m, "WizardHashCalculator")
        .def(py::init<>());

    py::enum_<Type::kind>(m, "TypeKind")
        .value("NONE", Type::kind::NONE)
        .value("PRIMITIVE", Type::kind::PRIMITIVE)
        .value("CLASS", Type::kind::CLASS)
        .value("ENUM", Type::kind::ENUM);

    // Type Definitions
    type_cls.def(py::init<const std::string &, const TypeSystem &>(), "name"_a, "type_system"_a);
    type_cls.def_readwrite("kind", &TypePublicist::m_kind);
    type_cls.def_property_readonly("name", &Type::get_name);
    type_cls.def_property_readonly("hash", &Type::get_hash);
    type_cls.def_property_readonly("type_system", &Type::get_type_system);
    type_cls.def("instantiate", &Type::instantiate, py::return_value_policy::take_ownership);

    // PrimitiveType Definitions
    def_primitive_type_class<bool>(primitive_types_submodule, "_bool_PrimitiveType");
    def_primitive_type_class<int8_t>(primitive_types_submodule, "_int8_PrimitiveType");
    def_primitive_type_class<uint8_t>(primitive_types_submodule, "_uint8_PrimitiveType");
    def_primitive_type_class<int16_t>(primitive_types_submodule, "_int16_PrimitiveType");
    def_primitive_type_class<uint16_t>(primitive_types_submodule, "_uint16_PrimitiveType");
    def_primitive_type_class<int32_t>(primitive_types_submodule, "_int32_PrimitiveType");
    def_primitive_type_class<uint32_t>(primitive_types_submodule, "_uint32_PrimitiveType");
    def_primitive_type_class<int64_t>(primitive_types_submodule, "_int64_PrimitiveType");
    def_primitive_type_class<uint64_t>(primitive_types_submodule, "_uint64_PrimitiveType");

    def_primitive_type_class<ki::bi<1>>(primitive_types_submodule, "_bi1_PrimitiveType");
    def_primitive_type_class<ki::bi<2>>(primitive_types_submodule, "_bi2_PrimitiveType");
    def_primitive_type_class<ki::bi<3>>(primitive_types_submodule, "_bi3_PrimitiveType");
    def_primitive_type_class<ki::bi<4>>(primitive_types_submodule, "_bi4_PrimitiveType");
    def_primitive_type_class<ki::bi<5>>(primitive_types_submodule, "_bi5_PrimitiveType");
    def_primitive_type_class<ki::bi<6>>(primitive_types_submodule, "_bi6_PrimitiveType");
    def_primitive_type_class<ki::bi<7>>(primitive_types_submodule, "_bi7_PrimitiveType");
    def_primitive_type_class<ki::bi<24>>(primitive_types_submodule, "_bi24_PrimitiveType");

    def_primitive_type_class<ki::bui<1>>(primitive_types_submodule, "_bui1_PrimitiveType");
    def_primitive_type_class<ki::bui<2>>(primitive_types_submodule, "_bui2_PrimitiveType");
    def_primitive_type_class<ki::bui<3>>(primitive_types_submodule, "_bui3_PrimitiveType");
    def_primitive_type_class<ki::bui<4>>(primitive_types_submodule, "_bui4_PrimitiveType");
    def_primitive_type_class<ki::bui<5>>(primitive_types_submodule, "_bui5_PrimitiveType");
    def_primitive_type_class<ki::bui<6>>(primitive_types_submodule, "_bui6_PrimitiveType");
    def_primitive_type_class<ki::bui<7>>(primitive_types_submodule, "_bui7_PrimitiveType");
    def_primitive_type_class<ki::bui<24>>(primitive_types_submodule, "_bui24_PrimitiveType");

    def_primitive_type_class<float>(primitive_types_submodule, "_float_PrimitiveType");
    def_primitive_type_class<double>(primitive_types_submodule, "_double_PrimitiveType");

    def_primitive_type_class<std::string>(primitive_types_submodule, "_string_PrimitiveType");
    def_primitive_type_class<std::wstring>(primitive_types_submodule, "_wstring_PrimitiveType");

    // TypeSystem Definitions
    type_system_cls.def(py::init<HashCalculator *>(), "hash_calculator"_a);
    type_system_cls.def("__contains__",
        static_cast<bool (TypeSystem::*)(const std::string &) const>(&TypeSystem::has_type));
    type_system_cls.def("__contains__",
        static_cast<bool (TypeSystem::*)(hash_t) const>(&TypeSystem::has_type));
    type_system_cls.def("__getitem__", 
        static_cast<const Type &(TypeSystem::*)(const std::string &) const>(&TypeSystem::get_type),
        py::return_value_policy::reference);
    type_system_cls.def("__getitem__",
        static_cast<const Type &(TypeSystem::*)(hash_t) const>(&TypeSystem::get_type),
        py::return_value_policy::reference);
    type_system_cls.def_property("hash_calculator",
        &TypeSystem::get_hash_calculator,
        &TypeSystem::set_hash_calculator, py::return_value_policy::reference);
    type_system_cls.def("has_type",
        static_cast<bool (TypeSystem::*)(const std::string &) const>(&TypeSystem::has_type), "name"_a);
    type_system_cls.def("has_type",
        static_cast<bool (TypeSystem::*)(hash_t) const>(&TypeSystem::has_type), "hash"_a);
    type_system_cls.def("get_type",
        static_cast<const Type &(TypeSystem::*)(const std::string &) const>(&TypeSystem::get_type),
        "name"_a, py::return_value_policy::reference);
    type_system_cls.def("get_type",
        static_cast<const Type &(TypeSystem::*)(hash_t) const>(&TypeSystem::get_type),
        "hash"_a, py::return_value_policy::reference);
    type_system_cls.def("define_type", &TypeSystemPublicist::define_type, "type"_a);

    // PropertyClass Definitions
    property_class_cls.def(py::init<const Type &, const TypeSystem &>(), "type"_a, "type_system"_a);
    property_class_cls.def_property_readonly("type", &PropertyClass::get_type, py::return_value_policy::reference);
    property_class_cls.def_property_readonly("properties",
        static_cast<PropertyList &(PropertyClass::*)()>(&PropertyClass::get_properties),
        py::return_value_policy::reference);

    // PropertyList Definitions
    property_list_cls.def(py::init<>());
    property_list_cls.def("__contains__",
        static_cast<bool (PropertyList::*)(const std::string &) const>(&PropertyList::has_property));
    property_list_cls.def("__contains__",
        static_cast<bool (PropertyList::*)(hash_t) const>(&PropertyList::has_property));
    property_list_cls.def("__getitem__",
        static_cast<const PropertyBase &(PropertyList::*)(int) const>(&PropertyList::get_property),
        py::return_value_policy::reference);
    property_list_cls.def("__getitem__",
        static_cast<const PropertyBase &(PropertyList::*)(const std::string &) const>(&PropertyList::get_property),
        py::return_value_policy::reference);
    property_list_cls.def("__getitem__",
        static_cast<const PropertyBase &(PropertyList::*)(hash_t) const>(&PropertyList::get_property),
        py::return_value_policy::reference);
    property_list_cls.def("__iter__", [](const PropertyList &self)
    {
        return py::make_iterator(self.begin(), self.end());
    }, py::keep_alive<0, 1>());
    property_list_cls.def_property_readonly("property_count", &PropertyList::get_property_count);
    property_list_cls.def("has_property",
        static_cast<bool (PropertyList::*)(const std::string &) const>(&PropertyList::has_property), "name"_a);
    property_list_cls.def("has_property",
        static_cast<bool (PropertyList::*)(hash_t) const>(&PropertyList::has_property), "hash"_a);
    property_list_cls.def("get_property",
        static_cast<const PropertyBase &(PropertyList::*)(int) const>(&PropertyList::get_property),
        "name"_a, py::return_value_policy::reference);
    property_list_cls.def("get_property",
        static_cast<const PropertyBase &(PropertyList::*)(const std::string &) const>(&PropertyList::get_property),
        "hash"_a, py::return_value_policy::reference);
    property_list_cls.def("get_property",
        static_cast<const PropertyBase &(PropertyList::*)(hash_t) const>(&PropertyList::get_property),
        "hash"_a, py::return_value_policy::reference);
    property_list_cls.def("add_property", &PropertyListPublicist::add_property, "prop"_a);

    // PropertyBase Definitions
    property_base_cls.def(py::init<PropertyClass &, const std::string &, const Type &>(),
        "obj"_a, "name"_a, "type"_a);
    property_base_cls.def_property_readonly("name", &PropertyBase::get_name);
    property_base_cls.def_property_readonly("name_hash", &PropertyBase::get_name_hash);
    property_base_cls.def_property_readonly("full_hash", &PropertyBase::get_full_hash);
    property_base_cls.def_property_readonly("type", &PropertyBase::get_type, py::return_value_policy::reference);
    property_base_cls.def("is_pointer", &PropertyBase::is_pointer);
    property_base_cls.def("is_dynamic", &PropertyBase::is_dynamic);
    property_base_cls.def("get_object", &PropertyBase::get_object);
    property_base_cls.def("write_value_to", &PropertyBase::write_value_to, "stream"_a);
    property_base_cls.def("read_value_from", &PropertyBase::read_value_from, "stream"_a);

    // DynamicPropertyBase Definitions
    dynamic_property_base_cls.def(py::init<PropertyClass &, const std::string &, const Type &>(),
        "obj"_a, "name"_a, "type"_a);
    dynamic_property_base_cls.def("get_element_count", &DynamicPropertyBase::get_element_count);
    dynamic_property_base_cls.def("get_object",
        static_cast<const PropertyClass *(DynamicPropertyBase::*)(int) const>(&DynamicPropertyBase::get_object),
        "index"_a);
    dynamic_property_base_cls.def("write_value_to",
        static_cast<void (DynamicPropertyBase::*)(BitStream &, int) const>(&DynamicPropertyBase::write_value_to),
        "stream"_a, "index"_a);
    dynamic_property_base_cls.def("read_value_from",
        static_cast<void (DynamicPropertyBase::*)(BitStream &, int)>(&DynamicPropertyBase::read_value_from),
        "stream"_a, "index"_a);
}

}
}
