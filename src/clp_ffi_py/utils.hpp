#ifndef CLP_FFI_PY_UTILS_HPP
#define CLP_FFI_PY_UTILS_HPP

#include <clp_ffi_py/Python.hpp>  // Must always be included before any other header files

#include <iostream>
#include <string>
#include <vector>

namespace clp_ffi_py {
/**
 * Adds the given Python type to the given Python module.
 * @param new_type Type object to add.
 * @param type_name Type name identifier.
 * @param module Python module to hold the new type.
 * @return true on success.
 * @return false on failure with the relevant Python exception and error set.
 */
auto add_type(PyTypeObject* new_type, char const* type_name, PyObject* module) -> bool;

/**
 * Parses a Python string into std::string.
 * @param py_string PyObject that represents a Python level string. Only Python
 * Unicode object or an instance of a Python Unicode subtype will be considered
 * as valid input.
 * @param out The string parsed.
 * @return true on success.
 * @return false on failure with the relevant Python exception and error set.
 */
auto parse_PyString(PyObject* py_string, std::string& out) -> bool;

/**
 * Parses a Python string into std::string_view.
 * @param py_string PyObject that represents a Python level string. Only Python
 * Unicode object or an instance of a Python Unicode subtype will be considered
 * as valid input.
 * @param view The string_view of the underlying byte data of py_string.
 * @return true on success.
 * @return false on failure with the relevant Python exception and error set.
 */
auto parse_PyString_as_string_view(PyObject* py_string, std::string_view& view) -> bool;

/**
 * Parses a Python integer into an int_type variable.
 * @param py_int PyObject that represents a Python level integer. Only
 * PyLongObject or an instance of a subtype of PyLongObject will be considered
 * as valid input.
 * @param int_type The string_view of the underlying byte data of py_string.
 * @return true on success.
 * @return false on failure with the relevant Python exception and error set.
 */

/**
 * Parses a Python integer into an int_type variable.
 * @tparam int_type Output integer type (size and signed/unsigned).
 * @param py_int PyObject that represents a Python level integer. Only
 * PyLongObject or an instance of a subtype of PyLongObject will be considered
 * as valid input.
 * @param val The integer parsed.
 * @return true on success.
 * @return false on failure with the relevant Python exception and error set.
 */
template <typename int_type>
auto parse_PyInt(PyObject* py_int, int_type& val) -> bool;
};  // namespace clp_ffi_py

#include <clp_ffi_py/utils.tpp>
#endif
