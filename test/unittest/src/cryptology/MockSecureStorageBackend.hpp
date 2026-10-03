#pragma once

#include <erbsland/cryptology/impl/secure_storage/SecureStorageBackend.hpp>
#include <erbsland/SecureStorageError.hpp>

/// Deterministic in-memory provider recording calls without accessing native storage.
class MockSecureStorageBackend final : public el::cryptology::impl::SecureStorageBackend {
public: // implement SecureStorageBackend
    [[nodiscard]] auto capabilities() const noexcept -> el::cryptology::SecureStorageCapabilities override {
        return {true, true};
    }
    [[nodiscard]] auto read(
        const el::String &applicationNamespace,
        const el::String &item,
        el::cryptology::SecureStorageInteraction interaction)
        -> std::optional<el::cryptology::ProtectedByteBlock> override {
        ++readCount;
        lastNamespace = applicationNamespace;
        lastItem = item;
        lastInteraction = interaction;
        if (failure.has_value()) {
            throw el::cryptology::SecureStorageError{*failure, el::String{"Injected native failure."}};
        }
        return value;
    }
    void write(
        const el::String &applicationNamespace,
        const el::String &item,
        el::ConstByteSpan data,
        el::cryptology::SecureStorageInteraction interaction) override {
        ++writeCount;
        lastNamespace = applicationNamespace;
        lastItem = item;
        lastInteraction = interaction;
        if (failure.has_value()) {
            throw el::cryptology::SecureStorageError{*failure, el::String{"Injected native failure."}};
        }
        auto replacement = el::cryptology::ProtectedByteBlock{data};
        value = std::move(replacement);
    }
    [[nodiscard]] auto remove(
        const el::String &, const el::String &, el::cryptology::SecureStorageInteraction interaction) -> bool override {
        ++removeCount;
        lastInteraction = interaction;
        if (failure.has_value()) {
            throw el::cryptology::SecureStorageError{*failure, el::String{"Injected native failure."}};
        }
        const auto existed = value.has_value();
        value.reset();
        return existed;
    }

    std::optional<el::cryptology::ProtectedByteBlock> value;
    std::optional<el::cryptology::SecureStorageErrorCategory> failure;
    std::size_t readCount{0};
    std::size_t writeCount{0};
    std::size_t removeCount{0};
    el::String lastNamespace;
    el::String lastItem;
    el::cryptology::SecureStorageInteraction lastInteraction{el::cryptology::SecureStorageInteraction::Allow};
};
