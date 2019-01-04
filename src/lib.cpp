#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_util(py::module &m);
void bind_dml(py::module &m);
void bind_protocol(py::module &m);
void bind_pclass(py::module &m);
void bind_serialization(py::module &m);

PYBIND11_MODULE(lib, m)
{
    auto util_submodule = m.def_submodule("util");
    auto dml_submodule = m.def_submodule("dml");
    auto protocol_submodule = m.def_submodule("protocol");
    auto pclass_submodule = m.def_submodule("pclass");
    auto serialization_submodule = m.def_submodule("serialization");

    bind_util(util_submodule);
    bind_dml(dml_submodule);
    bind_protocol(protocol_submodule);
    bind_pclass(pclass_submodule);
    bind_serialization(serialization_submodule);
}
