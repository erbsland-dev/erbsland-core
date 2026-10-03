#pragma once

#include "../../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::cryptology {

/// Revocation evidence available during an accepted certificate evaluation.
enum class X509RevocationStatus : uint8_t {
    NotChecked, ///< The portable validator does not perform revocation checks.
    Unknown,    ///< Native evaluation did not establish complete non-revocation evidence.
    Checked,    ///< Native evaluation established non-revocation for the non-anchor path.
};

}
