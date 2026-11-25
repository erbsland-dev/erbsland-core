// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VectorConverter.hpp"

#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::test_data::impl {

using namespace el::text::literals;

void VectorConverter::convertBlake(const el::json::JsonValue &root) {
    auto records = el::render::ValueList{};
    auto count = 0;
    for (const auto &value : array(root)) {
        if (text(value, "hash"_el) != "blake2b"_el || !text(value, "key"_el).isEmpty()) {
            continue;
        }
        const auto message = text(value, "in"_el);
        records.append(
            el::render::Value{el::render::ValueList{
                field("Count"_el, decimal(count++)),
                field("Len"_el, decimal(static_cast<int64_t>(message.length().toSizeT()) * 4)),
                field("Msg"_el, message),
                field("MD"_el, text(value, "out"_el))}});
    }
    if (count != 256) {
        fail("Expected 256 unkeyed BLAKE2b-512 records."_el);
    }
    _outputs.set(
        "hash/BLAKE2b.rsp"_el,
        normalize(render(
            el::StringList{
                "# Official BLAKE2 unkeyed BLAKE2b-512 known-answer vectors"_el,
                "# Converted deterministically from blake2-kat.json"_el},
            el::render::ValueList{group(
                el::render::ValueList{
                    field("Hash"_el, "BLAKE2b"_el), field("Keylen"_el, "0"_el), field("Outlen"_el, "512"_el)},
                records)})));
}

void VectorConverter::convertLimbo(const el::json::JsonValue &root) {
    const auto cases = array(root.get("testcases"_el));
    if (cases.count() != el::ItemCount{9786}) {
        fail("Unexpected x509-limbo source dimensions."_el);
    }
    // Preserve the reviewed subset and its order; these cases match the library's supported validation policy.
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
    auto byId = el::StringMap<el::json::JsonValue>{};
    for (const auto &test : cases) {
        const auto id = text(test, "id"_el);
        if (ids.contains(id)) {
            if (byId.contains(id)) {
                fail("Duplicate selected x509-limbo case."_el);
            }
            byId.set(id, test);
        }
    }
    auto records = el::render::ValueList{};
    auto count = 0;
    for (const auto &id : ids) {
        const auto test = byId.get(id);
        if (!test) {
            fail(el::join("Missing selected x509-limbo case: "_el, id));
        }
        const auto peer = test->get("expected_peer_name"_el);
        const auto anchors = array(test->get("trusted_certs"_el));
        const auto intermediates = array(test->get("untrusted_intermediates"_el));
        if (text(*test, "validation_kind"_el) != "SERVER"_el || text(peer, "kind"_el) != "DNS"_el ||
            anchors.count() != el::ItemCount{1}) {
            fail("Unsupported selected x509-limbo parameters."_el);
        }
        const auto anchor = anchors.first().getText();
        if (!anchor) {
            fail("Invalid trust-anchor PEM field."_el);
        }
        auto time = el::String{};
        if (!test->get("validation_time"_el).is(el::json::JsonType::Null)) {
            time = text(*test, "validation_time"_el);
        }
        auto fields = el::render::ValueList{
            field("Count"_el, decimal(count++)),
            field("Id"_el, id),
            field("Expected"_el, text(*test, "expected_result"_el)),
            field("ValidationTime"_el, time),
            field("IdentityKind"_el, "DNS"_el),
            field("Identity"_el, text(peer, "value"_el)),
            field("PeerDer"_el, certificate(text(*test, "peer_certificate"_el))),
            field("AnchorDer"_el, certificate(*anchor)),
            field("IntermediateCount"_el, decimal(static_cast<int64_t>(intermediates.count().toSizeT())))};
        auto index = 0;
        for (const auto &intermediate : intermediates) {
            const auto pem = intermediate.getText();
            if (!pem) {
                fail("Invalid intermediate PEM field."_el);
            }
            fields.append(field(el::StringFormat{"Intermediate{}Der"_el}.build(index++), certificate(*pem)));
        }
        records.append(el::render::Value{fields});
    }
    const auto header = el::StringList{
        "# C2SP x509-limbo server-certificate path-validation subset"_el,
        "# SPDX-License-Identifier: Apache-2.0"_el,
        el::join("# Upstream commit: "_el, _revision),
        "# Modified by Erbsland Core: selected compatible cases and converted certificates to DER hex"_el,
        "# Excluded features: RFC 4518 name equivalence, IDNA conversion, name constraints, policies, revocation"_el};
    _outputs.set(
        "x509/x509_limbo.rsp"_el,
        el::join(
            normalize(render(
                header,
                el::render::ValueList{group(el::render::ValueList{field("SourceCommit"_el, _revision)}, records)})),
            "\n"_el));
}

}
