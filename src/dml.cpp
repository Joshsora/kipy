#include <pybind11/pybind11.h>
#include <ki/dml/Record.h>
#include <ki/dml/FieldBase.h>
#include <ki/dml/Field.h>

#define DEF_FIELD_CLASS(TYPE, NAME)                                             \
    py::class_<Field<TYPE>>(m, NAME)                                            \
        .def(py::init<std::string>())                                           \
        .def_property_readonly("name", &Field<TYPE>::get_name,                  \
			py::return_value_policy::copy)                                      \
		.def_property_readonly("transferable", &Field<TYPE>::is_transferable,   \
			py::return_value_policy::copy)                                      \
		.def_property("value", &Field<TYPE>::get_value,                         \
			static_cast<void (Field<TYPE>::*)(TYPE)>(&Field<TYPE>::set_value),  \
            py::return_value_policy::copy)                                      \
        .def_property_readonly("type_name", &Field<TYPE>::get_type_name,        \
			py::return_value_policy::copy)                                      \
        .def_property_readonly("size", &Field<TYPE>::get_size,                  \
			py::return_value_policy::copy)

#define DEF_RECORD_FIELD_METHODS(PREFIX)                               \
    .def(#PREFIX ## "_byt_field", &Record::PREFIX ## _field<BYT>)      \
    .def(#PREFIX ## "_ubyt_field", &Record::PREFIX ## _field<UBYT>)    \
    .def(#PREFIX ## "_shrt_field", &Record::PREFIX ## _field<SHRT>)    \
    .def(#PREFIX ## "_ushrt_field", &Record::PREFIX ## _field<USHRT>)  \
    .def(#PREFIX ## "_int_field", &Record::PREFIX ## _field<INT>)      \
    .def(#PREFIX ## "_uint_field", &Record::PREFIX ## _field<UINT>)    \
    .def(#PREFIX ## "_str_field", &Record::PREFIX ## _field<STR>)      \
    .def(#PREFIX ## "_wstr_field", &Record::PREFIX ## _field<WSTR>)    \
    .def(#PREFIX ## "_flt_field", &Record::PREFIX ## _field<FLT>)      \
    .def(#PREFIX ## "_dbl_field", &Record::PREFIX ## _field<DBL>)      \
    .def(#PREFIX ## "_gid_field", &Record::PREFIX ## _field<GID>)

using namespace ki::dml;
namespace py = pybind11;

PYBIND11_MODULE(dml, m)
{
	// Record
    py::class_<Record>(m, "Record")
        .def(py::init<>())
		DEF_RECORD_FIELD_METHODS(has)
		DEF_RECORD_FIELD_METHODS(add)
		DEF_RECORD_FIELD_METHODS(get)
		.def_property_readonly("field_count", &Record::get_field_count,
			py::return_value_policy::copy)
        .def_property_readonly("size", &Record::get_size,
			py::return_value_policy::copy);

    // Fields
	DEF_FIELD_CLASS(BYT, "BytField");
	DEF_FIELD_CLASS(UBYT, "UBytField");
	DEF_FIELD_CLASS(SHRT, "ShrtField");
	DEF_FIELD_CLASS(USHRT, "UShrtField");
	DEF_FIELD_CLASS(INT, "IntField");
	DEF_FIELD_CLASS(UINT, "UIntField");
	DEF_FIELD_CLASS(STR, "StrField");
	DEF_FIELD_CLASS(WSTR, "WStrField");
	DEF_FIELD_CLASS(FLT, "FltField");
	DEF_FIELD_CLASS(DBL, "DblField");
	DEF_FIELD_CLASS(GID, "GidField");
}
