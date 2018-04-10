#include <string>
#include <iostream>

#include <pybind11/pybind11.h>

#include <ki/dml/exception.h>
#include <ki/dml/Record.h>
#include <ki/dml/FieldBase.h>
#include <ki/dml/Field.h>

#define DEF_FIELD_CLASS(NAME, TYPE)                                             \
    py::class_<Field<TYPE>>(m, NAME)                                            \
        .def(py::init<std::string>())                                           \
        .def_property_readonly("name",                                          \
            &Field<TYPE>::get_name,                                             \
            py::return_value_policy::copy)                                      \
        .def_property_readonly("transferable",                                  \
            &Field<TYPE>::is_transferable,                                      \
            py::return_value_policy::copy)                                      \
        .def_property("value",                                                  \
            &Field<TYPE>::get_value,                                            \
            static_cast<void (Field<TYPE>::*)(TYPE)>(&Field<TYPE>::set_value),  \
            py::return_value_policy::copy)                                      \
        .def_property_readonly("type_name",                                     \
            &Field<TYPE>::get_type_name,                                        \
            py::return_value_policy::copy)                                      \
        .def_property_readonly("size",                                          \
            &Field<TYPE>::get_size,                                             \
            py::return_value_policy::copy)

#define DEF_HAS_FIELD_METHOD(NAME, TYPE)            \
    .def(NAME,                                      \
        &Record::has_field<TYPE>,                   \
        py::arg("name"),                            \
        py::return_value_policy::copy)
#define DEF_GET_FIELD_METHOD(NAME, TYPE)     \
    .def(NAME,                               \
        &Record::get_field<TYPE>,            \
        py::arg("name"),                     \
        py::return_value_policy::reference)
#define DEF_ADD_FIELD_METHOD(NAME, TYPE)     \
    .def(NAME,                               \
        &Record::add_field<TYPE>,            \
        py::arg("name"),                     \
        py::arg("transferable") = true,      \
        py::return_value_policy::reference)

namespace py = pybind11;

PYBIND11_MODULE(dml, m)
{
    using namespace ki::dml;

    // Exceptions
    py::register_exception<runtime_error>(m, "DMLRuntimeError");
    py::register_exception<parse_error>(m, "DMLParseError");
    py::register_exception<value_error>(m, "DMLValueError");

    // Record Class
    py::class_<Record>(m, "Record")
        .def(py::init<>())
        // Descriptors
        .def("__getitem__",
            [](const Record &self, std::string key)
            {
                auto *field = self.get_field(key);
                if (field)
                    return field;
                throw py::key_error("Field '" + key + "' does not exist");
            },
            py::arg("key"),
            py::return_value_policy::reference)
        .def("__iter__",
            [](const Record &self)
            {
                return py::make_iterator(self.fields_begin(), self.fields_end());
            },
            py::keep_alive<0, 1>())
        .def("__contains__",
            [](const Record &self, std::string key)
            {
                return self.has_field(key);
            },
            py::arg("key"),
            py::return_value_policy::copy)
        // Read-only Properties
        .def_property_readonly("field_count",
            &Record::get_field_count,
            py::return_value_policy::copy)
        .def_property_readonly("size",
            &Record::get_size,
            py::return_value_policy::copy)
        // Has (Field)
        DEF_HAS_FIELD_METHOD("has_byt_field", BYT)
        DEF_HAS_FIELD_METHOD("has_ubyt_field", UBYT)
        DEF_HAS_FIELD_METHOD("has_shrt_field", SHRT)
        DEF_HAS_FIELD_METHOD("has_ushrt_field", USHRT)
        DEF_HAS_FIELD_METHOD("has_int_field", INT)
        DEF_HAS_FIELD_METHOD("has_uint_field", UINT)
        DEF_HAS_FIELD_METHOD("has_str_field", STR)
        DEF_HAS_FIELD_METHOD("has_wstr_field", WSTR)
        DEF_HAS_FIELD_METHOD("has_flt_field", FLT)
        DEF_HAS_FIELD_METHOD("has_dbl_field", DBL)
        DEF_HAS_FIELD_METHOD("has_gid_field", GID)
        // Get (Field)
        DEF_GET_FIELD_METHOD("get_byt_field", BYT)
        DEF_GET_FIELD_METHOD("get_ubyt_field", UBYT)
        DEF_GET_FIELD_METHOD("get_shrt_field", SHRT)
        DEF_GET_FIELD_METHOD("get_ushrt_field", USHRT)
        DEF_GET_FIELD_METHOD("get_int_field", INT)
        DEF_GET_FIELD_METHOD("get_uint_field", UINT)
        DEF_GET_FIELD_METHOD("get_str_field", STR)
        DEF_GET_FIELD_METHOD("get_wstr_field", WSTR)
        DEF_GET_FIELD_METHOD("get_flt_field", FLT)
        DEF_GET_FIELD_METHOD("get_dbl_field", DBL)
        DEF_GET_FIELD_METHOD("get_gid_field", GID)
        // Add (Field)
        DEF_ADD_FIELD_METHOD("add_byt_field", BYT)
        DEF_ADD_FIELD_METHOD("add_ubyt_field", UBYT)
        DEF_ADD_FIELD_METHOD("add_shrt_field", SHRT)
        DEF_ADD_FIELD_METHOD("add_ushrt_field", USHRT)
        DEF_ADD_FIELD_METHOD("add_int_field", INT)
        DEF_ADD_FIELD_METHOD("add_uint_field", UINT)
        DEF_ADD_FIELD_METHOD("add_str_field", STR)
        DEF_ADD_FIELD_METHOD("add_wstr_field", WSTR)
        DEF_ADD_FIELD_METHOD("add_flt_field", FLT)
        DEF_ADD_FIELD_METHOD("add_dbl_field", DBL)
        DEF_ADD_FIELD_METHOD("add_gid_field", GID)
        // Extensions
        .def("to_bytes",
            [](const Record &self)
            {
                std::ostringstream oss;
                self.write_to(oss);
                return py::bytes(oss.str());
            },
            py::return_value_policy::copy)
        .def("from_bytes",
            [](Record &self, std::string data)
            {
                std::istringstream iss(data);
                self.read_from(iss);
            },
            py::arg("data"));

    // Field Classes
    DEF_FIELD_CLASS("BytField", BYT);
    DEF_FIELD_CLASS("UBytField", UBYT);
    DEF_FIELD_CLASS("ShrtField", SHRT);
    DEF_FIELD_CLASS("UShrtField", USHRT);
    DEF_FIELD_CLASS("IntField", INT);
    DEF_FIELD_CLASS("UIntField", UINT);
    DEF_FIELD_CLASS("StrField", STR);
    DEF_FIELD_CLASS("WStrField", WSTR);
    DEF_FIELD_CLASS("FltField", FLT);
    DEF_FIELD_CLASS("DblField", DBL);
    DEF_FIELD_CLASS("GidField", GID);
}
