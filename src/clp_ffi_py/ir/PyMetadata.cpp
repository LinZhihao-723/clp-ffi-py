#include <clp_ffi_py/Python.hpp>

#include <clp_ffi_py/ErrorMessage.hpp>
#include <clp_ffi_py/ExceptionFFI.hpp>
#include <clp_ffi_py/ir/Metadata.hpp>
#include <clp_ffi_py/ir/PyMetadata.hpp>
#include <clp_ffi_py/Py_utils.hpp>
#include <clp_ffi_py/PyObjectPtr.hpp>
#include <clp_ffi_py/utils.hpp>

namespace clp_ffi_py::ir {
namespace {
extern "C" {
/**
 * Callback of PyMetadata `__init__` method:
 * __init__(self, ref_timestamp, timestamp_format, timezone_id)
 * Keyword argument parsing is supported.
 * Note: double initialization will result in memory leaks.
 * @param self
 * @param args
 * @param keywords
 * @return 0 on success.
 * @return -1 on failure with the relevant Python exception and error set.
 */
auto PyMetadata_init(PyMetadata* self, PyObject* args, PyObject* keywords) -> int {
    static char keyword_ref_timestamp[]{"ref_timestamp"};
    static char keyword_timestamp_format[]{"timestamp_format"};
    static char keyword_timezone_id[]{"timezone_id"};
    static char* keyword_table[]{
            static_cast<char*>(keyword_ref_timestamp),
            static_cast<char*>(keyword_timestamp_format),
            static_cast<char*>(keyword_timezone_id),
            nullptr};

    ffi::epoch_time_ms_t ref_timestamp{0};
    char const* input_timestamp_format{nullptr};
    char const* input_timezone{nullptr};

    // If the argument parsing fails, `self` will be deallocated. We must reset
    // all pointers to nullptr in advance, otherwise the deallocator might
    // trigger segmentation fault
    self->reset();

    if (false
        == static_cast<bool>(PyArg_ParseTupleAndKeywords(
                args,
                keywords,
                "Lss",
                static_cast<char**>(keyword_table),
                &ref_timestamp,
                &input_timestamp_format,
                &input_timezone
        )))
    {
        return -1;
    }

    auto success{self->init(ref_timestamp, input_timestamp_format, input_timezone)};
    return success ? 0 : -1;
}

/**
 * Callback of PyMetadata deallocator.
 * @param self
 */
auto PyMetadata_dealloc(PyMetadata* self) -> void {
    self->clean();
    PyObject_Del(self);
}

PyDoc_STRVAR(
        cPyMetadataIsUsingFourByteEncodingDoc,
        "is_using_four_byte_encoding(self)\n"
        "--\n\n"
        "Checks whether the CLP IR is encoded using 4-byte or 8-byte encoding methods.\n"
        ":param self\n"
        ":return: True for 4-byte encoding, and False for 8-byte encoding.\n"
);

auto PyMetadata_is_using_four_byte_encoding(PyMetadata* self) -> PyObject* {
    if (self->get_metadata()->is_using_four_byte_encoding()) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(
        cPyMetadataGetRefTimestampDoc,
        "get_ref_timestamp(self)\n"
        "--\n\n"
        "Gets the reference Unix epoch timestamp in milliseconds used to calculate the timestamp "
        "of the first log message in the IR stream.\n"
        ":param self\n"
        ":return: The reference timestamp.\n"
);

auto PyMetadata_get_ref_timestamp(PyMetadata* self) -> PyObject* {
    return PyLong_FromLongLong(self->get_metadata()->get_ref_timestamp());
}

PyDoc_STRVAR(
        cPyMetadataGetTimestampFormatDoc,
        "get_timestamp_format(self)\n"
        "--\n\n"
        "Gets the timestamp format to be use when generating the logs with a reader.\n"
        ":param self\n"
        ":return: The timestamp format.\n"
);

auto PyMetadata_get_timestamp_format(PyMetadata* self) -> PyObject* {
    return PyUnicode_FromString(self->get_metadata()->get_timestamp_format().c_str());
}

PyDoc_STRVAR(
        cPyMetadataGetTimezoneIdDoc,
        "get_timezone_id(self)\n"
        "--\n\n"
        "Gets the timezone id to be use when generating the timestamp from Unix epoch time.\n"
        ":param self\n"
        ":return: The timezone ID in TZID format.\n"
);

auto PyMetadata_get_timezone_id(PyMetadata* self) -> PyObject* {
    return PyUnicode_FromString(self->get_metadata()->get_timezone_id().c_str());
}

PyDoc_STRVAR(
        cPyMetadataGetTimezoneDoc,
        "get_timezone(self)\n"
        "--\n\n"
        "Gets the timezone represented as tzinfo to be use when generating the timestamp from Unix "
        "epoch time.\n"
        ":param self\n"
        ":return: A new reference to the timezone as tzinfo.\n"
);

auto PyMetadata_get_timezone(PyMetadata* self) -> PyObject* {
    auto* timezone{self->get_py_timezone()};
    if (nullptr == timezone) {
        PyErr_SetString(
                PyExc_RuntimeError,
                clp_ffi_py::error_messages::cTimezoneObjectNotInitialzed
        );
        return nullptr;
    }
    Py_INCREF(timezone);
    return timezone;
}
}

/**
 * PyMetadata method table.
 */
PyMethodDef PyMetadata_method_table[]{
        {"is_using_four_byte_encoding",
         reinterpret_cast<PyCFunction>(PyMetadata_is_using_four_byte_encoding),
         METH_NOARGS,
         static_cast<char const*>(cPyMetadataIsUsingFourByteEncodingDoc)},

        {"get_ref_timestamp",
         reinterpret_cast<PyCFunction>(PyMetadata_get_ref_timestamp),
         METH_NOARGS,
         static_cast<char const*>(cPyMetadataGetRefTimestampDoc)},

        {"get_timestamp_format",
         reinterpret_cast<PyCFunction>(PyMetadata_get_timestamp_format),
         METH_NOARGS,
         static_cast<char const*>(cPyMetadataGetTimestampFormatDoc)},

        {"get_timezone_id",
         reinterpret_cast<PyCFunction>(PyMetadata_get_timezone_id),
         METH_NOARGS,
         static_cast<char const*>(cPyMetadataGetTimezoneIdDoc)},

        {"get_timezone",
         reinterpret_cast<PyCFunction>(PyMetadata_get_timezone),
         METH_NOARGS,
         static_cast<char const*>(cPyMetadataGetTimezoneDoc)},

        {nullptr}};

/**
 * PyMetadata class doc string.
 */
PyDoc_STRVAR(
        cPyMetadataDoc,
        "This class represents the IR stream preamble and provides ways to access the underlying "
        "metadata. Normally, this class will be instantiated by the FFI IR decoding methods.\n"
        "However, with `__init__` method provided below, direct instantiation is also possible.\n\n"
        "__init__(self, ref_timestamp, timestamp_format, timezone_id)\n"
        "Initialize an object that represents CLP IR metadata. Notice that each object should be "
        "strictly initialized only once. Double initialization will result in memory leaks.\n"
        ":param self\n"
        ":param ref_timestamp: the reference Unix epoch timestamp in milliseconds used to "
        "calculate the timestamp of the first log message in the IR stream.\n"
        ":param timestamp_format: the timestamp format to be use when generating the logs with a "
        "reader.\n"
        ":param timezone_id: the timezone id to be use when generating the timestamp from Unix "
        "epoch time.\n"
        ":return: 0 on success, -1 on failure with relevant Python exceptions and error set.\n"
);

/**
 * PyMetadata Python type slots.
 */
PyType_Slot PyMetadata_slots[]{
        {Py_tp_alloc, reinterpret_cast<void*>(PyType_GenericAlloc)},
        {Py_tp_dealloc, reinterpret_cast<void*>(PyMetadata_dealloc)},
        {Py_tp_init, reinterpret_cast<void*>(PyMetadata_init)},
        {Py_tp_new, reinterpret_cast<void*>(PyType_GenericNew)},
        {Py_tp_methods, static_cast<void*>(PyMetadata_method_table)},
        {Py_tp_doc, const_cast<void*>(static_cast<void const*>(cPyMetadataDoc))},
        {0, nullptr}};

/**
 * PyMetadata Python type specifications.
 */
PyType_Spec PyMetadata_type_spec{
        "clp_ffi_py.CLPIR.Metadata",
        sizeof(PyMetadata),
        0,
        Py_TPFLAGS_DEFAULT,
        static_cast<PyType_Slot*>(PyMetadata_slots)};

/**
 * PyMetadata's Python type.
 */
PyObjectPtr<PyTypeObject> PyMetadata_type;
}  // namespace

auto PyMetadata::init(
        ffi::epoch_time_ms_t ref_timestamp,
        char const* input_timestamp_format,
        char const* input_timezone
) -> bool {
    this->m_metadata = new Metadata(ref_timestamp, input_timestamp_format, input_timezone);
    if (nullptr == this->m_metadata) {
        PyErr_SetString(PyExc_RuntimeError, clp_ffi_py::error_messages::cOutofMemoryError);
        return false;
    }
    return this->init_py_timezone();
}

auto PyMetadata::init(nlohmann::json const& metadata, bool is_four_byte_encoding) -> bool {
    try {
        this->m_metadata = new Metadata(metadata, is_four_byte_encoding);
    } catch (ExceptionFFI const& ex) {
        PyErr_Format(
                PyExc_RuntimeError,
                "Failed to initialize metadata from decoded JSON format preamble. "
                "Error message: %s",
                ex.what()
        );
        this->m_metadata = nullptr;
        return false;
    }
    if (nullptr == this->m_metadata) {
        PyErr_SetString(PyExc_RuntimeError, clp_ffi_py::error_messages::cOutofMemoryError);
        return false;
    }
    return this->init_py_timezone();
}

auto PyMetadata::init_py_timezone() -> bool {
    assert(this->m_metadata);
    this->m_py_timezone
            = Py_utils_get_timezone_from_timezone_id(this->m_metadata->get_timezone_id());
    if (nullptr == this->m_py_timezone) {
        return false;
    }
    Py_INCREF(this->m_py_timezone);
    return true;
}

auto PyMetadata_get_PyType() -> PyTypeObject* {
    return PyMetadata_type.get();
}

auto PyMetadata_module_level_init(PyObject* py_module) -> bool {
    auto* type{reinterpret_cast<PyTypeObject*>(PyType_FromSpec(&PyMetadata_type_spec))};
    PyMetadata_type.reset(type);
    if (nullptr == type) {
        return false;
    }
    return add_type(PyMetadata_get_PyType(), "Metadata", py_module);
}

auto PyMetadata_init_from_json(nlohmann::json const& metadata, bool is_four_byte_encoding)
        -> PyMetadata* {
    PyMetadata* self{
            reinterpret_cast<PyMetadata*>(PyObject_New(PyMetadata, PyMetadata_get_PyType()))};
    if (nullptr == self) {
        return nullptr;
    }
    self->reset();
    if (false == self->init(metadata, is_four_byte_encoding)) {
        Py_DECREF(self);
        return nullptr;
    }
    return self;
}
}  // namespace clp_ffi_py::ir
