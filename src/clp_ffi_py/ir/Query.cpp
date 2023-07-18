#include "Query.hpp"

#include <clp/components/core/src/string_utils.hpp>

namespace clp_ffi_py::ir {
auto Query::wildcard_matches(std::string_view log_message)
        -> std::pair<bool, std::optional<std::string_view>> {
    if (m_wildcard_list.empty()) {
        return {true, std::nullopt};
    }
    for (auto const& wildcard_query : m_wildcard_list) {
        if (wildcard_match_unsafe(log_message, wildcard_query, m_case_sensitive)) {
            return {true, wildcard_query};
        }   
    }
    return {false, std::nullopt};
}
}  // namespace clp_ffi_py::ir
