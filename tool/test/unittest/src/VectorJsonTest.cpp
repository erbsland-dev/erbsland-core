// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VectorJsonFixture.hpp"

#include <erbsland/json/JsonValue.hpp>
#include <tool/test_data/impl/VectorConverter.hpp>

TESTED_TARGETS(VectorConverter)
class VectorJsonTest final : public UNITTEST_SUBCLASS(VectorJsonFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testEd25519PayloadsAttributionAndDimensions() {
        runInsideApplication([&] {
            auto groups = Array{};
            auto count = 0;
            for (auto group = 0; group < 78; ++group) {
                auto tests = Array{};
                const auto add = [&] {
                    auto result = el::String{"invalid"_el};
                    if (count < 88) {
                        result = "valid"_el;
                    }
                    tests.append(record(++count, result));
                };
                add();
                if (group < 73) {
                    add();
                }
                groups.append(
                    Json{Object{
                        {"type"_el, "EddsaVerify"_el},
                        {"publicKey"_el,
                            Json{Object{
                                {"type"_el, "EDDSAPublicKey"_el},
                                {"curve"_el, "edwards25519"_el},
                                {"keySize"_el, 255},
                                {"pk"_el, "00"_el}}}},
                        {"publicKeyDer"_el, "302a300506032b657003210000"_el},
                        {"tests"_el, tests}}});
            }
            auto root = Json{Object{
                {"algorithm"_el, "EDDSA"_el},
                {"numberOfTests"_el, 151},
                {"generatorVersion"_el, "fixture-version"_el},
                {"testGroups"_el, groups}}};
            const auto output = convert("ed25519"_el, root).get("signature/ED25519.rsp"_el, el::String{});
            REQUIRE(output.contains("# Upstream Wycheproof generator version: fixture-version"_el));
            REQUIRE(output.contains("[AlgorithmDer = 300506032b6570]"_el));
            REQUIRE(output.contains("Result = invalid\nFlags = Synthetic\nMsg =\nSig = 3001"_el));
            REQUIRE_EQUAL(
                tool::test_data::impl::VectorConverter::counts("signature/ED25519.rsp"_el, output).first, 151);
            root.set("numberOfTests"_el, 150);
            REQUIRE_THROWS(convert("ed25519"_el, root));
        });
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRsaAlgorithmIdentifiersAndNegativePayloads() {
        runInsideApplication([&] {
            for (
                const auto &id : el::StringList{
                    "rsa_pss256_32"_el,
                    "rsa_pss256_0"_el,
                    "rsa_pss256_0_params"_el,
                    "rsa_pss256_32_params"_el,
                    "rsa_pss384_48"_el,
                    "rsa_pkcs256"_el,
                    "rsa_pkcs384"_el}) {
                auto sha = el::String{"SHA-256"_el};
                auto salt = 32;
                auto count = 108;
                auto type = el::String{"RsassaPssVerify"_el};
                auto algorithm = el::String{"RSASSA-PSS"_el};
                if (id.contains("_0"_el)) {
                    salt = 0;
                    count = 103;
                }
                if (id.contains("params"_el)) {
                    type = "RsassaPssWithParametersVerify"_el;
                }
                if (id == "rsa_pss384_48"_el) {
                    sha = "SHA-384"_el;
                    salt = 48;
                    count = 141;
                }
                if (id.startsWith("rsa_pkcs"_el)) {
                    algorithm = "RSASSA-PKCS1-v1_5"_el;
                    type = "RsassaPkcs1Verify"_el;
                    salt = 0;
                    count = 259;
                }
                if (id == "rsa_pkcs384"_el) {
                    sha = "SHA-384"_el;
                    count = 258;
                }
                auto tests = Array{};
                for (auto i = 0; i < count; ++i) {
                    tests.append(record(i, "invalid"_el));
                }
                const auto group = Json{Object{
                    {"type"_el, type},
                    {"keySize"_el, 2048},
                    {"sha"_el, sha},
                    {"mgf"_el, "MGF1"_el},
                    {"mgfSha"_el, sha},
                    {"sLen"_el, salt},
                    {"publicKeyDer"_el, "3000"_el},
                    {"source"_el, Json{Object{{"name"_el, "test-source"_el}, {"version"_el, "1"_el}}}},
                    {"tests"_el, tests}}};
                const auto root = Json{
                    Object{{"algorithm"_el, algorithm}, {"numberOfTests"_el, count}, {"testGroups"_el, Array{group}}}};
                const auto outputs = convert(id, root);
                REQUIRE_EQUAL(outputs.count(), el::ItemCount{1});
                for (const auto &[name, text] : outputs) {
                    REQUIRE(text.contains("# Upstream vector source: test-source 1"_el));
                    REQUIRE(text.contains("[PublicKeyDer = 3000]"_el));
                    REQUIRE_EQUAL(tool::test_data::impl::VectorConverter::counts(name, text).first, count);
                    if (algorithm == "RSASSA-PSS"_el) {
                        REQUIRE(text.contains("[AlgorithmDer = 304106092a864886f70d01010a"_el));
                    } else if (sha == "SHA-256"_el) {
                        REQUIRE(text.contains("[AlgorithmDer = 300d06092a864886f70d01010b0500]"_el));
                    } else {
                        REQUIRE(text.contains("[AlgorithmDer = 300d06092a864886f70d01010c0500]"_el));
                    }
                }
            }
        });
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testEcdsaSchemas() {
        runInsideApplication([&] {
            for (const auto &id : el::StringList{"ecdsa_p256"_el, "ecdsa_p384"_el}) {
                auto count = 484;
                auto groupCount = 113;
                auto bits = 256;
                auto curve = el::String{"secp256r1"_el};
                auto sha = el::String{"SHA-256"_el};
                if (id == "ecdsa_p384"_el) {
                    count = 504;
                    groupCount = 105;
                    bits = 384;
                    curve = "secp384r1"_el;
                    sha = "SHA-384"_el;
                }
                auto groups = Array{};
                auto remaining = count;
                for (auto i = 0; i < groupCount; ++i) {
                    auto tests = Array{};
                    const auto length = remaining / (groupCount - i);
                    remaining -= length;
                    for (auto j = 0; j < length; ++j) {
                        tests.append(record(j, "invalid"_el));
                    }
                    groups.append(
                        Json{Object{
                            {"type"_el, "EcdsaVerify"_el},
                            {"sha"_el, sha},
                            {"publicKeyDer"_el, "3000"_el},
                            {"publicKey"_el, Json{Object{{"curve"_el, curve}, {"keySize"_el, bits}}}},
                            {"tests"_el, tests}}});
                }
                const auto root = Json{Object{
                    {"algorithm"_el, "ECDSA"_el},
                    {"numberOfTests"_el, count},
                    {"generatorVersion"_el, "test"_el},
                    {"testGroups"_el, groups}}};
                const auto outputs = convert(id, root);
                for (const auto &[name, text] : outputs) {
                    REQUIRE_EQUAL(tool::test_data::impl::VectorConverter::counts(name, text).first, count);
                    REQUIRE(text.contains("Sig = 3001"_el));
                }
            }
        });
    }
};
