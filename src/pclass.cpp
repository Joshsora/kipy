#include <python.h>
#include <pybind11/pybind11.h>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include <ki/util/unique.h>
#include <ki/util/BitTypes.h>
#include <ki/pclass/Value.h>
#include <ki/pclass/Type.h>
#include <ki/pclass/PrimitiveType.h>
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
    DECLARE_VALUE_CASTER(ki::bi<n>); \
    DECLARE_VALUE_CASTER(ki::bui<n>)

namespace py = pybind11;

using namespace pybind11::literals;

using ki::BitStream;

using ki::pclass::hash_t;

using ki::pclass::IHashCalculator;
using ki::pclass::IClassType;
using ki::pclass::IProperty;

using ki::pclass::WizardHashCalculator;
using ki::pclass::Value;
using ki::pclass::ValueCaster;
using ki::pclass::Type;
using ki::pclass::TypeSystem;
using ki::pclass::PropertyClass;
using ki::pclass::PropertyList;

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
    * PropertyClass can be assigned to a py::object.
    */
    template <>
    struct is_py_assignable<PropertyClass> : std::true_type {};

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

    /**
    * value_caster specialization for casting any signed bit integer
    * value to a py::object.
    */
    template <uint8_t N>
    struct value_caster<ki::BitInteger<N, false>, py::object>
        : value_caster_impl<ki::BitInteger<N, false>, py::object>
    {
        py::object cast_value(const ki::BitInteger<N, false> &value) const override
        {
            try
            {
                return py::cast(static_cast<int64_t>(value));
            }
            catch (py::cast_error &e)
            {
                return bad_cast();
            }
        }
    };

    /**
    * value_caster specialization for casting any unsigned bit integer
    * value to a py::object.
    */
    template <uint8_t N>
    struct value_caster<ki::BitInteger<N, true>, py::object>
        : value_caster_impl<ki::BitInteger<N, true>, py::object>
    {
        py::object cast_value(const ki::BitInteger<N, true> &value) const override
        {
            try
            {
                return py::cast(static_cast<uint64_t>(value));
            }
            catch (py::cast_error &e)
            {
                return bad_cast();
            }
        }
    };

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
    * value_caster specialization for casting py::object to any
    * signed bit integer value that is <= 32 bits.
    */
    template <uint8_t N>
    struct value_caster<
        py::object, ki::BitInteger<N, false>,
        typename std::enable_if<(N <= 32)>::type
    > : value_caster_impl<py::object, ki::BitInteger<N, false>>
    {
        ki::BitInteger<N, false> cast_value(const py::object &value) const override
        {
            auto *value_ptr = value.ptr();
            if (PyLong_Check(value_ptr))
                return PyLong_AsLong(value_ptr);
            return bad_cast();
        }
    };

    /**
    * value_caster specialization for casting py::object to any
    * signed bit integer value that is > 32 bits.
    */
    template <uint8_t N>
    struct value_caster<
        py::object, ki::BitInteger<N, false>,
        typename std::enable_if<(N > 32)>::type
    > : value_caster_impl<py::object, ki::BitInteger<N, false>>
    {
        ki::BitInteger<N, false> cast_value(const py::object &value) const override
        {
            auto *value_ptr = value.ptr();
            if (PyLong_Check(value_ptr))
                return PyLong_AsLongLong(value_ptr);
            return bad_cast();
        }
    };

    /**
    * value_caster specialization for casting py::object to any
    * unsigned bit integer value that is <= 32 bits.
    */
    template <uint8_t N>
    struct value_caster<
        py::object, ki::BitInteger<N, true>,
        typename std::enable_if<(N <= 32)>::type
    > : value_caster_impl<py::object, ki::BitInteger<N, true>>
    {
        ki::BitInteger<N, true> cast_value(const py::object &value) const override
        {
            auto *value_ptr = value.ptr();
            if (PyLong_Check(value_ptr))
                return PyLong_AsUnsignedLong(value_ptr);
            return bad_cast();
        }
    };

    /**
    * value_caster specialization for casting py::object to any
    * signed bit integer value that is > 32 bits.
    */
    template <uint8_t N>
    struct value_caster<
        py::object, ki::BitInteger<N, true>,
        typename std::enable_if<(N > 32)>::type
    > : value_caster_impl<py::object, ki::BitInteger<N, true>>
    {
        ki::BitInteger<N, true> cast_value(const py::object &value) const override
        {
            auto *value_ptr = value.ptr();
            if (PyLong_Check(value_ptr))
                return PyLong_AsUnsignedLongLong(value_ptr);
            return bad_cast();
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
    * value_caster specialization for casting py::object to an
    * std::u16string.
    */
    template <>
    struct value_caster<py::object, std::u16string>
        : value_caster_impl<py::object, std::u16string>
    {
        std::u16string cast_value(const py::object &value) const override
        {
            try
            {
                return value.cast<std::u16string>();
            }
            catch (py::cast_error &e)
            {
                return bad_cast();
            }
        }
    };

    /**
    * value_caster specialization for casting a py::object value to a
    * nlohmann::json value.
    */
    template <>
    struct value_caster<py::object, nlohmann::json>
        : value_caster_impl<py::object, nlohmann::json>
    {
        nlohmann::json cast_value(const py::object &value) const override
        {
            auto *value_ptr = value.ptr();
            if (PyBool_Check(value_ptr))
                return static_cast<bool>(PyObject_IsTrue(value_ptr));
            else if (PyLong_Check(value_ptr))
                return PyLong_AsLong(value_ptr);
            else if (PyFloat_Check(value_ptr))
                return PyFloat_AsDouble(value_ptr);
            else if (PyBytes_Check(value_ptr))
                return PyBytes_AsString(value_ptr);
            else if (PyUnicode_Check(value_ptr))
                return PyUnicode_AsUTF8(value_ptr);
            else if (!PyType_Check(value_ptr) &&
                PyObject_HasAttrString(value_ptr, "__int__"))
                return value.attr("__int__")().cast<long>();
            else
                return bad_cast();
        }
    };

    /**
    * value_caster specialization for casting a nlohmann::json to a
    * py::object value.
    */
    template <>
    struct value_caster<nlohmann::json, py::object>
        : value_caster_impl<nlohmann::json, py::object>
    {
        py::object cast_value(const nlohmann::json &value) const override
        {
            if (value.is_null())
                return py::cast(nullptr);
            else if (value.is_boolean())
                return py::cast(value.get<bool>());
            else if (value.is_number_integer())
                return py::cast(value.get<int64_t>());
            else if (value.is_number_unsigned())
                return py::cast(value.get<uint64_t>());
            else if (value.is_number_float())
                return py::cast(value.get<double>());
            else if (value.is_string())
                return py::cast(value.get<std::string>());
            else
                return bad_cast();
        }
    };
}
}
}

namespace
{
    /**
    * A Python alias class for IHashCalculator.
    */
    class PyIHashCalculator : public IHashCalculator
    {
    public:
        using IHashCalculator::IHashCalculator;

        hash_t calculate_type_hash(const std::string &name) const override
        {
            PYBIND11_OVERLOAD_PURE(hash_t, IHashCalculator, calculate_type_hash, name);
        }

        hash_t calculate_property_hash(const std::string &name) const override
        {
            PYBIND11_OVERLOAD_PURE(hash_t, IHashCalculator, calculate_property_hash, name);
        }
    };

    /**
    * A Python-based TypeSystem.
    * Provides a method for defining Python-based class types, and
    * declares py::object value casters.
    */
    class PyTypeSystem : public TypeSystem
    {
    public:
        explicit PyTypeSystem(std::unique_ptr<IHashCalculator> &hash_calculator);

        void define_class(const std::string &name, const py::object &cls, const Type *base_class);
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
            const auto *type = static_cast<const Type *>(this);

            // Cast TypeSystem to its Python-based counterpart.
            const auto *type_system = dynamic_cast<const PyTypeSystem *>(&get_type_system());

            // Instantiate the Python object.
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

        void write_to(BitStream &stream, Value &value) const override
        {
            const auto &object = value.as<PropertyClass>().get<PropertyClass>();
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

    PyTypeSystem::PyTypeSystem(std::unique_ptr<IHashCalculator> &hash_calculator)
        : TypeSystem(hash_calculator)
    {
        // Declare py::object->integer value casters.
        DECLARE_VALUE_CASTER(bool);
        DECLARE_INTEGER_VALUE_CASTER(int8_t, uint8_t);
        DECLARE_INTEGER_VALUE_CASTER(int16_t, uint16_t);
        DECLARE_INTEGER_VALUE_CASTER(int32_t, uint32_t);
        DECLARE_INTEGER_VALUE_CASTER(int64_t, uint64_t);

        // Declare py::object->ki::BitInteger value casters.
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
        DECLARE_VALUE_CASTER(PropertyClass);

        // Declare py::object->nlohmann::json value caster.
        DECLARE_VALUE_CASTER(nlohmann::json);
    }

    void PyTypeSystem::define_class(const std::string &name, const py::object &cls, const Type *base_class)
    {
        define_type(
            std::unique_ptr<Type>(
                new PyClassType(name, cls, base_class, *this)
            )
        );
    }

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
            m_value = value.as<py::object>().get<py::object>();
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
            if (size < 0)
                throw std::runtime_error("Invalid element count given to VectorProperty.");

            // Empty the list.
            m_list = py::list();

            // Fill it with with the desired amount of nullptr elements.
            for (auto i = 0; i < size; i++)
                m_list.append(nullptr);
        }

        Value get_value(std::size_t index) const override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");
            return Value::make_value(py::object(m_list[index]));
        }

        void set_value(Value value, const std::size_t index) override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");
            m_list[index] = value.as<py::object>().get<py::object>();
        }

        const PropertyClass *get_object(const std::size_t index) const override
        {
            if (index < 0 || index >= get_element_count())
                throw std::runtime_error("Index out of bounds.");

            try
            {
                return py::object(m_list[index]).cast<const PropertyClass *>();
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

            py::object instance = py::cast(
                dynamic_cast<PropertyClass *>(object.get())
            );
            m_list[index] = instance;

            // This object was just instantiated, and as such now has a
            // reference count >= 2; decrement the reference count to
            // avoid memory leaks.
            instance.dec_ref();
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
    * An extension on PyVectorProperty, which attempts to enforce a
    * fixed size array.
    */
    class PyArrayProperty : public PyVectorProperty
    {
    public:
        PyArrayProperty(PropertyClass &object, const std::string &name,
            const Type &type, std::size_t size, bool is_pointer = false)
            : PyVectorProperty(object, name, type, is_pointer)
        {
            set_element_count(size);
        }

        void set_element_count(const std::size_t size) override
        {
            if (size <= 0)
                throw std::runtime_error("Invalid element count given to ArrayProperty.");
            PyVectorProperty::set_element_count(size);
        }

        void set(const py::list &value)
        {
            if (value.size() != get_element_count())
            {
                std::ostringstream oss;
                oss << "Value of ArrayProperty '" << get_name() <<
                    "' must be of length " << get_element_count() << ".";
                throw std::runtime_error(oss.str());
            }
            PyVectorProperty::set(value);
        }
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
            const auto &type_system = object.get_type().get_type_system();
            const auto &type = type_system.get_type(get_type_name());
            new PyVectorProperty(object, get_name(), type, is_pointer());
        }
    };

    /**
    * TODO: Documentation.
    */
    class ArrayPropertyDef : public PropertyDef
    {
    public:
        ArrayPropertyDef(
            const std::string &name, const std::string &type_name,
            std::size_t size, bool is_pointer = false
        ) : PropertyDef(name, type_name, is_pointer)
        {
            m_size = size;
        }

        void instantiate(PropertyClass &object) const override
        {
            const auto &type_system = object.get_type().get_type_system();
            const auto &type = type_system.get_type(get_type_name());
            new PyArrayProperty(object, get_name(), type, m_size, is_pointer());
        }

    private:
        std::size_t m_size;
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

void bind_pclass(py::module &m)
{
    // Classes
    py::class_<IHashCalculator, PyIHashCalculator>(m, "IHashCalculator")
        .def(py::init<>())
        .def("calculate_type_hash", &IHashCalculator::calculate_type_hash, "name"_a)
        .def("calculate_property_hash", &IHashCalculator::calculate_property_hash, "name"_a);

    py::class_<WizardHashCalculator, IHashCalculator>(m, "WizardHashCalculator")
        .def(py::init<>());

    py::enum_<Type::Kind>(m, "TypeKind")
        .value("NONE", Type::Kind::NONE)
        .value("PRIMITIVE", Type::Kind::PRIMITIVE)
        .value("CLASS", Type::Kind::CLASS)
        .value("ENUM", Type::Kind::ENUM);

    py::class_<Type> type_cls(m, "Type");
    py::class_<TypeSystem, PyTypeSystem> type_system_cls(m, "TypeSystem");
    py::class_<PropertyDef, PyPropertyDef> property_def_cls(m, "PropertyDef");
    py::class_<StaticPropertyDef, PropertyDef> static_property_def_cls(m, "StaticProperty");
    py::class_<VectorPropertyDef, PropertyDef> vector_property_def_cls(m, "VectorProperty");
    py::class_<ArrayPropertyDef, PropertyDef> array_property_def_cls(m, "ArrayProperty");
    py::class_<PropertyClass> property_class_cls(m, "PropertyClass", py::metaclass((PyObject *)&PyType_Type));

    // Type Definitions
    type_cls.def(py::init<const std::string &, const TypeSystem &>(),
        "name"_a, "type_system"_a);
    type_cls.def_property_readonly("name", &Type::get_name);
    type_cls.def_property_readonly("hash", &Type::get_hash);
    type_cls.def_readwrite("kind", &TypePublicist::m_kind);
    type_cls.def_property_readonly("type_system", &Type::get_type_system);

    // _TypeSystem Definitions
    type_system_cls.def("__init__", [](PyTypeSystem &self, IHashCalculator *hash_calculator)
    {
        new (&self) PyTypeSystem { std::unique_ptr<IHashCalculator>(hash_calculator) };
    });
    type_system_cls.def_property_readonly("hash_calculator", &TypeSystem::get_hash_calculator);
    type_system_cls.def("has_type",
        static_cast<bool (TypeSystem::*)(const std::string &) const>(&TypeSystem::has_type), "name"_a);
    type_system_cls.def("__contains__",
        static_cast<bool (TypeSystem::*)(const std::string &) const>(&TypeSystem::has_type));
    type_system_cls.def("has_type",
        static_cast<bool (TypeSystem::*)(hash_t) const>(&TypeSystem::has_type), "hash"_a);
    type_system_cls.def("__contains__",
        static_cast<bool (TypeSystem::*)(hash_t) const>(&TypeSystem::has_type));
    type_system_cls.def("get_type",
        static_cast<const Type &(TypeSystem::*)(const std::string &) const>(&TypeSystem::get_type),
        py::return_value_policy::reference_internal, "name"_a);
    type_system_cls.def("__getitem__",
        static_cast<const Type &(TypeSystem::*)(const std::string &) const>(&TypeSystem::get_type),
        py::return_value_policy::reference_internal);
    type_system_cls.def("get_type",
        static_cast<const Type &(TypeSystem::*)(hash_t) const>(&TypeSystem::get_type),
        "hash"_a, py::return_value_policy::reference_internal);
    type_system_cls.def("__getitem__",
        static_cast<const Type &(TypeSystem::*)(hash_t) const>(&TypeSystem::get_type),
        py::return_value_policy::reference_internal);
    type_system_cls.def("define_class", [](PyTypeSystem &self, const std::string &name, const py::object &cls, const Type *base_class)
    {
        return self.define_class(name, cls, base_class);
    }, "name"_a, "cls"_a, "base_class"_a);
    type_system_cls.def("instantiate", [](PyTypeSystem &self, const std::string &name)
    {
        // Instantiate the PropertyClass, and cast it to its Python representation.
        const auto &type = self.get_type(name);
        py::object instance = py::cast(type.instantiate().release());

        // This object was just instantiated, and as such now has a
        // reference count >= 2; decrement the reference count to avoid
        // memory leaks.
        instance.dec_ref();

        return instance;
    }, py::return_value_policy::take_ownership, "name"_a);
    type_system_cls.def("instantiate", [](PyTypeSystem &self, hash_t hash)
    {
        // Instantiate the PropertyClass, and cast it to its Python representation.
        const auto &type = self.get_type(hash);
        py::object instance = py::cast(type.instantiate().release());

        // This object was just instantiated, and as such now has a
        // reference count >= 2; decrement the reference count to avoid
        // memory leaks.
        instance.dec_ref();

        return instance;
    }, py::return_value_policy::take_ownership, "hash"_a);

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
            const auto *object = instance.cast<const PropertyClass *>();
            const auto &properties = object->get_properties();
            const auto &prop = properties.get_property(self.get_name());

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
            auto *object = instance.cast<PropertyClass *>();
            auto &properties = object->get_properties();
            auto &prop = properties.get_property(self.get_name());

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
            const auto *object = instance.cast<const PropertyClass *>();
            const auto &properties = object->get_properties();
            const auto &prop = properties.get_property(self.get_name());

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
            auto *object = instance.cast<PropertyClass *>();
            PropertyList &properties = object->get_properties();
            IProperty &prop = properties.get_property(self.get_name());

            // Cast the property to a PyVectorProperty.
            auto &vector_prop = dynamic_cast<PyVectorProperty &>(prop);
            vector_prop.set(value);
        }
    });

    // ArrayPropertyDef Definitions
    array_property_def_cls.def(py::init<const std::string &, const std::string &, std::size_t, bool>(),
        "name"_a, "type_name"_a, "size"_a,  "is_pointer"_a = false);
    array_property_def_cls.def("__get__", [](const ArrayPropertyDef &self, const py::object &instance, const py::object &owner)
    {
        // Are we attempting to read the value of this property from a
        // PropertyClass instance?
        if (py::isinstance<PropertyClass>(instance))
        {
            // Yep. Read the value of the property.
            const auto *object = instance.cast<const PropertyClass *>();
            const auto &properties = object->get_properties();
            const auto &prop = properties.get_property(self.get_name());

            // Cast the property to a PyArrayProperty.
            const auto &array_prop = dynamic_cast<const PyArrayProperty &>(prop);
            return py::object(array_prop.get());
        }

        // Nope. Simply return this PropertyDef instance.
        return py::cast(self);
    }, py::return_value_policy::reference_internal);
    array_property_def_cls.def("__set__", [](const ArrayPropertyDef &self, py::object &instance, py::object &value)
    {
        // Are we attempting to set the value of this property on a
        // PropertyClass instance?
        if (py::isinstance<PropertyClass>(instance))
        {
            // Yep. Set the value of the property.
            auto *object = instance.cast<PropertyClass *>();
            PropertyList &properties = object->get_properties();
            IProperty &prop = properties.get_property(self.get_name());

            // Cast the property to a PyArrayProperty.
            auto &array_prop = dynamic_cast<PyArrayProperty &>(prop);
            array_prop.set(value);
        }
    });

    // PropertyClass Definitions
    property_class_cls.def(py::init<const Type &, const TypeSystem &>(),
        py::keep_alive<1, 2>(), py::keep_alive<1, 3>(),
        "type"_a, "type_system"_a);
    property_class_cls.def_property_readonly("type", &PropertyClass::get_type,
        py::return_value_policy::reference);
}
