#include "MockSecureStorageBackend.hpp"
#include "SecureStorageTestAccess.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/placeholder/Replacer.hpp>
#include <erbsland/placeholder/ReplacerError.hpp>
#include <erbsland/placeholder/SecureStorageSource.hpp>
#include <erbsland/ProtectedDataMode.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>

using namespace el::text::literals;
using namespace el::cryptology;
namespace ph = el::text::placeholder;

TESTED_TARGETS(SecureStorageSource)
class SecureStorageSourceTest final : public el::UnitTest {
public:
    void testValidationNeverRetrieves() {
        const auto backend = std::make_shared<MockSecureStorageBackend>();
        auto replacer = ph::Replacer{};
        auto storage = SecureStorage{"test.namespace"_el};
        el::cryptology::impl::SecureStorageTestAccess{storage}.setBackend(backend);
        replacer.addSource(std::make_shared<ph::SecureStorageSource>(storage));
        REQUIRE(replacer.validate("${secure:not-present}"_el));
        REQUIRE_FALSE(replacer.validate("${secure:}"_el));
        REQUIRE_EQUAL(backend->readCount, 0U);
        REQUIRE_THROWS_AS(ph::ReplacerError, replacer.replaceOrThrow("${secure:not-present}"_el));
        REQUIRE_EQUAL(backend->readCount, 1U);
    }

    void testExactUtf8AndSensitiveFilters() {
        auto applicationScope = ApplicationTestScope<>();
        applicationScope.app().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto backend = std::make_shared<MockSecureStorageBackend>();
        auto storage = SecureStorage{"test.namespace"_el};
        el::cryptology::impl::SecureStorageTestAccess{storage}.setBackend(backend);
        const auto expected = el::String{"  café\t\r\n"_el};
        const auto plaintext = el::text::StringEncoder{expected}.encode(el::text::StringEncoding::Utf8);
        storage.write("item"_el, plaintext.span());
        auto replacer = ph::Replacer{};
        replacer.addSource(std::make_shared<ph::SecureStorageSource>(storage));
        replacer.addTextFilters();
        auto result = replacer.replaceOrThrow("${secure:item}"_el);
        REQUIRE_EQUAL(result, expected);
        REQUIRE(result.isSensitive());
        result = replacer.replaceOrThrow("prefix:${secure:item|trim|upper}:suffix"_el);
        REQUIRE_EQUAL(result, "prefix:CAFÉ:suffix"_el);
        REQUIRE(result.isSensitive());
        REQUIRE(result.copy().isSensitive());
        REQUIRE(result.slice(el::CpRange{el::CpIndex{7U}, el::CpCount{4U}}).isSensitive());
    }

    void testMissingEmptyBinaryAndErrors() {
        auto applicationScope = ApplicationTestScope<>();
        applicationScope.app().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto backend = std::make_shared<MockSecureStorageBackend>();
        auto storage = SecureStorage{"test.namespace"_el};
        el::cryptology::impl::SecureStorageTestAccess{storage}.setBackend(backend);
        auto source = ph::SecureStorageSource{storage};
        try {
            [[maybe_unused]] const auto result = source.resolve("secure"_el, "missing"_el);
            REQUIRE(false);
        } catch (const ph::ReplacerError &error) {
            REQUIRE(error.category() == ph::ReplacerErrorCategory::ValueNotFound);
        }
        storage.write("item"_el, el::ConstByteSpan{});
        REQUIRE(source.resolve("secure"_el, "item"_el).isEmpty());
        REQUIRE(source.resolve("secure"_el, "item"_el).isSensitive());
        auto replacer = ph::Replacer{};
        replacer.addSource(std::make_shared<ph::SecureStorageSource>(storage));
        replacer.addTextFilters();
        const auto fallback = replacer.replaceOrThrow("${secure:item|default:replacement}"_el);
        REQUIRE_EQUAL(fallback, "replacement"_el);
        REQUIRE(fallback.isSensitive());
        const auto invalidUtf8 = std::array{el::Byte{0xffU}};
        storage.write("item"_el, invalidUtf8);
        REQUIRE_THROWS_AS(ph::ReplacerError, source.resolve("secure"_el, "item"_el));
        const auto binary = std::array{el::Byte{0U}, el::Byte{0x41U}};
        storage.write("item"_el, binary);
        REQUIRE_THROWS_AS(ph::ReplacerError, source.resolve("secure"_el, "item"_el));
        backend->failure = SecureStorageErrorCategory::InteractionRequired;
        try {
            [[maybe_unused]] const auto result = source.resolve("secure"_el, "item"_el);
            REQUIRE(false);
        } catch (const ph::ReplacerError &error) {
            REQUIRE(error.category() == ph::ReplacerErrorCategory::Access);
            REQUIRE_EQUAL(error.reason(), "Secure-storage retrieval failed."_el);
        }
    }

    void testConfigurationCopiesKeepSensitivityButSecretRemainsRuleBased() {
        auto applicationScope = ApplicationTestScope<>();
        applicationScope.app().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto backend = std::make_shared<MockSecureStorageBackend>();
        auto storage = SecureStorage{"test.namespace"_el};
        el::cryptology::impl::SecureStorageTestAccess{storage}.setBackend(backend);
        const auto data =
            el::text::StringEncoder{el::String{"  example value  "_el}}.encode(el::text::StringEncoding::Utf8);
        storage.write("item"_el, data.span());
        auto parser = el::conf::Parser{};
        parser.addPlaceholderSource(std::make_shared<ph::SecureStorageSource>(storage));
        parser.addPlaceholderTextFilters();
        auto document = parser.parseTextOrThrow("[credentials]\nvalue: \"before ${secure:item|trim} after\"\n"_el);
        auto value = document->value("credentials.value"_el);
        REQUIRE_EQUAL(value->asText(), "before example value after"_el);
        REQUIRE(value->asText().isSensitive());
        REQUIRE_FALSE(value->isSecret());
        const auto internal = std::dynamic_pointer_cast<el::conf::impl::Value>(value);
        REQUIRE(internal != nullptr);
        REQUIRE(internal->deepCopy()->asText().isSensitive());
        auto rulesParser = el::conf::Parser{};
        const auto rules = el::conf::vr::Rules::createFromDocument(
            rulesParser.parseTextOrThrow("[credentials.value]\ntype: \"text\"\nis_secret: yes\n"_el));
        rules->validate(document, 1);
        REQUIRE(value->isSecret());
        const auto rejectingRules = el::conf::vr::Rules::createFromDocument(rulesParser.parseTextOrThrow(
            "[credentials.value]\ntype: \"text\"\nis_secret: yes\nchars: \"digits\"\n"_el));
        try {
            rejectingRules->validate(document, 1);
            REQUIRE(false);
        } catch (const el::conf::ConfError &error) {
            REQUIRE_FALSE(error.context().codeSnippet().has_value());
            REQUIRE_FALSE(error.reason().contains("example value"_el));
        }
    }
};
