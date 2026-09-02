// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/core/Application.hpp>
#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/protected_data/ProtectedDataMode.hpp>
#include <erbsland/cryptology/tls/TlsConfigurationParser.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <string_view>

using namespace el::text::literals;

TESTED_TARGETS(
    TlsConfigurationParser TlsConfigurationEntry TlsConfigurationLabel DocumentValidator MatchesConstraint RulesBuilder)
class TlsConfigurationParserTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto parseEntry(const el::String &text) -> el::conf::ValuePtr {
        const auto document = el::conf::Parser{}.parseTextOrThrow(text);
        return document->valueOrThrow("tls"_el)->valueOrThrow("[0]"_el);
    }

    [[nodiscard]] static auto parseFixture(const std::string_view path) -> el::conf::ValuePtr {
        const auto fixturePath = el::path::Path{el::unittest::fh::resolveDataPath(path)};
        const auto document = el::conf::Parser{}.parseFileOrThrow(fixturePath);
        return document->valueOrThrow("tls"_el);
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
    void testCompiledRulesCoverCompleteSchema() {
        const auto entry = parseEntry(
            "*[tls]*\n"
            "label: \"http/server\"\n"
            "trust_anchors_file: \"roots.pem\"\n"
            "intermediate_certificates_file: \"intermediates.pem\"\n"
            "server_certificate_chain_file: \"server.pem\"\n"
            "server_signing_key_file: \"server-key.pem\"\n"_el);

        REQUIRE_EQUAL(el::cryptology::TlsConfigurationParser::version(), el::conf::Integer{1});
        REQUIRE_NOTHROW(el::cryptology::TlsConfigurationParser::validationRules()->validate(entry, 1));
        WITH_CONTEXT(requireRuleDocumentation(entry));
    }

    void testRulesRejectInvalidLabelsUnknownValuesAndDependencies() {
        const auto invalidEntries = std::array{
            "*[tls]*\nlabel: \"Http/server\"\n"_el,
            "*[tls]*\nlabel: \"http//server\"\n"_el,
            "*[tls]*\nlabel: \"http/server-\"\n"_el,
            "*[tls]*\nlabel: \"http/server\"\nunknown: \"value\"\n"_el,
            "*[tls]*\nlabel: \"http/client\"\nintermediate_certificates_file: \"issuer.pem\"\n"_el,
            "*[tls]*\nlabel: \"http/server\"\nserver_certificate_chain_file: \"server.pem\"\n"_el,
            "*[tls]*\nlabel: \"http/server\"\nserver_signing_key_file: \"server-key.pem\"\n"_el,
        };
        for (const auto &text : invalidEntries) {
            const auto entry = parseEntry(text);
            REQUIRE_THROWS_AS(
                el::conf::ConfError, el::cryptology::TlsConfigurationParser::validationRules()->validate(entry, 1));
        }
        REQUIRE_THROWS_AS(el::err::ParameterError, el::cryptology::TlsConfigurationParser{}.parse({}));
    }

    void testParsesEmptyAndGlobalOverrides() {
        auto entry = el::cryptology::TlsConfigurationParser{}.parse(parseEntry("*[tls]*\nlabel: \"\"\n"_el));
        REQUIRE(entry.label().isEmpty());
        REQUIRE_FALSE(entry.configuration().hasServerCertificatePolicy());
        REQUIRE_FALSE(entry.configuration().hasServerIdentity());

        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        configuration.setTlsConfiguration(entry.label(), entry.takeConfiguration());
        REQUIRE(configuration.hasTlsConfiguration(""_el));
    }

    void testParsesRelativeServerClientAndCombinedProfiles() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(
            el::cryptology::ProtectedDataMode::InternalOnly);
        const auto entries = parseFixture("data/cryptology/tls_configuration/profiles.elcl");
        REQUIRE_EQUAL(entries->size(), std::size_t{3U});

        const auto server = el::cryptology::TlsConfigurationParser{}.parse(entries->valueOrThrow("[0]"_el));
        REQUIRE_EQUAL(server.label(), "http/server"_el);
        REQUIRE(server.configuration().hasServerIdentity());
        REQUIRE_FALSE(server.configuration().hasServerCertificatePolicy());

        const auto client = el::cryptology::TlsConfigurationParser{}.parse(entries->valueOrThrow("[1]"_el));
        REQUIRE_EQUAL(client.label(), "http/client"_el);
        REQUIRE(client.configuration().hasServerCertificatePolicy());
        REQUIRE_FALSE(client.configuration().hasServerIdentity());
        REQUIRE_EQUAL(
            client.configuration().serverCertificatePolicy()->trustAnchors().certificates().count(), el::ItemCount{1U});
        REQUIRE_EQUAL(
            client.configuration().serverCertificatePolicy()->intermediates().certificates().count(),
            el::ItemCount{1U});

        const auto combined = el::cryptology::TlsConfigurationParser{}.parse(entries->valueOrThrow("[2]"_el));
        REQUIRE(combined.configuration().hasServerCertificatePolicy());
        REQUIRE(combined.configuration().hasServerIdentity());
    }

    void testIncludedProfileResolvesBesideIncludedSource() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(
            el::cryptology::ProtectedDataMode::InternalOnly);
        const auto entries = parseFixture("data/cryptology/tls_configuration/include-root.elcl");
        const auto entry = el::cryptology::TlsConfigurationParser{}.parse(entries->valueOrThrow("[0]"_el));
        REQUIRE_EQUAL(entry.label(), "included/server"_el);
        REQUIRE(entry.configuration().hasServerIdentity());
    }

    void testRelativeReferenceFromTextSourceIsRejectedAtValue() {
        const auto entry = parseEntry(
            "*[tls]*\n"
            "label: \"http/client\"\n"
            "trust_anchors_file: \"roots.pem\"\n"_el);
        const auto responsibleValue = entry->valueOrThrow("trust_anchors_file"_el);
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
        const auto text = el::text::StringFormat{"*[tls]*\n"
                                                 "label: \"http/server\"\n"
                                                 "server_certificate_chain_file: \"{}\"\n"
                                                 "server_signing_key_file: \"{}\"\n"_el}
                              .build(certificatePath.toString(), keyPath.toString());
        const auto entry = parseEntry(text);
        const auto responsibleValue = entry->valueOrThrow("server_signing_key_file"_el);
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
            "*[tls]*\n"
            "label: \"http/client\"\n"
            "trust_anchors_file: \"/this/path/does/not/exist.pem\"\n"_el);
        REQUIRE_THROWS_AS(el::conf::ConfError, el::cryptology::TlsConfigurationParser{}.parse(missing));

        const auto certificatePath =
            el::path::Path{el::unittest::fh::resolveDataPath("data/network/tls-interop/server.pem")};
        const auto malformedKeyPath =
            el::path::Path{el::unittest::fh::resolveDataPath("data/cryptology/x509/server_authentication.pem")};
        const auto malformed = parseEntry(
            el::text::StringFormat{"*[tls]*\n"
                                   "label: \"http/server\"\n"
                                   "server_certificate_chain_file: \"{}\"\n"
                                   "server_signing_key_file: \"{}\"\n"_el}
                .build(certificatePath.toString(), malformedKeyPath.toString()));
        REQUIRE_THROWS_AS(el::conf::ConfError, el::cryptology::TlsConfigurationParser{}.parse(malformed));

        const auto encryptedKeyPath =
            el::path::Path{el::unittest::fh::resolveDataPath("data/cryptology/tls_configuration/encrypted-key.pem")};
        const auto encrypted = parseEntry(
            el::text::StringFormat{"*[tls]*\n"
                                   "label: \"http/server\"\n"
                                   "server_certificate_chain_file: \"{}\"\n"
                                   "server_signing_key_file: \"{}\"\n"_el}
                .build(certificatePath.toString(), encryptedKeyPath.toString()));
        REQUIRE_THROWS_AS(el::conf::ConfError, el::cryptology::TlsConfigurationParser{}.parse(encrypted));
    }
};
