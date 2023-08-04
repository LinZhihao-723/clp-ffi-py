#include <clp_ffi_py/Python.hpp>  // Must always be included before any other header files

#include "decoding_methods.hpp"

#include <clp/components/core/src/ffi/ir_stream/decoding_methods.hpp>
#include <clp/components/core/src/type_utils.hpp>
#include <clp/components/core/submodules/json/single_include/nlohmann/json.hpp>
#include <gsl/span>

#include <clp_ffi_py/error_messages.hpp>
#include <clp_ffi_py/ir/error_messages.hpp>
#include <clp_ffi_py/ir/PyDecoderBuffer.hpp>
#include <clp_ffi_py/ir/PyLogEvent.hpp>
#include <clp_ffi_py/ir/PyMetadata.hpp>
#include <clp_ffi_py/ir/PyQuery.hpp>
#include <clp_ffi_py/PyObjectCast.hpp>
#include <clp_ffi_py/PyObjectUtils.hpp>
#include <clp_ffi_py/utils.hpp>

namespace clp_ffi_py::ir {
namespace {
/**
 * Attempts to populate the decoder buffer. When this function is called, it is
 * expected to have more bytes to read from the IR stream.
 * @param decoder_buffer Input decoder buffer.
 * @return true on success.
 * @return false on failure with the relevant Python exception and error set.
 */
auto try_read_more(PyDecoderBuffer* decoder_buffer) -> bool {
    Py_ssize_t num_bytes_read;
    if (false == decoder_buffer->populate_read_buffer(num_bytes_read)) {
        return false;
    }
    if (0 == num_bytes_read) {
        PyErr_SetString(PyExc_RuntimeError, cDecoderIncompleteIRError);
        return false;
    }
    return true;
}

/**
 * Decodes the next log event that matches the query (if given) from the input
 * DecoderBuffer using CLP decoding methods.
 * @param decoder_buffer IR decoder buffer of the input IR stream.
 * @param py_metadata The metadata associated with the input IR stream.
 * @param py_query Search query to filter log events.
 * @return Log event represented as PyLogEvent on success.
 * @return PyNone on termination.
 * @return nullptr on failure with the relevant Python exception and error set.
 */
auto decode(PyDecoderBuffer* decoder_buffer, PyMetadata* py_metadata, PyQuery* py_query)
        -> PyObject* {
    std::string decoded_message;
    ffi::epoch_time_ms_t timestamp_delta{0};
    auto timestamp{decoder_buffer->get_ref_timestamp()};
    size_t current_log_event_idx{0};
    while (true) {
        auto const unconsumed_bytes{decoder_buffer->get_unconsumed_bytes()};
        ffi::ir_stream::IrBuffer ir_buffer{unconsumed_bytes.data(), unconsumed_bytes.size()};
        auto const err{ffi::ir_stream::four_byte_encoding::decode_next_message(
                ir_buffer,
                decoded_message,
                timestamp_delta
        )};
        switch (err) {
            case ffi::ir_stream::IRErrorCode_Success:
                timestamp += timestamp_delta;
                current_log_event_idx = decoder_buffer->get_and_increment_decoded_message_count();
                decoder_buffer->commit_read_buffer_consumption(ir_buffer.get_cursor_pos());

                if (nullptr != py_query) {
                    auto* query{py_query->get_query()};
                    if (query->ts_safely_outside_time_range(timestamp)) {
                        Py_RETURN_NONE;
                    }
                    if (false == query->matches_time_range(timestamp)
                        || false == query->matches_wildcard_queries(decoded_message))
                    {
                        continue;
                    }
                }

                decoder_buffer->set_ref_timestamp(timestamp);
                return py_reinterpret_cast<PyObject>(PyLogEvent::create_new_log_event(
                        decoded_message,
                        timestamp,
                        current_log_event_idx,
                        py_metadata
                ));
            case ffi::ir_stream::IRErrorCode_Incomplete_IR:
                if (false == try_read_more(decoder_buffer)) {
                    return nullptr;
                }
                break;
            case ffi::ir_stream::IRErrorCode_Eof:
                Py_RETURN_NONE;
            default:
                PyErr_Format(PyExc_RuntimeError, cDecoderErrorCodeTemplate, err);
                return nullptr;
        }
    }
    return nullptr;
}
}  // namespace

extern "C" {
auto decode_preamble(PyObject* self, PyObject* py_decoder_buffer) -> PyObject* {
    if (false
        == static_cast<bool>(PyObject_TypeCheck(py_decoder_buffer, PyDecoderBuffer::get_py_type())))
    {
        PyErr_SetString(PyExc_TypeError, cPyTypeError);
        return nullptr;
    }

    bool success{false};
    auto* decoder_buffer{py_reinterpret_cast<PyDecoderBuffer>(py_decoder_buffer)};
    bool is_four_byte_encoding{false};
    while (false == success) {
        auto const unconsumed_bytes{decoder_buffer->get_unconsumed_bytes()};
        ffi::ir_stream::IrBuffer ir_buffer{unconsumed_bytes.data(), unconsumed_bytes.size()};
        auto const err{ffi::ir_stream::get_encoding_type(ir_buffer, is_four_byte_encoding)};
        switch (err) {
            case ffi::ir_stream::IRErrorCode_Success:
                decoder_buffer->commit_read_buffer_consumption(ir_buffer.get_cursor_pos());
                success = true;
                break;
            case ffi::ir_stream::IRErrorCode_Incomplete_IR:
                if (false == try_read_more(decoder_buffer)) {
                    return nullptr;
                }
                break;
            default:
                PyErr_Format(PyExc_RuntimeError, cDecoderErrorCodeTemplate, err);
                return nullptr;
        }
    }

    if (false == is_four_byte_encoding) {
        PyErr_SetString(PyExc_NotImplementedError, "8-byte IR decoding is not supported yet.");
        return nullptr;
    }

    ffi::ir_stream::encoded_tag_t metadata_type_tag{0};
    size_t metadata_pos{0};
    uint16_t metadata_size{0};
    gsl::span<int8_t> metadata_buffer;
    success = false;
    while (false == success) {
        auto const unconsumed_bytes{decoder_buffer->get_unconsumed_bytes()};
        ffi::ir_stream::IrBuffer ir_buffer{unconsumed_bytes.data(), unconsumed_bytes.size()};
        auto const err{ffi::ir_stream::decode_preamble(
                ir_buffer,
                metadata_type_tag,
                metadata_pos,
                metadata_size
        )};
        switch (err) {
            case ffi::ir_stream::IRErrorCode_Success:
                metadata_buffer = unconsumed_bytes.subspan(
                        metadata_pos,
                        static_cast<size_t>(metadata_size)
                );
                decoder_buffer->commit_read_buffer_consumption(ir_buffer.get_cursor_pos());
                success = true;
                break;
            case ffi::ir_stream::IRErrorCode_Incomplete_IR:
                if (false == try_read_more(decoder_buffer)) {
                    return nullptr;
                }
                break;
            default:
                PyErr_Format(PyExc_RuntimeError, cDecoderErrorCodeTemplate, err);
                return nullptr;
        }
    }

    PyMetadata* metadata{nullptr};
    try {
        // Initialization list should not be used in this case:
        // https://github.com/nlohmann/json/discussions/4096
        nlohmann::json metadata_json(
                std::move(nlohmann::json::parse(metadata_buffer.begin(), metadata_buffer.end()))
        );
        metadata = PyMetadata::create_new_from_json(metadata_json, is_four_byte_encoding);
        if (nullptr != metadata) {
            decoder_buffer->set_ref_timestamp(metadata->get_metadata()->get_ref_timestamp());
        }
    } catch (nlohmann::json::exception& ex) {
        PyErr_Format(PyExc_RuntimeError, "Json Parsing Error: %s", ex.what());
        return nullptr;
    }
    return py_reinterpret_cast<PyObject>(metadata);
}

auto decode_next_log_event(PyObject* self, PyObject* args, PyObject* keywords) -> PyObject* {
    static char keyword_decoder_buffer[]{"decoder_buffer"};
    static char keyword_metadata[]{"metadata"};
    static char keyword_query[]{"query"};
    static char* keyword_table[]{
            static_cast<char*>(keyword_decoder_buffer),
            static_cast<char*>(keyword_metadata),
            static_cast<char*>(keyword_query),
            nullptr};

    PyObject* decoder_buffer{nullptr};
    PyObject* metadata{nullptr};
    PyObject* query{Py_None};

    if (false
        == static_cast<bool>(PyArg_ParseTupleAndKeywords(
                args,
                keywords,
                "O!O!|O",
                static_cast<char**>(keyword_table),
                PyDecoderBuffer::get_py_type(),
                &decoder_buffer,
                PyMetadata::get_py_type(),
                &metadata,
                &query
        )))
    {
        return nullptr;
    }

    bool const is_query_given{Py_None != query};
    if (is_query_given
        && false == static_cast<bool>(PyObject_TypeCheck(query, PyQuery::get_py_type())))
    {
        PyErr_SetString(PyExc_TypeError, cPyTypeError);
        return nullptr;
    }

    return decode(
            py_reinterpret_cast<PyDecoderBuffer>(decoder_buffer),
            py_reinterpret_cast<PyMetadata>(metadata),
            is_query_given ? py_reinterpret_cast<PyQuery>(query) : nullptr
    );
}
}
}  // namespace clp_ffi_py::ir
