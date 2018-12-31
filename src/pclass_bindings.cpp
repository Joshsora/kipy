#include <pybind11/pybind11.h>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include <ki/util/unique.h>
#include <ki/util/BitTypes.h>
#include <ki/pclass/Value.h>
#include <ki/pclass/types/Type.h>
#include <ki/pclass/types/PrimitiveType.h>
#include <ki/pclass/TypeSystem.h>
#include <ki/pclass/HashCalculator.h>
#include <ki/pclass/PropertyClass.h>
#include <ki/pclass/PropertyList.h>
#include <ki/pclass/Property.h>
#include <ki/pclass/VectorProperty.h>

#define DECLARE_VALUE_CASTER(t) \
    ValueCaster::declare<py::object, t>(); \
    ValueCaster::declare<t, py::object>()

#define DECLARE_INTEGER_VALUE_CASTER(st, ut) \
    DECLARE_VALUE_CASTER(st); \
    DECLARE_VALUE_CASTER(ut)

#define DECLARE_BIT_INTEGER_VALUE_CASTER(n) \
    DECLARE_VALUE_CASTER(bi<n>); \
    DECLARE_VALUE_CASTER(bui<n>)

namespace py = pybind11;
using namespace pybind11::literals;

namespace ki
{
namespace pclass
{

namespace detail
{
    /**
    * Determines whether a type can be assigned to a py::object.
    */
    template <typename SrcT, typename Enable = void>
    struct is_py_assignable : std::false_type {};

    /**
    * All fundamental types can be assigned to a py::object.
    */
    template <typename SrcT>
    struct is_py_assignable<
        SrcT,
        typename std::enable_if<std::is_fundamental<SrcT>::value>::type
    > : std::true_type {};

    /**
    * std::string can be assigned to a py::object.
    */
    template <>
    struct is_py_assignable<std::string> : std::true_type {};

    /**
    * std::u16string can be assigned to a py::object.
    */
    template <>
    struct is_py_assignable<std::u16string> : std::true_type {};

    /**
    * BitInteger can be assigned to a py::object.
    */
    template <uint8_t N, bool Unsigned>
    struct is_py_assignable<BitInteger<N, Unsigned>> : std::true_type {};

    /**
    * value_caster specialization for casting py::object to any
    * py::object-assignable value.
    */
    template <typename DestT>
    struct value_caster<
        py::object, DestT,
        typename std::enable_if<is_py_assignable<DestT>::value>::type
    > : value_caster_impl<py::object, DestT>
    {
        DestT cast_value(const py::object &value) const override
        {
            try
            {
                return value.cast<DestT>();
            }
            catch (py::cast_error &e)
            {
                return bad_cast();
            }
        }
    };

    /**
    * value_caster specialization for casting py::object to an
    * std::string.
    */
    template <>
    struct value_caster<py::object, std::string>
        : value_caster_impl<py::object, std::string>
    {
        std::string cast_value(const py::object &value) const override
        {
            try
            {
                return value.cast<std::string>();
            }
            catch (py::cast_error &e)
            {
                return bad_cast();
            }
        }
    };

    /**
    * value_caster specialization for casting any py::object-assignable
    * value to a py::object.
    */
    template <typename SrcT>
    struct value_caster<
        SrcT, py::object,
        typename std::enable_if<is_py_assignable<SrcT>::value>::type
    > : value_caster_impl<SrcT, py::object>
    {
        py::object cast_value(const SrcT &value) const override
        {
            try
            {
                return py::cast(value);
            }
            catch (py::cast_error &e)
            {
                return bad_cast();
            }
        }
    };
}

namespace
{
    /**
    * A Python alias class for HashCalculator.
    */
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

    /**
    * A Python-based ClassType.
    * PropertyClass instances will be instantiated from the given
    * Python-defined class.
    */
    class PyClassType : public IClassType
    {
    public:
        PyClassType(const std::string &name, const py::object &cls,
            const Type *base_class, const TypeSystem &type_system)
            : IClassType(name, base_class, type_system)
        {
            // Verify that this is a valid class type.
            auto *type_ptr = cls.ptr();
            if (!PyType_Check(type_ptr))
            {
                throw std::runtime_error("Invalid class object passed to ClassType constructor.");
            }
            
            // TODO: Verify that the subclass is PropertyClass.
            m_cls = cls;
        }

        std::unique_ptr<PropertyClass> instantiate() const override
        {
            // Python only knows about the Type base class.
            const auto &type = static_cast<const Type &>(*this);

            // Instantiate the Python object.
            py::object type_system = py::cast(get_type_system());  // Cast to PyTypeSystem
            py::object instance = m_cls(type, type_system);

            // Increment the reference count so that it isn't destroyed
            // when the stack is cleaned up.
            // At a later point, instance.dec_ref() should be called to
            // avoid memory leaks.
            instance.inc_ref();

            return std::unique_ptr<PropertyClass>(
                instance.cast<PropertyClass *>()
            );
        }

        void write_to(BitStream &stream, Value value) const override
        {
            const auto &object = value.dereference<PropertyClass>().get<PropertyClass>();
            const auto &properties = object.get_properties();
            for (auto it = properties.begin(); it != properties.end(); ++it)
                it->write_value_to(stream);
        }

        Value read_from(BitStream &stream) const override
        {
            auto &object = *instantiate();
            auto &properties = object.get_properties();
            for (auto it = properties.begin(); it != properties.end(); ++it)
                it->read_value_from(stream);
            return Value::make_reference<PropertyClass>(object);
        }

    private:
        py::object m_cls;
    };

    /**
    * A Python-based TypeSystem.
    * Provides a method for defining class types, and instantiating
    * Python-defined PropertyClass objects.
    */
    class PyTypeSystem : public TypeSystem
    {
    public:
        using TypeSystem::TypeSystem;

        void define_class(const std::string &name, const py::object &cls, const Type *base_class)
        {
            define_type(std::unique_ptr<Type>(new PyClassType(name, cls, base_class, *this)));
        }

        py::object instantiate(const std::string &name)
        {
            // Instantiate the PropertyClass, and cast it to its Python representation.
            const auto &type = get_type(name);
            py::object instance = py::cast(type.instantiate().release());

            // This object was just instantiated, and as such now has a
            // reference count >= 2; decrement the reference count to avoid
            // memory leaks.
            instance.dec_ref();

            return instance;
        }
    };

    /**
    * A Python-based StaticProperty.
    * This makes use of py::object as a way of storing the value,
    * rather than ki::Value.
    */
    class PyStaticProperty : public IProperty
    {
    public:
        PyStaticProperty(PropertyClass &object, const std::string &name,
            const Type &type, bool is_pointer = false)
            : IProperty(object, name, type)
        {
            m_is_pointer = is_pointer;
        }

        bool is_pointer() const override
        {
            return m_is_pointer;
        }

        bool is_dynamic() const override
        {
            return false;
        }

        bool is_array() const override
        {
            return false;
        }

        std::size_t get_element_count() const override
        {
            return 1;
        }

        void set_element_count(std::size_t size) override
        {
            throw std::runtime_error("Tried to call set_element_count() on a property that is not dynamic.");
        }

        Value get_value(std::size_t index) const override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");
            return Value::make_reference(m_value);
        }

        void set_value(Value value, const std::size_t index) override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");
            m_value = value.dereference<py::object>().get<py::object>();
        }

        const PropertyClass *get_object(const std::size_t index) const override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");

            try
            {
                return m_value.cast<const PropertyClass *>();
            }
            catch (py::cast_error &e)
            {
                std::ostringstream oss;
                oss << "Failed to cast value of StaticProperty to PropertyClass -- " << e.what();
                throw std::runtime_error(oss.str());
            }
        }

        void set_object(std::unique_ptr<PropertyClass> &object, const std::size_t index) override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");

            m_value = py::cast(dynamic_cast<PropertyClass *>(object.get()));

            // This object was just instantiated, and as such now has a
            // reference count >= 2; decrement the reference count to
            // avoid memory leaks.
            m_value.dec_ref();
        }

        py::object &get()
        {
            return m_value;
        }

        const py::object &get() const
        {
            return m_value;
        }

        void set(const py::object &value)
        {
            m_value = value;
        }

    private:
        bool m_is_pointer;
        py::object m_value;
    };

    /**
    * A Python-based VectorProperty.
    * This makes use of py::list as a way of storing the value,
    * rather than std::vector.
    */
    class PyVectorProperty : public IProperty
    {
    public:
        PyVectorProperty(PropertyClass &object, const std::string &name,
            const Type &type, bool is_pointer = false)
            : IProperty(object, name, type)
        {
            m_is_pointer = is_pointer;
        }

        bool is_pointer() const override
        {
            return m_is_pointer;
        }

        bool is_dynamic() const override
        {
            return true;
        }

        bool is_array() const override
        {
            return true;
        }

        std::size_t get_element_count() const override
        {
            return m_list.size();
        }
        
        void set_element_count(const std::size_t size) override
        {
            // Empty the list.
            m_list = py::list();

            // Fill it with NoneTypes up to the desired size.
            for (auto i = 0; i < size; i++)
                m_list.append(nullptr);
        }

        Value get_value(std::size_t index) const override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");
            return Value::make_reference(m_list[index]);
        }

        void set_value(Value value, const std::size_t index) override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");
            m_list[index] = value.dereference<py::object>().get<py::object>();
        }

        const PropertyClass *get_object(const std::size_t index) const override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");

            try
            {
                return m_list[index].cast<const PropertyClass *>();
            }
            catch (py::cast_error &e)
            {
                std::ostringstream oss;
                oss << "Failed to cast value of VectorProperty.at(" << index <<
                    ") to PropertyClass -- " << e.what();
                throw std::runtime_error(oss.str());
            }
        }

        void set_object(std::unique_ptr<PropertyClass> &object, const std::size_t index) override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");

            py::object py_object = py::cast(dynamic_cast<PropertyClass *>(object.get()));
            m_list[index] = py_object;

            // This object was just instantiated, and as such now has a
            // reference count >= 2; decrement the reference count to
            // avoid memory leaks.
            py_object.dec_ref();
        }

        py::list &get()
        {
            return m_list;
        }

        const py::list &get() const
        {
            return m_list;
        }

        void set(const py::list &value)
        {
            m_list = value;
        }

    private:
        bool m_is_pointer;
        py::list m_list;
    };

    /**
    * A Python alias class for PropertyClass.
    */
    class PyPropertyClass : public PropertyClass
    {
    public:
        using PropertyClass::PropertyClass;

        void on_created() const override
        {
            PYBIND11_OVERLOAD(void, PropertyClass, on_created, );
        }
    };

    /**
    * Allows for public access of the Type::m_kind attribute.
    */
    class TypePublicist : public Type
    {
    public:
        using Type::m_kind;
    };

    /**
    * TODO: Documentation.
    */
    class PropertyDef
    {
    public:
        PropertyDef(
            const std::string &name, const std::string &type_name,
            bool is_pointer = false)
        {
            m_name = name;
            m_type_name = type_name;
            m_is_pointer = is_pointer;
        }

        std::string get_name() const
        {
            return m_name;
        }

        std::string get_type_name() const
        {
            return m_type_name;
        }

        bool is_pointer() const
        {
            return m_is_pointer;
        }

        virtual void instantiate(PropertyClass &object) const
        {
            std::ostringstream oss;
            oss << "PropertyDef '" << m_name << "' does not implement PropertyDef::instantiate.";
            throw std::runtime_error(oss.str());
        }
        
    private:
        std::string m_name;
        std::string m_type_name;
        bool m_is_pointer;
    };

    /**
    * TODO: Documentation.
    */
    class StaticPropertyDef : public PropertyDef
    {
    public:
        using PropertyDef::PropertyDef;

        void instantiate(PropertyClass &object) const override
        {
            const auto &type_system = object.get_type().get_type_system();
            const auto &type = type_system.get_type(get_type_name());
            new PyStaticProperty(object, get_name(), type, is_pointer());
        }
    };

    /**
    * TODO: Documentation.
    */
    class VectorPropertyDef : public PropertyDef
    {
    public:
        using PropertyDef::PropertyDef;

        void instantiate(PropertyClass &object) const override
        {
            const TypeSystem &type_system = object.get_type().get_type_system();
            const Type &type = type_system.get_type(get_type_name());
            new PyVectorProperty(object, get_name(), type, is_pointer());
        }
    };

    /**
    * A Python alias class for PropertyDef.
    */
    class PyPropertyDef : public PropertyDef
    {
        using PropertyDef::PropertyDef;

        void instantiate(PropertyClass &object) const override
        {
            PYBIND11_OVERLOAD(void, PropertyDef, instantiate, object);
        }
    };
}

PYBIND11_MODULE(pclass, m)
{
    // Declare py::object->integer value casters.
    DECLARE_VALUE_CASTER(bool);
    DECLARE_INTEGER_VALUE_CASTER(int8_t, uint8_t);
    DECLARE_INTEGER_VALUE_CASTER(int16_t, uint16_t);
    DECLARE_INTEGER_VALUE_CASTER(int32_t, uint32_t);
    DECLARE_INTEGER_VALUE_CASTER(int64_t, uint64_t);
    
    // Declare py::object->BitInteger value casters.
    DECLARE_BIT_INTEGER_VALUE_CASTER(1);
    DECLARE_BIT_INTEGER_VALUE_CASTER(2);
    DECLARE_BIT_INTEGER_VALUE_CASTER(3);
    DECLARE_BIT_INTEGER_VALUE_CASTER(4);
    DECLARE_BIT_INTEGER_VALUE_CASTER(5);
    DECLARE_BIT_INTEGER_VALUE_CASTER(6);
    DECLARE_BIT_INTEGER_VALUE_CASTER(7);
    DECLARE_BIT_INTEGER_VALUE_CASTER(24);

    // Declare py::object->floating point value casters.
    DECLARE_VALUE_CASTER(float);
    DECLARE_VALUE_CASTER(double);

    // Declare py::object->string value casters.
    DECLARE_VALUE_CASTER(std::string);
    DECLARE_VALUE_CASTER(std::u16string);

    // Declare py::object->PropertyClass value caster.
    // DECLARE_VALUE_CASTER(PropertyClass);

    // Classes
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

    py::class_<Type> type_cls(m, "Type");
    py::class_<TypeSystem> _type_system_cls(m, "_TypeSystem");
    py::class_<PyTypeSystem, TypeSystem> type_system_cls(m, "TypeSystem");
    py::class_<PropertyDef, PyPropertyDef> property_def_cls(m, "PropertyDef");
    py::class_<StaticPropertyDef, PropertyDef> static_property_def_cls(m, "StaticProperty");
    py::class_<VectorPropertyDef, PropertyDef> vector_property_def_cls(m, "VectorProperty");
    py::class_<PropertyClass> property_class_cls(m, "PropertyClass", py::metaclass((PyObject *)&PyType_Type));

    // Type Definitions
    type_cls.def(py::init<const std::string &, const TypeSystem &>(),
        "name"_a, "type_system"_a);
    type_cls.def_property_readonly("name", &Type::get_name);
    type_cls.def_property_readonly("hash", &Type::get_hash);
    type_cls.def_readwrite("kind", &TypePublicist::m_kind);
    type_cls.def_property_readonly("type_system", &Type::get_type_system);

    // _TypeSystem Definitions
    _type_system_cls.def("__init__", [](TypeSystem &self, HashCalculator &hash_calculator)
    {
        new (&self) TypeSystem { std::unique_ptr<HashCalculator>(&hash_calculator) };
    });
    _type_system_cls.def_property_readonly("hash_calculator",
        &TypeSystem::get_hash_calculator, py::return_value_policy::reference);
    _type_system_cls.def("has_type",
        static_cast<bool (TypeSystem::*)(const std::string &) const>(&TypeSystem::has_type), "name"_a);
    _type_system_cls.def("__contains__",
        static_cast<bool (TypeSystem::*)(const std::string &) const>(&TypeSystem::has_type));
    _type_system_cls.def("has_type",
        static_cast<bool (TypeSystem::*)(hash_t) const>(&TypeSystem::has_type), "hash"_a);
    _type_system_cls.def("__contains__",
        static_cast<bool (TypeSystem::*)(hash_t) const>(&TypeSystem::has_type));
    _type_system_cls.def("get_type",
        static_cast<const Type &(TypeSystem::*)(const std::string &) const>(&TypeSystem::get_type),
        "name"_a, py::return_value_policy::reference_internal);
    _type_system_cls.def("__getitem__",
        static_cast<const Type &(TypeSystem::*)(const std::string &) const>(&TypeSystem::get_type),
        py::return_value_policy::reference_internal);
    _type_system_cls.def("get_type",
        static_cast<const Type &(TypeSystem::*)(hash_t) const>(&TypeSystem::get_type),
        "hash"_a, py::return_value_policy::reference_internal);
    _type_system_cls.def("__getitem__",
        static_cast<const Type &(TypeSystem::*)(hash_t) const>(&TypeSystem::get_type),
        py::return_value_policy::reference_internal);

    // TypeSystem Definitions
    type_system_cls.def("__init__", [](PyTypeSystem &self, HashCalculator *hash_calculator)
    {
        new (&self) PyTypeSystem { std::unique_ptr<HashCalculator>(hash_calculator) };
    });
    type_system_cls.def("define_class", &PyTypeSystem::define_class,
        "name"_a, "cls"_a, "base_class"_a);
    type_system_cls.def("instantiate", &PyTypeSystem::instantiate,
        py::return_value_policy::take_ownership, "name"_a);

    // PropertyDef Definitions
    property_def_cls.def(py::init<const std::string &, const std::string &, bool>(),
        "name"_a, "type_name"_a, "is_pointer"_a = false);
    property_def_cls.def("instantiate", &PropertyDef::instantiate, "object"_a);

    // StaticPropertyDef Definitions
    static_property_def_cls.def(py::init<const std::string &, const std::string &, bool>(),
        "name"_a, "type_name"_a, "is_pointer"_a = false);
    static_property_def_cls.def("__get__", [](const StaticPropertyDef &self, const py::object &instance, const py::object &owner)
    {
        // Are we attempting to read the value of this property from a
        // PropertyClass instance?
        if (py::isinstance<PropertyClass>(instance))
        {
            // Yep. Read the value of the property.
            const auto &object = instance.cast<const PropertyClass &>();
            const PropertyList &properties = object.get_properties();
            const IProperty &prop = properties.get_property(self.get_name());

            // Cast the property to a PyStaticProperty.
            const auto &static_prop = dynamic_cast<const PyStaticProperty &>(prop);
            return static_prop.get();
        }

        // Nope. Simply return this PropertyDef instance.
        return py::cast(self);
    }, py::return_value_policy::reference_internal);
    static_property_def_cls.def("__set__", [](const StaticPropertyDef &self, py::object &instance, py::object &value)
    {
        // Are we attempting to set the value of this property on a
        // PropertyClass instance?
        if (py::isinstance<PropertyClass>(instance))
        {
            // Yep. Set the value of the property.
            auto &object = instance.cast<PropertyClass &>();
            PropertyList &properties = object.get_properties();
            IProperty &prop = properties.get_property(self.get_name());

            // Cast the property to a PyStaticProperty.
            auto &static_prop = dynamic_cast<PyStaticProperty &>(prop);
            static_prop.set(value);
        }
    });

    // VectorPropertyDef Definitions
    vector_property_def_cls.def(py::init<const std::string &, const std::string &, bool>(),
        "name"_a, "type_name"_a, "is_pointer"_a = false);
    vector_property_def_cls.def("__get__", [](const VectorPropertyDef &self, const py::object &instance, const py::object &owner)
    {
        // Are we attempting to read the value of this property from a
        // PropertyClass instance?
        if (py::isinstance<PropertyClass>(instance))
        {
            // Yep. Read the value of the property.
            const auto &object = instance.cast<const PropertyClass &>();
            const PropertyList &properties = object.get_properties();
            const IProperty &prop = properties.get_property(self.get_name());

            // Cast the property to a PyVectorProperty.
            const auto &vector_prop = dynamic_cast<const PyVectorProperty &>(prop);
            return py::object(vector_prop.get());
        }

        // Nope. Simply return this PropertyDef instance.
        return py::cast(self);
    }, py::return_value_policy::reference_internal);
    vector_property_def_cls.def("__set__", [](const VectorPropertyDef &self, py::object &instance, py::object &value)
    {
        // Are we attempting to set the value of this property on a
        // PropertyClass instance?
        if (py::isinstance<PropertyClass>(instance))
        {
            // Yep. Set the value of the property.
            auto &object = instance.cast<PropertyClass &>();
            PropertyList &properties = object.get_properties();
            IProperty &prop = properties.get_property(self.get_name());

            // Cast the property to a PyVectorProperty.
            auto &vector_prop = dynamic_cast<PyVectorProperty &>(prop);
            vector_prop.set(value);
        }
    });

    // PropertyClass Definitions
    property_class_cls.def(py::init<const Type &, const TypeSystem &>(), "type"_a, "type_system"_a);
    property_class_cls.def_property_readonly("type", &PropertyClass::get_type);
}

}
}
