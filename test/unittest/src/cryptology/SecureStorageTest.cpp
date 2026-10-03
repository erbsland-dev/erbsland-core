#include "MockSecureStorageBackend.hpp"
#include "SecureStorageTestAccess.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/ByteBuffer.hpp>
#include <erbsland/cryptology/impl/secure_storage/UnsupportedSecureStorageBackend.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ProtectedDataMode.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using namespace el::cryptology;

TESTED_TARGETS(
    SecureStorage SecureStorageCapabilities SecureStorageError SecureStorageErrorCategory SecureStorageInteraction
        SecureStorageBackend UnsupportedSecureStorageBackend)
class SecureStorageTest final : public el::UnitTest {
public:
    void testNamesAndCapabilitiesWithoutVaultAccess() {
        const auto backend = std::make_shared<MockSecureStorageBackend>();
        auto storage = SecureStorage{"test.namespace"_el};
        el::cryptology::impl::SecureStorageTestAccess{storage}.setBackend(backend);
        REQUIRE(storage.capabilities().supported);
        REQUIRE_EQUAL(storage.applicationNamespace(), "test.namespace"_el);
        REQUIRE_EQUAL(storage.maximumPayloadLength(), el::ByteCount{2560U});
        REQUIRE_EQUAL(backend->readCount, 0U);
        REQUIRE(SecureStorage::isValidName("Unicode café: exact name"_el));
        REQUIRE_FALSE(SecureStorage::isValidName({}));
        REQUIRE_FALSE(SecureStorage::isValidName("line\nbreak"_el));
        REQUIRE_THROWS_AS(SecureStorageError, SecureStorage{""_el});
        REQUIRE_THROWS_AS(SecureStorageError, storage.read(""_el));
        REQUIRE_EQUAL(backend->readCount, 0U);
    }

    void testMissingEmptyAndReplacement() {
        auto applicationScope = ApplicationTestScope<>();
        applicationScope.app().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto backend = std::make_shared<MockSecureStorageBackend>();
        auto storage = SecureStorage{"test.namespace"_el};
        el::cryptology::impl::SecureStorageTestAccess{storage}.setBackend(backend);
        REQUIRE_FALSE(storage.read("item"_el).has_value());
        storage.write("item"_el, el::ConstByteSpan{});
        REQUIRE(storage.read("item"_el).has_value());
        REQUIRE(storage.read("item"_el)->isEmpty());
        const auto data = el::ByteBuffer{el::ByteCount{32U}, el::Byte{0x42U}};
        storage.write("item"_el, ProtectedByteBlock{data.span()});
        REQUIRE_EQUAL(storage.read("item"_el)->byteLength(), el::ByteCount{32U});
        REQUIRE(storage.read("item"_el)->unprotect().isSensitive());
        REQUIRE(backend->lastInteraction == SecureStorageInteraction::Forbid);
        REQUIRE(storage.remove("item"_el));
        REQUIRE_FALSE(storage.remove("item"_el));
    }

    void testPayloadLimitAndExplicitInteraction() {
        auto applicationScope = ApplicationTestScope<>();
        applicationScope.app().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto backend = std::make_shared<MockSecureStorageBackend>();
        auto storage = SecureStorage{"test.namespace"_el, SecureStorageInteraction::Allow};
        el::cryptology::impl::SecureStorageTestAccess{storage}.setBackend(backend);
        auto data = el::ByteBuffer{el::ByteCount{2560U}};
        storage.write("item"_el, data.span());
        REQUIRE(backend->lastInteraction == SecureStorageInteraction::Allow);
        REQUIRE_EQUAL(storage.read("item"_el)->byteLength(), el::ByteCount{2560U});
        data = el::ByteBuffer{el::ByteCount{2561U}};
        try {
            storage.write("item"_el, data.span());
            REQUIRE(false);
        } catch (const SecureStorageError &error) {
            REQUIRE(error.category() == SecureStorageErrorCategory::SizeLimit);
        }
        REQUIRE_EQUAL(backend->writeCount, 1U);
        backend->value = ProtectedByteBlock{data.span()};
        REQUIRE_THROWS_AS(SecureStorageError, storage.read("item"_el));
    }

    void testFailuresAreNotMissingValues() {
        const auto backend = std::make_shared<MockSecureStorageBackend>();
        auto storage = SecureStorage{"test.namespace"_el};
        el::cryptology::impl::SecureStorageTestAccess{storage}.setBackend(backend);
        for (
            const auto category :
            {SecureStorageErrorCategory::AccessDenied,
                SecureStorageErrorCategory::InteractionRequired,
                SecureStorageErrorCategory::Cancelled,
                SecureStorageErrorCategory::NativeFailure}) {
            backend->failure = category;
            try {
                [[maybe_unused]] auto value = storage.read("item"_el);
                REQUIRE(false);
            } catch (const SecureStorageError &error) {
                REQUIRE(error.category() == category);
            }
        }
    }

    void testUnsupportedHasNoFallback() {
        auto storage = SecureStorage{"test.namespace"_el};
        el::cryptology::impl::SecureStorageTestAccess{storage}.setBackend(
            std::make_shared<el::cryptology::impl::UnsupportedSecureStorageBackend>());
        REQUIRE_FALSE(storage.capabilities().supported);
        REQUIRE_THROWS_AS(SecureStorageError, storage.read("item"_el));
        REQUIRE_THROWS_AS(SecureStorageError, storage.write("item"_el, el::ConstByteSpan{}));
        REQUIRE_THROWS_AS(SecureStorageError, storage.remove("item"_el));
    }
};
