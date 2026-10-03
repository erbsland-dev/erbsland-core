// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VectorConverter.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/base_n/BaseNDecoder.hpp>
#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/json/JsonParseOptions.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/StringDecoder.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/StringTransformations.hpp>

namespace tool::test_data::impl {

using namespace el::text::literals;

VectorConverter::VectorConverter(el::render::EnvironmentPtr renderer, VectorSource source, el::String revision) :
    _renderer{std::move(renderer)}, _source{std::move(source)}, _revision{std::move(revision)} {
}

auto VectorConverter::convert(const el::ByteBlock &data) -> el::StringMap<el::String> {
    _outputs.clear();
    if (_source.filename.endsWith(".zip"_el)) {
        convertArchive(data);
    } else {
        convertJson(data);
    }
    return std::move(_outputs);
}

auto VectorConverter::normalize(const el::String &text) -> el::String {
    const auto normalized = text.replacedAll("\r\n"_el, "\n"_el).replacedAll("\r"_el, "\n"_el);
    auto lines = el::StringSplitter{normalized, U'\n', el::StringSplitMode::KeepSeparator};
    auto output = el::StringEditor{};
    const auto whitespace = el::CharSet{U' ', U'\t'};
    while (!lines.isAtEnd()) {
        auto line = lines.next();
        if (line.endsWith("\n"_el)) {
            line = line.slice(el::StringSide::Front, line.length() - el::ByteCount{1});
            output.append(line.trimmed(whitespace, el::StringSide::Back));
            output.append("\n"_el);
        } else {
            output.append(line.trimmed(whitespace, el::StringSide::Back));
        }
    }
    return output;
}

auto VectorConverter::counts(const el::String &path, const el::String &text) -> std::pair<int64_t, int64_t> {
    auto recordPrefix = el::String{"Count = "_el};
    auto groupPrefix = el::String{"[L"_el};
    if (path.startsWith("aes/"_el)) {
        recordPrefix = "COUNT = "_el;
        groupPrefix = "[ENCRYPT]"_el;
    } else if (path.startsWith("sha/"_el) || path.startsWith("sha3/"_el)) {
        recordPrefix = "MD = "_el;
    } else if (path.startsWith("gcm/"_el) || path.startsWith("chacha20_poly1305/"_el)) {
        groupPrefix = "[IVlen = "_el;
    } else if (path.startsWith("x25519/"_el)) {
        groupPrefix = "[Curve = "_el;
    } else if (path.startsWith("signature/"_el)) {
        groupPrefix = "[PublicKeyDer = "_el;
    } else if (path.startsWith("x509/"_el)) {
        groupPrefix = "[SourceCommit = "_el;
    } else if (path.endsWith("BLAKE2b.rsp"_el)) {
        groupPrefix = "[Hash = "_el;
    }
    auto records = int64_t{};
    auto groups = int64_t{};
    auto lines = el::StringSplitter{normalize(text), U'\n'};
    while (!lines.isAtEnd()) {
        const auto line = lines.next();
        if (line.startsWith(recordPrefix)) {
            ++records;
        }
        if (line.startsWith(groupPrefix) || (path.startsWith("aes/"_el) && line == "[DECRYPT]"_el)) {
            ++groups;
        }
    }
    return {records, groups};
}

auto VectorConverter::field(const el::String &name, const el::String &value) -> el::render::Value {
    auto result = el::render::ValueMap{};
    result.set("name"_el, name);
    result.set("value"_el, value);
    return result;
}

auto VectorConverter::group(const el::render::ValueList &headers, const el::render::ValueList &records)
    -> el::render::Value {
    auto result = el::render::ValueMap{};
    result.set("headers"_el, headers);
    result.set("records"_el, records);
    return result;
}

auto VectorConverter::decimal(const int64_t value) -> el::String {
    return el::StringFormat{"{}"_el}.build(value);
}

auto VectorConverter::render(const el::StringList &header, const el::render::ValueList &groups) const -> el::String {
    auto context = el::render::Context{};
    context.set("header"_el, header);
    context.set("groups"_el, groups);
    return el::join(
        _renderer->render("cryptology_vectors/response.tpl"_el, context)
            .trimmed(el::CharSet{U'\n'}, el::StringSide::Back),
        "\n"_el);
}

void VectorConverter::fail(const el::String &description) const {
    throw el::ApplicationError{"Invalid Cryptology Vector Source"_el, description}.withSourcePath(_source.filename);
}

auto VectorConverter::text(const el::json::JsonValue &object, const el::String &key) const -> el::String {
    const auto value = object.get(key).getText();
    if (!value) {
        fail(el::StringFormat{"Expected a text field named '{}'."_el}.build(key));
    }
    return *value;
}

auto VectorConverter::integer(const el::json::JsonValue &object, const el::String &key) const -> int64_t {
    const auto value = object.get(key).get<int64_t>();
    if (!value) {
        fail(el::StringFormat{"Expected an integer field named '{}'."_el}.build(key));
    }
    return *value;
}

auto VectorConverter::array(const el::json::JsonValue &value) const -> el::json::JsonArray {
    const auto result = value.get<el::json::JsonArray>();
    if (!result) {
        fail("Expected a JSON array."_el);
    }
    return *result;
}

auto VectorConverter::flags(const el::json::JsonValue &test) const -> el::String {
    auto result = el::StringList{};
    for (const auto &value : array(test.get("flags"_el))) {
        const auto flag = value.getText();
        if (!flag) {
            fail("A test flag must be text."_el);
        }
        result.append(*flag);
    }
    return result.join(","_el);
}

auto VectorConverter::signatureRecord(const el::json::JsonValue &test) const -> el::render::Value {
    const auto result = text(test, "result"_el);
    if (result != "valid"_el && result != "invalid"_el && result != "acceptable"_el) {
        fail("Unexpected signature result."_el);
    }
    return el::render::ValueList{
        field("Count"_el, decimal(integer(test, "tcId"_el))),
        field("Result"_el, result),
        field("Flags"_el, flags(test)),
        field("Msg"_el, text(test, "msg"_el)),
        field("Sig"_el, text(test, "sig"_el))};
}

void VectorConverter::convertJson(const el::ByteBlock &data) {
    auto options = el::json::JsonParseOptions{};
    options.setMaximumInputLength(el::ByteCount{134217728}).setMaximumValueCount(el::ItemCount{4000000});
    const auto root =
        el::json::JsonValue::fromStringOrThrow(el::StringDecoder{data}.decode(el::StringEncoding::Utf8), options);
    if (_source.family == "blake2b"_el) {
        convertBlake(root);
    } else if (_source.family == "x509-limbo"_el) {
        convertLimbo(root);
    } else {
        convertWycheproof(root);
    }
}

auto VectorConverter::certificate(const el::String &pem) const -> el::String {
    auto lines = el::StringList::fromSplit(normalize(pem).trimmed(), el::CharSet{U'\n'});
    if (lines.count() < el::ItemCount{3} || lines.first() != "-----BEGIN CERTIFICATE-----"_el ||
        lines.last() != "-----END CERTIFICATE-----"_el) {
        fail("Unexpected certificate PEM envelope."_el);
    }
    lines.removeFirst();
    lines.removeLast();
    auto format = el::base_n::BaseNFormat::base64();
    format.setWhitespace(el::CharSet{});
    const auto bytes = el::base_n::BaseNDecoder{lines.join(), format}.toDataOrThrow();
    return el::toAsciiLowercase(el::String::fromByteBlock(bytes));
}

}
