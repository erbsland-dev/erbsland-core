// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VectorConverter.hpp"

#include <erbsland/IntegerParseOptions.hpp>
#include <erbsland/StringDecoder.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/StringTransformations.hpp>
#include <erbsland/zip/ArchiveReader.hpp>

namespace tool::test_data::impl {

using namespace el::text::literals;

void VectorConverter::convertArchive(const el::ByteBlock &data) {
    auto members = el::StringList{};
    auto inputPrefix = el::String{};
    auto outputPrefix = el::join(_source.family, "/"_el);
    if (_source.id == "aes_kat"_el) {
        members = el::StringList{
            "CBCGFSbox256.rsp"_el,
            "CBCKeySbox256.rsp"_el,
            "CBCVarKey256.rsp"_el,
            "CBCVarTxt256.rsp"_el,
            "ECBGFSbox128.rsp"_el,
            "ECBGFSbox256.rsp"_el,
            "ECBKeySbox128.rsp"_el,
            "ECBKeySbox256.rsp"_el,
            "ECBVarKey128.rsp"_el,
            "ECBVarKey256.rsp"_el,
            "ECBVarTxt128.rsp"_el,
            "ECBVarTxt256.rsp"_el};
    } else if (_source.id == "aes_mct"_el) {
        members = el::StringList{"CBCMCT256.rsp"_el, "ECBMCT128.rsp"_el, "ECBMCT256.rsp"_el};
    } else if (_source.id == "aes_mmt"_el) {
        members = el::StringList{"CBCMMT256.rsp"_el, "ECBMMT128.rsp"_el, "ECBMMT256.rsp"_el};
    } else if (_source.family == "sha"_el || _source.family == "sha3"_el) {
        auto algorithms = el::StringList{"SHA1"_el, "SHA256"_el, "SHA384"_el, "SHA512"_el};
        inputPrefix = "shabytetestvectors/"_el;
        if (_source.family == "sha3"_el) {
            algorithms = el::StringList{"SHA3_256"_el, "SHA3_384"_el, "SHA3_512"_el};
            inputPrefix = {};
        }
        for (const auto &algorithm : algorithms) {
            for (const auto suffix : {"LongMsg.rsp"_el, "Monte.rsp"_el, "ShortMsg.rsp"_el}) {
                members.append(el::join(algorithm, suffix));
            }
        }
    } else if (_source.family == "gcm"_el) {
        members = el::StringList{
            "gcmDecrypt128.rsp"_el, "gcmDecrypt256.rsp"_el, "gcmEncryptExtIV128.rsp"_el, "gcmEncryptExtIV256.rsp"_el};
    } else if (_source.family == "hmac"_el) {
        members = el::StringList{"HMAC.rsp"_el};
    } else {
        members = el::StringList{"SigVer.rsp"_el};
        inputPrefix = "186-4ecdsatestvectors/"_el;
    }
    const auto archive = el::zip::ArchiveReader::create(data);
    for (const auto &member : members) {
        auto found = false;
        for (const auto &item : archive->items()) {
            if (item->path().toString() != el::join(inputPrefix, member)) {
                continue;
            }
            found = true;
            const auto text =
                normalize(el::StringDecoder{item->extract(el::ByteCount{134217728})}.decode(el::StringEncoding::Utf8));
            if (_source.family == "gcm"_el) {
                _outputs.set(el::join(outputPrefix, member), filterGcm(text));
            } else if (_source.family == "hmac"_el) {
                _outputs.set("hash/HMAC_SHA256.rsp"_el, filterHmac(text, 32));
                _outputs.set("hash/HMAC_SHA384.rsp"_el, filterHmac(text, 48));
            } else if (_source.family == "ecdsa"_el) {
                _outputs.set("signature/ECDSA_NIST_SIGVER.rsp"_el, filterEcdsa(text));
            } else {
                _outputs.set(el::join(outputPrefix, member), text);
            }
            break;
        }
        if (!found) {
            fail(el::join("Missing ZIP member: "_el, inputPrefix, member));
        }
    }
}

auto VectorConverter::filterGcm(const el::String &text) const -> el::String {
    auto comments = el::StringList{};
    auto sections = el::List<el::StringList>{};
    auto section = el::StringList{};
    auto key = el::String{};
    auto lines = el::StringSplitter{text, U'\n'};
    while (!lines.isAtEnd()) {
        const auto line = lines.next();
        if (line.startsWith("#"_el)) {
            comments.append(line);
        }
        if (line.startsWith("[Keylen = "_el)) {
            key = line;
        }
        if (line.startsWith("[IVlen = "_el)) {
            if (!section.isEmpty()) {
                sections.append(section);
            }
            section.clear();
        }
        if (!section.isEmpty() || line.startsWith("[IVlen = "_el)) {
            section.append(line);
        }
    }
    if (!section.isEmpty()) {
        sections.append(section);
    }
    if (key.isEmpty()) {
        fail("Missing GCM key length."_el);
    }
    auto output = comments;
    output.append(""_el);
    output.append(key);
    output.append(""_el);
    auto groups = 0;
    auto records = 0;
    for (const auto &candidate : sections) {
        if (!candidate.contains("[IVlen = 96]"_el) || !candidate.contains("[Taglen = 128]"_el)) {
            continue;
        }
        ++groups;
        for (const auto &line : candidate) {
            if (line.startsWith("Count = "_el)) {
                ++records;
            }
        }
        output.append(candidate);
        output.append(""_el);
    }
    if (groups != 25 || records != 375) {
        fail("Expected 25 GCM groups and 375 selected records."_el);
    }
    return el::join(output.join("\n"_el).trimmed(el::CharSet{U'\n'}, el::StringSide::Back), "\n"_el);
}

auto VectorConverter::filterHmac(const el::String &text, const int length) const -> el::String {
    auto comments = el::StringList{};
    auto selected = el::StringList{};
    auto active = false;
    auto records = 0;
    auto lines = el::StringSplitter{text, U'\n'};
    const auto marker = el::StringFormat{"[L={}]"_el}.build(length);
    while (!lines.isAtEnd()) {
        const auto line = lines.next();
        if (line.startsWith("#"_el)) {
            comments.append(line);
        }
        if (line.startsWith("[L="_el)) {
            active = line == marker;
        }
        if (active) {
            selected.append(line);
            if (line.startsWith("Count = "_el)) {
                ++records;
            }
        }
    }
    auto expected = 225;
    if (length == 48) {
        expected = 300;
    }
    if (records != expected) {
        fail("Unexpected selected HMAC record count."_el);
    }
    comments.append(""_el);
    comments.append(selected);
    return el::join(comments.join("\n"_el).trimmed(el::CharSet{U'\n'}, el::StringSide::Back), "\n"_el);
}

auto VectorConverter::derInteger(const el::String &value) const -> el::String {
    static const auto cHex = el::CharSet{"0123456789abcdefABCDEF"_el};
    if (value.isEmpty() || !value.containsOnly(cHex)) {
        fail("Malformed ECDSA integer."_el);
    }
    auto digits = value.trimmed(el::CharSet{U'0'}, el::StringSide::Front);
    if (digits.isEmpty()) {
        digits = "0"_el;
    }
    if (digits.length().toSizeT() % 2 != 0) {
        digits = el::join("0"_el, digits);
    }
    const auto first = digits.slice(el::StringSide::Front, el::ByteCount{1});
    static const auto cHighNibble = el::CharSet{"89abcdefABCDEF"_el};
    if (first.containsOnly(cHighNibble)) {
        digits = el::join("00"_el, digits);
    }
    const auto length = digits.length().toSizeT() / 2;
    if (length >= 128) {
        fail("ECDSA integer exceeds the supported DER length."_el);
    }
    return el::StringFormat{"02{:number:hex,precision=2,lc=lowercase}{}"_el}.build(length, digits);
}

auto VectorConverter::filterEcdsa(const el::String &text) const -> el::String {
    const auto header = el::StringList{
        "# NIST CAVP FIPS 186-4 ECDSA SigVer vectors"_el,
        "# Upstream archive: 186-4ecdsatestvectors.zip, member 186-4ecdsatestvectors/SigVer.rsp"_el,
        "# Modified by Erbsland Core: selected matched curves and encoded SPKI/signatures as canonical DER"_el};
    auto groups = el::render::ValueList{};
    auto section = el::String{};
    auto values = el::StringMap<el::String>{};
    auto count = 0;
    const auto finish = [&]() -> void {
        if (values.count().isZero()) {
            return;
        }
        if (values.count() != el::ItemCount{6}) {
            fail("Unexpected NIST ECDSA record fields."_el);
        }
        for (const auto name : {"Msg"_el, cQxKey, cQyKey, "R"_el, "S"_el, cResultKey}) {
            if (!values.contains(name)) {
                fail("Missing NIST ECDSA record field."_el);
            }
        }
        auto curve = el::String{"P-256"_el};
        auto sha = el::String{"SHA-256"_el};
        auto width = el::ByteCount{64};
        auto spki = el::String{"3059301306072a8648ce3d020106082a8648ce3d03010703420004"_el};
        if (section == "P-384,SHA-384"_el) {
            curve = "P-384"_el;
            sha = "SHA-384"_el;
            width = el::ByteCount{96};
            spki = "3076301006072a8648ce3d020106052b8104002203620004"_el;
        }
        if (values.get(cQxKey, el::String{}).length() != width || values.get(cQyKey, el::String{}).length() != width) {
            fail("Invalid ECDSA coordinate width."_el);
        }
        spki = el::join(spki, values.get(cQxKey, el::String{}), values.get(cQyKey, el::String{}));
        const auto contents =
            el::join(derInteger(values.get("R"_el, el::String{})), derInteger(values.get("S"_el, el::String{})));
        const auto signature = el::StringFormat{"30{:number:hex,precision=2,lc=lowercase}{}"_el}.build(
            contents.length().toSizeT() / 2, contents);
        auto result = el::String{"invalid"_el};
        if (values.get(cResultKey, el::String{}).startsWith("P "_el)) {
            result = "valid"_el;
        }
        groups.append(group(
            el::render::ValueList{
                field("Curve"_el, curve),
                field("Hash"_el, sha),
                field("PublicKeyDer"_el, spki),
                field("AlgorithmDer"_el, algorithmDer("ECDSA"_el, sha, 0))},
            el::render::ValueList{el::render::Value{el::render::ValueList{
                field("Count"_el, decimal(count++)),
                field(cResultKey, result),
                field("Flags"_el, values.get(cResultKey, el::String{})),
                field("Msg"_el, values.get("Msg"_el, el::String{})),
                field("Sig"_el, signature)}}}));
        values.clear();
    };
    auto lines = el::StringSplitter{text, U'\n'};
    while (!lines.isAtEnd()) {
        const auto line = lines.next().trimmed();
        if (line.isEmpty() || line.startsWith("#"_el)) {
            finish();
            continue;
        }
        if (line.startsWith("["_el) && line.endsWith("]"_el)) {
            finish();
            section = line.slice(el::ByteRange{el::ByteIndex{1}, line.length() - el::ByteCount{2}});
            continue;
        }
        if (section != "P-256,SHA-256"_el && section != "P-384,SHA-384"_el) {
            continue;
        }
        const auto separator = line.find("="_el);
        if (separator.isNoIndex()) {
            fail("Malformed NIST ECDSA line."_el);
        }
        const auto name = line.slice(el::StringSide::Front, separator).trimmed();
        if (values.contains(name)) {
            fail("Duplicate NIST ECDSA field."_el);
        }
        values.set(name, line.slice(el::StringSide::Back, separator + el::ByteCount{1}).trimmed());
    }
    finish();
    if (count != 30) {
        fail("Expected 30 selected NIST ECDSA records."_el);
    }
    return render(header, groups);
}

}
