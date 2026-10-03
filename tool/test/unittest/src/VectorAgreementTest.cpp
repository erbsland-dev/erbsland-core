// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VectorJsonFixture.hpp"

TESTED_TARGETS(VectorConverter)
class VectorAgreementTest final : public UNITTEST_SUBCLASS(VectorJsonFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testChaChaCasesAndX25519ZeroSecrets() {
        runInsideApplication([&] {
            auto groups = Array{};
            for (auto group = 0; group < 10; ++group) {
                auto length = 1;
                auto iv = 8;
                if (group == 0) {
                    length = 316;
                    iv = 96;
                }
                auto tests = Array{};
                for (auto i = 0; i < length; ++i) {
                    auto result = el::String{"invalid"_el};
                    if (group == 0 && i < 256) {
                        result = "valid"_el;
                    }
                    auto test = record(i, result);
                    for (const auto key : {"key"_el, "iv"_el, "aad"_el, "msg"_el, "ct"_el, "tag"_el}) {
                        test.set(key, "00"_el);
                    }
                    tests.append(test);
                }
                groups.append(
                    Json{Object{
                        {"type"_el, "AeadTest"_el},
                        {"keySize"_el, 256},
                        {"tagSize"_el, 128},
                        {"ivSize"_el, iv},
                        {"tests"_el, tests}}});
            }
            auto root = Json{Object{
                {"algorithm"_el, "CHACHA20-POLY1305"_el},
                {"numberOfTests"_el, 325},
                {"generatorVersion"_el, "test"_el},
                {"testGroups"_el, groups}}};
            const auto chacha =
                convert("chacha20_poly1305"_el, root).get("chacha20_poly1305/wycheproof.rsp"_el, el::String{});
            REQUIRE_EQUAL(
                tool::test_data::impl::VectorConverter::counts("chacha20_poly1305/wycheproof.rsp"_el, chacha).first,
                325);
            REQUIRE(chacha.contains("[IVlen = 8]"_el));
            REQUIRE(chacha.contains("Tag = 00"_el));
            auto tests = Array{};
            for (auto i = 0; i < 518; ++i) {
                auto result = el::String{"acceptable"_el};
                if (i < 264) {
                    result = "valid"_el;
                }
                auto test = record(i, result);
                test.set("public"_el, "01"_el).set("private"_el, "02"_el).set("shared"_el, "00"_el);
                if (i >= 487) {
                    test.set("flags"_el, Array{Json{"ZeroSharedSecret"_el}});
                }
                tests.append(test);
            }
            const auto group =
                Json{Object{{"type"_el, "XdhComp"_el}, {"curve"_el, "curve25519"_el}, {"tests"_el, tests}}};
            root.set("algorithm"_el, "XDH"_el).set("numberOfTests"_el, 518).set("testGroups"_el, Array{group});
            const auto x25519 = convert("x25519"_el, root).get("x25519/wycheproof.rsp"_el, el::String{});
            REQUIRE_EQUAL(
                tool::test_data::impl::VectorConverter::counts("x25519/wycheproof.rsp"_el, x25519).first, 518);
            REQUIRE(x25519.contains("Flags = ZeroSharedSecret\nPublic = 01\nPrivate = 02\nShared = 00"_el));
        });
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testLimboSelectionAndUnvalidatedCertificatePayloads() {
        runInsideApplication([&] {
            const auto ids = el::StringList{
                "pathlen::intermediate-violates-pathlen-0"_el,
                "pathlen::self-issued-certs-pathlen"_el,
                "rfc5280::eku::ee-eku-empty"_el,
                "rfc5280::eku::ee-wrong-eku"_el,
                "rfc5280::eku::ee-without-eku"_el,
                "rfc5280::validity::notbefore-exact"_el,
                "rfc5280::validity::notafter-exact"_el,
                "webpki::san::exact-dns-san"_el,
                "webpki::san::mismatch-domain-san"_el,
                "webpki::san::mismatch-subdomain-san"_el,
                "webpki::san::leftmost-wildcard-san"_el,
                "webpki::san::wildcard-embedded-leftmost-san"_el,
                "webpki::san::no-san"_el};
            const auto pem = "-----BEGIN CERTIFICATE-----\nMAA=\n-----END CERTIFICATE-----\n"_el;
            auto cases = Array{};
            for (const auto &id : ids) {
                cases.append(
                    Json{Object{
                        {"id"_el, id},
                        {"validation_kind"_el, "SERVER"_el},
                        {"expected_peer_name"_el, Json{Object{{"kind"_el, "DNS"_el}, {"value"_el, "example.test"_el}}}},
                        {"trusted_certs"_el, Array{Json{pem}}},
                        {"untrusted_intermediates"_el, Array{Json{pem}}},
                        {"validation_time"_el, Json{}},
                        {"expected_result"_el, "FAILURE"_el},
                        {"peer_certificate"_el, pem}}});
            }
            while (cases.count() < el::ItemCount{9786}) {
                cases.append(Json{Object{{"id"_el, "unselected"_el}}});
            }
            const auto root = Json{Object{{"testcases"_el, cases}}};
            const auto output = convert("x509_limbo"_el, root).get("x509/x509_limbo.rsp"_el, el::String{});
            REQUIRE_EQUAL(tool::test_data::impl::VectorConverter::counts("x509/x509_limbo.rsp"_el, output).first, 13);
            REQUIRE(
                output.contains("PeerDer = 3000\nAnchorDer = 3000\nIntermediateCount = 1\nIntermediate0Der = 3000"_el));
            REQUIRE_FALSE(output.contains("unselected"_el));
        });
    }
};
