// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/UnitLabel.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_value_policy.hpp>

#include <boost/scoped_array.hpp>
#include <memory>

// For Python unicode object
#include <unicodeobject.h>

using Mantid::Kernel::UnitLabel;
using namespace boost::python;

namespace {
std::shared_ptr<UnitLabel> createLabel(const object &ascii, const object &utf8, const object &latex) {
  using Utf8Char = UnitLabel::Utf8String::value_type;
  if (PyUnicode_Check(utf8.ptr())) {
#if PY_MAJOR_VERSION >= 3
    auto length = PyUnicode_GetLength(utf8.ptr());
    boost::scoped_array<Utf8Char> buffer(new Utf8Char[length]);
    PyUnicode_AsWideChar(utf8.ptr(), buffer.get(), length);
#else
    auto length = PyUnicode_GetSize(utf8.ptr());
    boost::scoped_array<Utf8Char> buffer(new Utf8Char[length]);
    PyUnicode_AsWideChar(reinterpret_cast<PyUnicodeObject *>(utf8.ptr()), buffer.get(), length);
#endif
    return std::make_shared<UnitLabel>(extract<std::string>(ascii)(),
                                       UnitLabel::Utf8String(buffer.get(), buffer.get() + length),
                                       extract<std::string>(latex)());
  } else {
    throw std::invalid_argument("utf8 label is not a unicode string object. "
                                "Try prefixing the string with a 'u' character.");
  }
}

/**
 * @param self A reference to the calling object
 * @return A new Python unicode string with the contents of the utf8 label
 */
PyObject *utf8ToUnicode(const UnitLabel &self) {
  const auto &label = self.utf8();
  return PyUnicode_FromWideChar(label.c_str(), label.size());
}
} // namespace

void export_UnitLabel() {
  class_<UnitLabel>("UnitLabel", no_init)
      .def("__init__",
           make_constructor(createLabel, default_call_policies(), (arg("ascii"), arg("utf8"), arg("latex"))),
           "Construct a label from a unicode object")

      .def(init<UnitLabel::AsciiString>((arg("ascii")), "Construct a label from a plain-text string"))

      .def("ascii", &UnitLabel::ascii, arg("self"), return_value_policy<copy_const_reference>(),
           "Return the label as a plain-text string")

      .def("utf8", &utf8ToUnicode, arg("self"), "Return the label as a unicode string")

      .def("latex", &UnitLabel::latex, return_value_policy<copy_const_reference>(), arg("self"),
           "Return the label as a plain-text string with latex formatting")

      // special functions
      .def("__str__", &UnitLabel::ascii, return_value_policy<copy_const_reference>(), arg("self"))
      .def("__unicode__", &utf8ToUnicode, arg("self"));
}
