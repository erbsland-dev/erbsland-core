// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VectorConverter.hpp"

#include <erbsland/Set.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::test_data::impl {

using namespace el::text::literals;

auto VectorConverter::wycheproofHeader(const el::json::JsonValue &root, const el::String &title) const
    -> el::StringList {
    auto header = el::StringList{
        el::join("# C2SP Wycheproof "_el, title, " vectors"_el),
        el::join("# Upstream file: "_el, _source.filename),
        el::join("# Upstream commit: "_el, _revision)};
    auto attributions = el::Set<el::String>{};
    for (const auto &value : array(root.get(cTestGroupsKey))) {
        const auto attribution = value.get("source"_el);
        if (!attribution.is(el::json::JsonType::Null)) {
            const auto name = text(attribution, "name"_el);
            const auto version = text(attribution, "version"_el);
            if (name.isEmpty() || version.isEmpty()) {
                fail("Empty source attribution."_el);
            }
            attributions.insert(el::join(name, " "_el, version));
        }
    }
    if (attributions.count().isZero()) {
        const auto version = text(root, "generatorVersion"_el);
        if (version.isEmpty()) {
            fail("Missing generator attribution."_el);
        }
        header.append(el::join("# Upstream Wycheproof generator version: "_el, version));
    } else {
        for (const auto &value : attributions) {
            header.append(el::join("# Upstream vector source: "_el, value));
        }
    }
    header.append("# Modified by Erbsland Core: converted from JSON into deterministic response-file form"_el);
    header.append("# License: Apache-2.0; see the repository LICENSE file"_el);
    header.append("# SPDX-License-Identifier: Apache-2.0"_el);
    return header;
}

auto VectorConverter::algorithmDer(const el::String &padding, const el::String &sha, const int64_t salt) const
    -> el::String {
    if (padding == "ECDSA"_el) {
        if (sha == "SHA-256"_el) {
            return "300a06082a8648ce3d040302"_el;
        }
        if (sha == "SHA-384"_el) {
            return "300a06082a8648ce3d040303"_el;
        }
    }
    if (padding == "PKCS1"_el && salt == 0) {
        if (sha == "SHA-256"_el) {
            return "300d06092a864886f70d01010b0500"_el;
        }
        if (sha == "SHA-384"_el) {
            return "300d06092a864886f70d01010c0500"_el;
        }
    }
    auto oid = el::String{};
    auto maximum = 0;
    if (sha == "SHA-256"_el) {
        oid = "01"_el;
        maximum = 32;
    }
    if (sha == "SHA-384"_el) {
        oid = "02"_el;
        maximum = 48;
    }
    if (padding != "PSS"_el || oid.isEmpty() || salt < 0 || salt > maximum) {
        fail("Unsupported signature AlgorithmIdentifier parameters."_el);
    }
    return el::StringFormat{
        "304106092a864886f70d01010a3034a00f300d06096086480165030402{}0500"
        "a11c301a06092a864886f70d010108300d06096086480165030402{}0500a2030201{:number:hex,precision=2,lc=lowercase}"_el}
        .build(oid, oid, salt);
}

void VectorConverter::convertWycheproof(const el::json::JsonValue &root) {
    auto algorithm = el::String{};
    auto title = el::String{};
    auto output = el::String{};
    auto expected = 0;
    auto expectedGroups = 0;
    auto sha = el::String{};
    auto curve = el::String{};
    auto padding = el::String{};
    auto expectedType = el::String{};
    if (_source.family == "chacha20-poly1305"_el) {
        algorithm = "CHACHA20-POLY1305"_el;
        title = "ChaCha20-Poly1305"_el;
        output = "chacha20_poly1305/wycheproof.rsp"_el;
        expected = 325;
        expectedGroups = 10;
    } else if (_source.family == "x25519"_el) {
        algorithm = "XDH"_el;
        title = "X25519"_el;
        output = "x25519/wycheproof.rsp"_el;
        expected = 518;
        expectedGroups = 1;
    } else if (_source.family == "ed25519"_el) {
        algorithm = "EDDSA"_el;
        title = "Ed25519"_el;
        output = "signature/ED25519.rsp"_el;
        expected = 151;
        expectedGroups = 78;
    } else if (_source.family == "ecdsa"_el) {
        algorithm = "ECDSA"_el;
        sha = "SHA-256"_el;
        curve = "secp256r1"_el;
        expected = 484;
        expectedGroups = 113;
        output = "signature/ECDSA_P256_SHA256.rsp"_el;
        if (_source.id == "ecdsa_p384"_el) {
            sha = "SHA-384"_el;
            curve = "secp384r1"_el;
            expected = 504;
            expectedGroups = 105;
            output = "signature/ECDSA_P384_SHA384.rsp"_el;
        }
        title = el::join("ECDSA "_el, curve, " "_el, sha);
    } else {
        padding = "PSS"_el;
        algorithm = "RSASSA-PSS"_el;
        sha = "SHA-256"_el;
        expectedType = "RsassaPssVerify"_el;
        if (_source.id == "rsa_pss256_32"_el) {
            expected = 108;
            output = "PSS_SHA256.rsp"_el;
        } else if (_source.id == "rsa_pss256_0"_el) {
            expected = 103;
            output = "PSS_SHA256_SALT0.rsp"_el;
        } else if (_source.id == "rsa_pss256_0_params"_el) {
            expected = 103;
            output = "PSS_SHA256_PSS_KEY_SALT0.rsp"_el;
            expectedType = "RsassaPssWithParametersVerify"_el;
        } else if (_source.id == "rsa_pss256_32_params"_el) {
            expected = 108;
            output = "PSS_SHA256_PSS_KEY_SALT32.rsp"_el;
            expectedType = "RsassaPssWithParametersVerify"_el;
        } else if (_source.id == "rsa_pss384_48"_el) {
            expected = 141;
            output = "PSS_SHA384.rsp"_el;
            sha = "SHA-384"_el;
        } else {
            padding = "PKCS1"_el;
            algorithm = "RSASSA-PKCS1-v1_5"_el;
            expectedType = "RsassaPkcs1Verify"_el;
            expected = 259;
            output = "PKCS1_SHA256.rsp"_el;
            if (_source.id == "rsa_pkcs384"_el) {
                expected = 258;
                output = "PKCS1_SHA384.rsp"_el;
                sha = "SHA-384"_el;
            }
        }
        output = el::join("signature/"_el, output);
        title = el::join(algorithm, " "_el, sha);
    }
    if (text(root, "algorithm"_el) != algorithm || integer(root, "numberOfTests"_el) != expected) {
        fail("Unexpected Wycheproof algorithm or test count."_el);
    }
    auto groups = el::render::ValueList{};
    auto records = 0;
    auto valid = 0;
    auto invalid = 0;
    auto acceptable = 0;
    auto zero = 0;
    auto authFailures = 0;
    auto nonceFailures = 0;
    for (const auto &sourceGroup : array(root.get(cTestGroupsKey))) {
        auto headers = el::render::ValueList{};
        auto ivSize = int64_t{};
        if (_source.family == "chacha20-poly1305"_el) {
            if (text(sourceGroup, "type"_el) != "AeadTest"_el || integer(sourceGroup, "keySize"_el) != 256 ||
                integer(sourceGroup, "tagSize"_el) != 128) {
                fail("Unexpected ChaCha20-Poly1305 group."_el);
            }
            ivSize = integer(sourceGroup, "ivSize"_el);
            headers = el::render::ValueList{
                field("IVlen"_el, decimal(ivSize)), field("Keylen"_el, "256"_el), field("Taglen"_el, "128"_el)};
        } else if (_source.family == "x25519"_el) {
            if (text(sourceGroup, "type"_el) != "XdhComp"_el || text(sourceGroup, "curve"_el) != "curve25519"_el) {
                fail("Unexpected X25519 group."_el);
            }
            headers = el::render::ValueList{field("Curve"_el, "curve25519"_el)};
        } else if (_source.family == "ed25519"_el) {
            const auto key = sourceGroup.get(cPublicKeyKey);
            if (text(sourceGroup, "type"_el) != "EddsaVerify"_el || text(key, "type"_el) != "EDDSAPublicKey"_el ||
                text(key, "curve"_el) != "edwards25519"_el || integer(key, "keySize"_el) != 255) {
                fail("Unexpected Ed25519 group."_el);
            }
            const auto der = text(sourceGroup, "publicKeyDer"_el);
            if (der != el::join("302a300506032b6570032100"_el, text(key, "pk"_el))) {
                fail("Unexpected Ed25519 SPKI encoding."_el);
            }
            headers =
                el::render::ValueList{field("PublicKeyDer"_el, der), field("AlgorithmDer"_el, "300506032b6570"_el)};
        } else if (_source.family == "ecdsa"_el) {
            const auto key = sourceGroup.get(cPublicKeyKey);
            auto size = 256;
            if (curve == "secp384r1"_el) {
                size = 384;
            }
            if (text(sourceGroup, "type"_el) != "EcdsaVerify"_el || text(sourceGroup, "sha"_el) != sha ||
                text(key, "curve"_el) != curve || integer(key, "keySize"_el) != size) {
                fail("Unexpected ECDSA group."_el);
            }
            headers = el::render::ValueList{
                field("Curve"_el, curve),
                field("Hash"_el, sha),
                field("PublicKeyDer"_el, text(sourceGroup, "publicKeyDer"_el)),
                field("AlgorithmDer"_el, algorithmDer("ECDSA"_el, sha, 0))};
        } else {
            auto salt = int64_t{};
            if (padding == "PSS"_el) {
                salt = integer(sourceGroup, "sLen"_el);
                if (text(sourceGroup, "mgf"_el) != "MGF1"_el || text(sourceGroup, "mgfSha"_el) != sha) {
                    fail("Unexpected RSA mask generation parameters."_el);
                }
            }
            if (text(sourceGroup, "type"_el) != expectedType || integer(sourceGroup, "keySize"_el) != 2048 ||
                text(sourceGroup, "sha"_el) != sha) {
                fail("Unexpected RSA group."_el);
            }
            headers = el::render::ValueList{
                field("Padding"_el, padding),
                field("Hash"_el, sha),
                field("SaltLength"_el, decimal(salt)),
                field("PublicKeyDer"_el, text(sourceGroup, "publicKeyDer"_el)),
                field("AlgorithmDer"_el, algorithmDer(padding, sha, salt))};
        }
        auto tests = el::render::ValueList{};
        for (const auto &test : array(sourceGroup.get("tests"_el))) {
            ++records;
            const auto result = text(test, "result"_el);
            if (result == "valid"_el) {
                ++valid;
            } else if (result == "invalid"_el) {
                ++invalid;
            } else if (result == "acceptable"_el) {
                ++acceptable;
            } else {
                fail("Unknown test result."_el);
            }
            if (_source.family == "chacha20-poly1305"_el || _source.family == "x25519"_el) {
                auto fields = el::render::ValueList{
                    field("Count"_el, decimal(integer(test, "tcId"_el))),
                    field("Result"_el, result),
                    field("Flags"_el, flags(test))};
                if (_source.family == "chacha20-poly1305"_el) {
                    if (result != "valid"_el) {
                        if (ivSize == 96) {
                            ++authFailures;
                        } else {
                            ++nonceFailures;
                        }
                    }
                    fields.append(field("Key"_el, text(test, "key"_el)));
                    fields.append(field("Nonce"_el, text(test, "iv"_el)));
                    fields.append(field("AAD"_el, text(test, "aad"_el)));
                    fields.append(field("Msg"_el, text(test, "msg"_el)));
                    fields.append(field("CT"_el, text(test, "ct"_el)));
                    fields.append(field("Tag"_el, text(test, "tag"_el)));
                } else {
                    for (const auto &flag : array(test.get("flags"_el))) {
                        if (flag.getText(el::String{}) == "ZeroSharedSecret"_el) {
                            ++zero;
                        }
                    }
                    fields.append(field("Public"_el, text(test, "public"_el)));
                    fields.append(field("Private"_el, text(test, "private"_el)));
                    fields.append(field("Shared"_el, text(test, "shared"_el)));
                }
                tests.append(el::render::Value{fields});
            } else {
                if (_source.family != "rsa"_el && result == "acceptable"_el) {
                    fail("Unexpected acceptable signature case."_el);
                }
                tests.append(signatureRecord(test));
            }
        }
        groups.append(group(headers, tests));
    }
    if (records != expected ||
        (expectedGroups != 0 && groups.count() != el::ItemCount::fromIntegerOrThrow(expectedGroups))) {
        fail("Unexpected Wycheproof dimensions."_el);
    }
    if (_source.family == "ed25519"_el && (valid != 88 || invalid != 63)) {
        fail("Unexpected Ed25519 result counts."_el);
    }
    if (_source.family == "x25519"_el && (valid != 264 || acceptable != 254 || zero != 31 || invalid != 0)) {
        fail("Unexpected X25519 result counts."_el);
    }
    if (_source.family == "chacha20-poly1305"_el && (valid != 256 || authFailures != 60 || nonceFailures != 9)) {
        fail("Unexpected AEAD result counts."_el);
    }
    _outputs.set(output, normalize(render(wycheproofHeader(root, title), groups)));
}

}
