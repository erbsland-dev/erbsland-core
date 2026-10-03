// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpRoutes.hpp"

#include "../../../../err/ParameterFailure.hpp"
#include "../../../../text/AnyString.hpp"
#include "../../../../text/AsciiCategory.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/NormalizationForm.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../../../text/StringEditor.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../http/HttpMethodType.hpp"

#include <algorithm>

namespace erbsland::network::impl {

using namespace err;
using namespace unit;
using namespace text;
using namespace text::literals;

void HttpRoutes::add(String pattern, HttpRouteHandler handler) {
    validateHandler(handler);
    _entries.emplace_back(parsePattern(std::move(pattern)), std::nullopt, std::move(handler));
}

void HttpRoutes::add(HttpMethod method, String pattern, HttpRouteHandler handler) {
    validateHandler(handler);
    if (!method.isValid()) {
        throw ParameterFailure{"An HTTP route requires a valid method and callback."_el, "method"_el};
    }
    auto methods = std::vector<HttpMethod>{};
    methods.emplace_back(std::move(method));
    _entries.emplace_back(parsePattern(std::move(pattern)), std::move(methods), std::move(handler));
}

void HttpRoutes::add(const HttpMethodTypes methods, String pattern, HttpRouteHandler handler) {
    validateHandler(handler);
    auto exactMethods = methodsFromFlags(methods);
    if (exactMethods.empty()) {
        throw ParameterFailure{"An HTTP route requires at least one method and a callback."_el, "methods"_el};
    }
    _entries.emplace_back(parsePattern(std::move(pattern)), std::move(exactMethods), std::move(handler));
}

void HttpRoutes::addFallback(HttpRouteHandler handler) {
    validateHandler(handler);
    if (handler.kind() != HttpRouteHandler::Kind::Head) {
        throw ParameterFailure{"An HTTP fallback must be a request-head handler."_el, "callback"_el};
    }
    _entries.emplace_back(std::nullopt, std::nullopt, std::move(handler));
}

auto HttpRoutes::match(const HttpMethod &method, const StringList &segments) const -> Match {
    auto result = Match{};
    const auto findHandler = [&](const bool getFallback) -> bool {
        for (const auto &entry : _entries) {
            if (!entry.pattern.has_value()) {
                continue;
            }
            auto parameters = Parameters{};
            if (!pathMatches(entry, segments, parameters)) {
                continue;
            }
            result.pathMatched = true;
            appendAllowed(entry, result.allowed);
            if (methodMatches(entry, method, getFallback)) {
                result.handler = entry.handler;
                result.parameters = std::move(parameters);
                result.usedHeadFallback = getFallback;
                return true;
            }
        }
        return false;
    };
    if (findHandler(false)) {
        return result;
    }
    if (method == HttpMethod{HttpMethodType::Head} && findHandler(true)) {
        return result;
    }
    for (const auto &entry : _entries) {
        if (!entry.pattern.has_value()) {
            result.handler = entry.handler;
            return result;
        }
    }
    return result;
}

void HttpRoutes::validateHandler(const HttpRouteHandler &handler) {
    if (!handler.isValid()) {
        throw ParameterFailure{"An HTTP route requires a callback."_el, "callback"_el};
    }
    if (handler.kind() == HttpRouteHandler::Kind::Head) {
        return;
    }
    const auto maximum = handler.options().maximumBodyLength();
    if (maximum.isZero() || maximum.isInfinite()) {
        throw ParameterFailure{"An automatic HTTP route requires a positive finite body limit."_el, "options"_el};
    }
    if (const auto &patterns = handler.options().acceptedContentTypes(); patterns.has_value()) {
        for (const auto &pattern : *patterns) {
            validateContentTypePattern(pattern);
        }
    }
}

void HttpRoutes::validateContentTypePattern(const String &pattern) {
    if (!pattern.isEncodingValid()) {
        throw ParameterFailure{"An HTTP Content-Type pattern must be valid ASCII."_el, "acceptedContentTypes"_el};
    }
    auto reader = StringCharReader{pattern};
    reader.startCapture();
    reader.advanceWhile(AsciiCategory::HttpToken);
    const auto type = reader.takeCapture().toString();
    if (type.isEmpty() || !reader.advanceIf(U'/')) {
        throw ParameterFailure{
            "An HTTP Content-Type pattern requires a type and subtype."_el, "acceptedContentTypes"_el};
    }
    if (type == "*"_el) {
        if (!reader.advanceIf(U'*') || !reader.isAtEnd()) {
            throw ParameterFailure{"The wildcard media type must be */*."_el, "acceptedContentTypes"_el};
        }
        return;
    }
    if (reader.advanceIf(U'*')) {
        if (reader.isAtEnd()) {
            return;
        }
        if (!reader.advanceIf(U'+')) {
            throw ParameterFailure{"A media suffix pattern must use *+suffix."_el, "acceptedContentTypes"_el};
        }
    }
    reader.startCapture();
    reader.advanceWhile(AsciiCategory::HttpToken);
    const auto subtype = reader.takeCapture().toString();
    if (subtype.isEmpty() || !reader.isAtEnd()) {
        throw ParameterFailure{"An HTTP Content-Type pattern has an invalid subtype."_el, "acceptedContentTypes"_el};
    }
}

auto HttpRoutes::parsePattern(String pattern) -> std::vector<Segment> {
    if (!pattern.isEncodingValid() || !pattern.startsWith("/"_el) || pattern.contains("?"_el) ||
        pattern.contains("#"_el)) {
        throw ParameterFailure{"An HTTP route pattern must be an absolute UTF-8 path."_el, "pattern"_el};
    }
    pattern = pattern.normalized(NormalizationForm::Nfc);
    auto result = std::vector<Segment>{};
    const auto parts = splitPath(pattern);
    for (auto index = ItemIndex{0U}; index.isWithin(parts.count()); index.uncheckedIncrement()) {
        const auto &part = parts.getRef(index);
        auto kind = SegmentKind::Literal;
        auto text = part;
        if (part.length() >= ByteCount{3U} && part.startsWith("{"_el) && part.endsWith("}"_el)) {
            text = part.slice(ByteRange{ByteIndex{1U}, part.length() - ByteCount{2U}});
            if (text.startsWith("*"_el)) {
                kind = SegmentKind::CatchAll;
                text = text.slice(StringSide::Back, ByteIndex{1U});
            } else {
                kind = SegmentKind::Parameter;
            }
            if (text.isEmpty() || !text.charAt(StringSide::Front).isAsciiLetter()) {
                throw ParameterFailure{"An HTTP route parameter has an invalid name."_el, "pattern"_el};
            }
            if (!text.containsOnly(AsciiCategory::Word)) {
                throw ParameterFailure{"An HTTP route parameter has an invalid name."_el, "pattern"_el};
            }
            if (kind == SegmentKind::CatchAll && index.incremented() != ItemIndex::end(parts.count())) {
                throw ParameterFailure{"An HTTP catch-all parameter must be the final segment."_el, "pattern"_el};
            }
            for (const auto &existing : result) {
                if (existing.kind != SegmentKind::Literal && existing.text == text) {
                    throw ParameterFailure{"An HTTP route contains a duplicate parameter name."_el, "pattern"_el};
                }
            }
        }
        result.emplace_back(kind, std::move(text));
    }
    return result;
}

auto HttpRoutes::splitPath(const String &path) -> StringList {
    auto result = StringList{};
    auto reader = StringCharReader{path};
    if (reader.advanceIf(U'/') && reader.isAtEnd()) {
        return result;
    }
    auto segment = StringEditor{};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == U'/') {
            result.append(segment);
            segment.clear();
        } else {
            segment.append(character);
        }
    }
    result.append(segment);
    return result;
}

auto HttpRoutes::methodsFromFlags(const HttpMethodTypes methods) -> std::vector<HttpMethod> {
    static constexpr auto cMethods = std::array{
        HttpMethodType::Connect,
        HttpMethodType::Delete,
        HttpMethodType::Get,
        HttpMethodType::Head,
        HttpMethodType::Options,
        HttpMethodType::Patch,
        HttpMethodType::Post,
        HttpMethodType::Put,
        HttpMethodType::Trace,
    };
    auto result = std::vector<HttpMethod>{};
    for (const auto method : cMethods) {
        if (methods.contains(method)) {
            result.emplace_back(method);
        }
    }
    return result;
}

auto HttpRoutes::methodMatches(const Entry &entry, const HttpMethod &method, const bool getFallback) -> bool {
    if (!entry.methods.has_value()) {
        return !getFallback;
    }
    const auto expected = getFallback ? HttpMethod{HttpMethodType::Get} : method;
    return std::ranges::find(*entry.methods, expected) != entry.methods->end();
}

auto HttpRoutes::pathMatches(const Entry &entry, const StringList &segments, Parameters &parameters) -> bool {
    if (!entry.pattern.has_value()) {
        return true;
    }
    const auto &pattern = *entry.pattern;
    auto segmentIndex = ItemIndex{};
    for (const auto &part : pattern) {
        if (part.kind == SegmentKind::CatchAll) {
            auto value = StringEditor{};
            while (segmentIndex.isWithin(segments.count())) {
                if (!value.isEmpty()) {
                    value.append(U'/');
                }
                value.append(segments.getRef(segmentIndex));
                segmentIndex.uncheckedIncrement();
            }
            parameters.emplace_back(part.text, value);
            return true;
        }
        if (segmentIndex >= ItemIndex::end(segments.count())) {
            return false;
        }
        if (part.kind == SegmentKind::Literal && part.text != segments.getRef(segmentIndex)) {
            return false;
        }
        if (part.kind == SegmentKind::Parameter) {
            if (segments.getRef(segmentIndex).isEmpty()) {
                return false;
            }
            parameters.emplace_back(part.text, segments.getRef(segmentIndex));
        }
        ++segmentIndex;
    }
    return segmentIndex == ItemIndex::end(segments.count());
}

void HttpRoutes::appendAllowed(const Entry &entry, std::vector<HttpMethod> &allowed) {
    if (!entry.methods.has_value()) {
        return;
    }
    for (const auto &method : *entry.methods) {
        if (std::ranges::find(allowed, method) == allowed.end()) {
            allowed.push_back(method);
        }
        if (method == HttpMethod{HttpMethodType::Get}) {
            const auto head = HttpMethod{HttpMethodType::Head};
            if (std::ranges::find(allowed, head) == allowed.end()) {
                allowed.push_back(head);
            }
        }
    }
}

}
