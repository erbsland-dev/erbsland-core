#include "../core/ApplicationTestScope.hpp"

#include <erbsland/ByteBuffer.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ProtectedDataMode.hpp>
#include <erbsland/SecureRandom.hpp>
#include <erbsland/SecureStorage.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using namespace el::cryptology;

TESTED_TARGETS(
    SecureStorage MacosSecureStorageBackend MacosPasswordBuffer WindowsSecureStorageBackend WindowsCredentialBuffer
        MacosKeychainInteraction)
class SecureStorageNativeTest final : public el::UnitTest {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testIsolatedSilentCrud() {
        auto applicationScope = ApplicationTestScope<>();
        applicationScope.app().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        auto random = el::random::SecureRandom{};
        const auto name = el::StringFormat{"erbsland.unittest.{}"_el}.build(random.getUInt64(0U, UINT64_MAX));
        const auto storage = SecureStorage{name};
        REQUIRE(storage.capabilities().supported);
        REQUIRE_FALSE(storage.read("isolated-item"_el).has_value());
        try {
            storage.write("isolated-item"_el, el::ConstByteSpan{});
            REQUIRE(storage.read("isolated-item"_el).has_value());
            REQUIRE(storage.read("isolated-item"_el)->isEmpty());
            const auto data = el::ByteBuffer{el::ByteCount{2560U}, el::Byte{0x5aU}};
            storage.write("isolated-item"_el, ProtectedByteBlock{data.span()});
            const auto result = storage.read("isolated-item"_el);
            REQUIRE(result.has_value());
            REQUIRE(result->unprotect().isEqualConstTime(data.span()));
            REQUIRE_FALSE(storage.read("ISOLATED-ITEM"_el).has_value());
            const auto secondNamespace = SecureStorage{el::join(name, ".other"_el)};
            REQUIRE_FALSE(secondNamespace.read("isolated-item"_el).has_value());
            REQUIRE(storage.remove("isolated-item"_el));
            REQUIRE_FALSE(storage.remove("isolated-item"_el));
        } catch (...) {
            [[maybe_unused]] const auto removed = storage.remove("isolated-item"_el);
            throw;
        }
    }
};
