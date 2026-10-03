#pragma once

#include "SystemIdentityBackend.hpp"

#include "../CryptologyError.hpp"

#include "../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

/// Explicit unsupported result without a Linux key-file or plaintext fallback.
/// @tested{SystemIdentityStoreTest}
class UnsupportedSystemIdentityBackend final : public SystemIdentityBackend {
public: // implement SystemIdentityBackend
    [[nodiscard]] auto isSupported() const noexcept -> bool override { return false; }
    [[nodiscard]] auto list() const -> util::List<SystemIdentityInfo> override {
        using namespace text::literals;
        throw CryptologyError{"Native identity storage is unsupported on this platform; configure a file identity."_el};
    }
    [[nodiscard]] auto load(const mem::ByteBlock &) const -> SystemIdentityMaterial override {
        using namespace text::literals;
        throw CryptologyError{"Native identity storage is unsupported on this platform; configure a file identity."_el};
    }
};

}
