set(
        CLP_FFI_PY_WRAPPED_FACADE_HEADER_LIST
        "wrapped_facade_headers/msgpack.hpp"
        "wrapped_facade_headers/Python.hpp"
)
add_library(wrapped_facade_headers INTERFACE)
add_library(clp_ffi_py::wrapped_facade_headers ALIAS wrapped_facade_headers)
target_sources(wrapped_facade_headers INTERFACE ${CLP_FFI_PY_WRAPPED_FACADE_HEADERS_LIST})
target_include_directories(wrapped_facade_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(wrapped_facade_headers INTERFACE cxx_std_20)