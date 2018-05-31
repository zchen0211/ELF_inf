#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include "elf/base/pybind_interface.h"
#include "test_elf.h"

namespace py = pybind11;

PYBIND11_MODULE(test_elf, m) {
  elf::register_common_func(m);

  m.def("getOptionSpec", &getOptionSpec);

  py::class_<GameContext>(m, "GameContext")
      .def(py::init<const elf::OptionMap&>())
      .def(
          "ctx",
          &GameContext::ctx,
          py::return_value_policy::reference_internal);
}
