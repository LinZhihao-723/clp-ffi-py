#ifndef CLP_PY_PYMESSAGE
#define CLP_PY_PYMESSAGE

#include <Python.h>

#include "Message.hpp"
#include "utilities.hpp"

namespace clp_ffi_py::components {
extern PyType_Spec PyMessageTy;

struct PyMessage {
    PyObject_HEAD;
    Message* message;
};

PyObject* PyMessage_new (PyTypeObject* type, PyObject* args, PyObject* kwds);
void PyMessage_dealloc (PyMessage* self);
PyObject* PyMessage_get_message (PyMessage* self);
PyObject* PyMessage_get_timestamp (PyMessage* self);
PyObject* PyMessage_wildcard_match (PyMessage* self, PyObject* args);
PyObject* PyMessage_wildcard_match_case_sensitive (PyMessage* self, PyObject* args);


constexpr char PyMessage_create_empty_key[] = "_PyMessage_create_empty";
PyMessage* PyMessage_create_empty ();
using PyMessageCreateFuncType = decltype(&PyMessage_create_empty);
} // namespace clp_ffi_py::components

#endif
