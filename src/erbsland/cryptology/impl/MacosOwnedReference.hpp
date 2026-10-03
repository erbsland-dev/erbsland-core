#pragma once

#include <CoreFoundation/CoreFoundation.h>

#include <utility>

namespace erbsland::cryptology::impl {

/// Move-only owner of one Core Foundation create/copy result.
/// @tparam Reference A Core Foundation reference type released by CFRelease.
/// @tested{SystemIdentityStoreTest MacosSystemIdentityNativeTest}
template <typename Reference>
class MacosOwnedReference final {
public:
    /// Take ownership of a create/copy result, including null results.
    explicit MacosOwnedReference(Reference reference = nullptr) noexcept : _reference{reference} {}
    /// Release the owned reference.
    ~MacosOwnedReference() { reset(); }
    /// Take another owner's reference, leaving it empty.
    MacosOwnedReference(MacosOwnedReference &&other) noexcept : _reference{other.release()} {}
    /// Release this reference and take another owner's reference.
    auto operator=(MacosOwnedReference &&other) noexcept -> MacosOwnedReference & {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    // defaults/deletions
    MacosOwnedReference(const MacosOwnedReference &) = delete;
    auto operator=(const MacosOwnedReference &) -> MacosOwnedReference & = delete;

public: // ownership
    /// Get the borrowed native reference.
    [[nodiscard]] auto get() const noexcept -> Reference { return _reference; }
    /// Transfer ownership to the caller.
    [[nodiscard]] auto release() noexcept -> Reference { return std::exchange(_reference, nullptr); }
    /// Release the old reference and own its replacement.
    void reset(Reference reference = nullptr) noexcept {
        if (_reference != nullptr) {
            CFRelease(_reference);
        }
        _reference = reference;
    }
    /// Release any existing value and receive one create/copy output parameter.
    [[nodiscard]] auto put() noexcept -> Reference * {
        reset();
        return &_reference;
    }

private:
    Reference _reference{}; ///< Sole owning native reference.
};

}
