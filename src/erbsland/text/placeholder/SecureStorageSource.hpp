#pragma once

#include "Source.hpp"

#include "../../cryptology/SecureStorage.hpp"

#include <utility>

namespace erbsland::text::placeholder {

/// An explicitly registered, namespace-bound `${secure:item}` persistent-secret source.
/// @seedoc{/reference/cryptology/protected_data}
/// @tested{SecureStorageSourceTest}
class SecureStorageSource final : public Source {
public:
    /// Bind the source to storage without retrieving secrets or unlocking a vault.
    /// @param storage The application namespace and interaction policy to use.
    explicit SecureStorageSource(cryptology::SecureStorage storage) : _storage{std::move(storage)} {}

public: // implement Source
    [[nodiscard]] auto sourceNames() const -> StringList override;
    [[nodiscard]] auto resolve(const String &sourceName, const String &parameter) -> String override;
    [[nodiscard]] auto validate(const String &sourceName, const String &parameter) -> bool override;

private:
    cryptology::SecureStorage _storage; ///< Namespace-bound synchronous storage.
};

}
