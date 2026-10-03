// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/ProtectedDataMode.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/TlsConfigurationParser.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <string_view>

using namespace el::text::literals;

TESTED_TARGETS(
    TlsConfigurationParser TlsConfigurationEntry TlsConfigurationLabel DocumentValidator MatchesConstraint RulesBuilder)
class TlsConfigurationParserTest final : public el::UnitTest {
private:
    static constexpr auto cFirstProfileValue = "[0]"_el;
    static constexpr auto cClientValue = "client"_el;
    static constexpr auto cTlsValue = "tls"_el;

private:
    [[nodiscard]] static auto parseEntry(const el::String &text) -> el::conf::ValuePtr {
        const auto document = el::conf::Parser{}.parseTextOrThrow(text);
        return document->valueOrThrow(cTlsValue);
    }

    [[nodiscard]] static auto parseFixture(const std::string_view path) -> el::conf::ValuePtr {
        const auto fixturePath = el::path::Path{el::unittest::fh::resolveDataPath(path)};
        const auto document = el::conf::Parser{}.parseFileOrThrow(fixturePath);
        return document->valueOrThrow(cTlsValue);
    }

    void requireRuleDocumentation(const el::conf::ValuePtr &value) {
        REQUIRE(value);
        const auto rule = value->validationRule();
        REQUIRE(rule);
        REQUIRE_FALSE(rule->title().isEmpty());
        REQUIRE_FALSE(rule->description().isEmpty());
        for (const auto &child : *value) {
            WITH_CONTEXT(requireRuleDocumentation(child));
        }
    }

public:
    void testDefaultsSettingsAndSecretRules() {
        using namespace el::cryptology;
        REQUIRE(TlsConfigurationParser{}.parse({}).entries().empty());
        const auto section = parseEntry(
            "[TLS]\nsystem_trust: true\ntrust_allow_network: false\nidentity_allow_interaction: false\n*[TLS.Client]*\nlabel: \"http\"\n"_el);
        const auto parsed = TlsConfigurationParser{}.parse(section);
        REQUIRE_EQUAL(parsed.entries().size(), std::size_t{1U});
        REQUIRE_FALSE(parsed.settings().allowsTrustNetwork());
        REQUIRE_FALSE(parsed.settings().allowsIdentityInteraction());
        const auto policy =
            parsed.entries()[0].configuration().client()->trust().resolve(parsed.settings(), TlsRole::Client);
        REQUIRE(policy->isSystemTrust());
        REQUIRE_FALSE(policy->allowsNetworkRetrieval());
        const auto password = parseEntry(
            "*[TLS.Client]*\nlabel: \"http\"\ncertificate_chain_file: \"client.pem\"\nsigning_key_file: \"key.pem\"\nsigning_key_password: \"\"\n"_el);
        TlsConfigurationParser::validationRules()->validate(password, 2);
        const auto value = password->valueOrThrow(cClientValue)
                               ->valueOrThrow(cFirstProfileValue)
                               ->valueOrThrow("signing_key_password"_el);
        requireRuleDocumentation(password);
        REQUIRE(value->isSecret());
        REQUIRE(value->asText().isEmpty());
        REQUIRE_EQUAL(TlsConfigurationParser::version(), el::conf::Integer{2});
    }

    void testRejectsOldSchemaReservedLabelsDuplicatesAndWrongRoleFields() {
        const auto inputs = std::array{
            "*[TLS]*\nlabel: \"http\"\n"_el,
            "*[TLS.Client]*\nlabel: \"\"\n"_el,
            "*[TLS.Client]*\nlabel: \"Http\"\n"_el,
            "*[TLS.Client]*\nlabel: \"http//internal\"\n"_el,
            "*[TLS.Client]*\nlabel: \"http\"\nunknown: true\n"_el,
            "*[TLS.Client]*\nlabel: \"http\"\ntrust_allow_network: false\n"_el,
            "*[TLS.Client]*\nlabel: \"http\"\nidentity_allow_interaction: false\n"_el,
            "*[TLS.Client]*\nlabel: \"http\"\nclient_authentication: \"required\"\n"_el,
            "*[TLS.Client]*\nlabel: \"http\"\n*[TLS.Client]*\nlabel: \"http\"\n"_el,
            "*[TLS.Server]*\nlabel: \"http\"\n"_el,
            "*[TLS.Client]*\nlabel: \"http\"\nsystem_trust: false\n"_el,
            "*[TLS.Client]*\nlabel: \"http\"\nintermediate_certificates_file: \"issuer.pem\"\n"_el,
            "*[TLS.Client]*\nlabel: \"http\"\nsigning_key_password: \"\"\n"_el,
            "*[TLS.Server]*\nlabel: \"http\"\ncertificate_chain_file: \"server.pem\"\n"_el};
        for (const auto input : inputs) {
            REQUIRE_THROWS_AS(el::conf::ConfError, el::cryptology::TlsConfigurationParser{}.parse(parseEntry(input)));
        }
    }

    void testParsesRelativeServerClientAndCombinedProfiles() {
        using namespace el::cryptology;
        auto scope = ApplicationTestScope<>{};
        auto &registry = scope.app().cryptologyConfiguration();
        registry.setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto section = parseFixture("data/cryptology/tls_configuration/profiles.elcl");
        const auto parsed = TlsConfigurationParser{}.parse(section);
        REQUIRE_EQUAL(parsed.entries().size(), std::size_t{3U});
        registry.replaceTlsConfigurations(parsed);
        REQUIRE(registry.hasTlsConfiguration("http/server"_el, TlsRole::Server));
        REQUIRE_FALSE(registry.hasTlsConfiguration("http/server"_el, TlsRole::Client));
        const auto client = registry.resolveTlsConfiguration("http/client"_el, TlsRole::Client);
        REQUIRE_FALSE(client.certificatePolicy()->isSystemTrust());
        REQUIRE_EQUAL(client.certificatePolicy()->trustAnchors().certificates().count(), el::ItemCount{1U});
        REQUIRE_EQUAL(client.certificatePolicy()->intermediates().certificates().count(), el::ItemCount{1U});
        REQUIRE(registry.hasTlsConfiguration("http/combined"_el, TlsRole::Client));
        REQUIRE(registry.hasTlsConfiguration("http/combined"_el, TlsRole::Server));
    }

    void testIncludedProfileResolvesBesideIncludedSource() {
        auto scope = ApplicationTestScope<>{};
        scope.app().cryptologyConfiguration().setProtectedDataMode(el::cryptology::ProtectedDataMode::InternalOnly);
        const auto parsed = el::cryptology::TlsConfigurationParser{}.parse(
            parseFixture("data/cryptology/tls_configuration/include-root.elcl"));
        REQUIRE_EQUAL(parsed.entries().size(), std::size_t{1U});
        REQUIRE_EQUAL(parsed.entries()[0].label(), "included/server"_el);
        REQUIRE(parsed.entries()[0].configuration().server()->identity());
    }

    void testRelativeReferenceFromTextSourceIsRejectedAtValue() {
        const auto entry = parseEntry(
            "*[TLS.Client]*\n"
            "label: \"http/client\"\n"
            "trust_anchors_file: \"roots.pem\"\n"_el);
        const auto responsibleValue =
            entry->valueOrThrow(cClientValue)->valueOrThrow(cFirstProfileValue)->valueOrThrow("trust_anchors_file"_el);
        try {
            static_cast<void>(el::cryptology::TlsConfigurationParser{}.parse(entry));
        } catch (const el::conf::ConfError &error) {
            REQUIRE_EQUAL(error.namePath(), responsibleValue->namePath());
            REQUIRE_FALSE(error.location().isUndefined());
            return;
        }
        REQUIRE(false);
    }

    void testMismatchedIdentityRetainsLocationAndCause() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(
            el::cryptology::ProtectedDataMode::InternalOnly);
        const auto certificatePath =
            el::path::Path{el::unittest::fh::resolveDataPath("data/network/tls-interop/server.pem")};
        const auto keyPath =
            el::path::Path{el::unittest::fh::resolveDataPath("data/network/tls-interop/server-ecdsa-key.pem")};
        const auto text = el::text::StringFormat{"*[TLS.Server]*\n"
                                                 "label: \"http/server\"\n"
                                                 "certificate_chain_file: \"{}\"\n"
                                                 "signing_key_file: \"{}\"\n"_el}
                              .build(certificatePath.toString(), keyPath.toString());
        const auto entry = parseEntry(text);
        const auto responsibleValue =
            entry->valueOrThrow("server"_el)->valueOrThrow(cFirstProfileValue)->valueOrThrow("signing_key_file"_el);
        try {
            static_cast<void>(el::cryptology::TlsConfigurationParser{}.parse(entry));
        } catch (const el::conf::ConfError &error) {
            REQUIRE_EQUAL(error.namePath(), responsibleValue->namePath());
            REQUIRE_FALSE(error.location().isUndefined());
            REQUIRE(error.hasCause());
            return;
        }
        REQUIRE(false);
    }

    void testMissingMalformedAndEncryptedFilesAreRejected() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(
            el::cryptology::ProtectedDataMode::InternalOnly);
        const auto missing = parseEntry(
            "*[TLS.Client]*\n"
            "label: \"http/client\"\n"
            "trust_anchors_file: \"/this/path/does/not/exist.pem\"\n"_el);
        REQUIRE_THROWS_AS(el::conf::ConfError, el::cryptology::TlsConfigurationParser{}.parse(missing));

        const auto certificatePath =
            el::path::Path{el::unittest::fh::resolveDataPath("data/network/tls-interop/server.pem")};
        const auto malformedKeyPath =
            el::path::Path{el::unittest::fh::resolveDataPath("data/cryptology/x509/server_authentication.pem")};
        const auto malformed = parseEntry(
            el::text::StringFormat{"*[TLS.Server]*\n"
                                   "label: \"http/server\"\n"
                                   "certificate_chain_file: \"{}\"\n"
                                   "signing_key_file: \"{}\"\n"_el}
                .build(certificatePath.toString(), malformedKeyPath.toString()));
        REQUIRE_THROWS_AS(el::conf::ConfError, el::cryptology::TlsConfigurationParser{}.parse(malformed));

        const auto encryptedKeyPath =
            el::path::Path{el::unittest::fh::resolveDataPath("data/cryptology/tls_configuration/encrypted-key.pem")};
        const auto encrypted = parseEntry(
            el::text::StringFormat{"*[TLS.Server]*\n"
                                   "label: \"http/server\"\n"
                                   "certificate_chain_file: \"{}\"\n"
                                   "signing_key_file: \"{}\"\n"_el}
                .build(certificatePath.toString(), encryptedKeyPath.toString()));
        REQUIRE_THROWS_AS(el::conf::ConfError, el::cryptology::TlsConfigurationParser{}.parse(encrypted));
    }
};
