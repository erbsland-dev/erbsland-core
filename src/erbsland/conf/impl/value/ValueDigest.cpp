// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueDigest.hpp"

#include "Value.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../re/RegEx.hpp"
#include "../../../text/AnyString.hpp"
#include "../../../text/impl/UnsafeU16StringAccess.hpp"
#include "../../../text/impl/UnsafeU32StringAccess.hpp"
#include "../../Document.hpp"

#include <vector>

namespace erbsland::conf::impl {

using namespace mem;
using namespace text;
using text::impl::UnsafeU16StringAccess;
using text::impl::UnsafeU32StringAccess;

auto ValueDigest::calculate(const Value &root) -> ByteBlock {
    nodeHeader(root, root.childrenImpl().size());
    root.updateDigestValue(*this);
    descendants(root.childrenImpl());
    return _hasher.finalize();
}

auto ValueDigest::calculate(const conf::Document &root, const std::span<const ValuePtr> children) -> ByteBlock {
    nodeHeader(root, children.size());
    descendants(children);
    return _hasher.finalize();
}

void ValueDigest::nodeHeader(const conf::Value &value, const std::size_t childCount) {
    native(static_cast<uint8_t>(value.type()));
    const auto name = value.name();
    native(static_cast<uint8_t>(name.type()));
    if (name.isIndex() || name.isTextIndex()) {
        native(name.asIndex());
    } else {
        update(name.asText());
    }
    native(childCount);
}

void ValueDigest::descendants(const std::span<const ValuePtr> children) {
    if (children.empty()) {
        return;
    }
    auto stack = std::vector<Frame>{};
    stack.reserve(32);
    stack.push_back(Frame{children});
    while (!stack.empty()) {
        auto &frame = stack.back();
        if (frame.next == frame.children.size()) {
            stack.pop_back();
            continue;
        }
        const auto &node = *frame.children[frame.next++];
        const auto &nested = node.childrenImpl();
        nodeHeader(node, nested.size());
        node.updateDigestValue(*this);
        if (!nested.empty()) {
            stack.push_back(Frame{nested});
        }
    }
}

void ValueDigest::bytes(const ConstByteSpan value) {
    native(value.size());
    _hasher.update(value);
}

void ValueDigest::update(const String &value) {
    native(value.length().toSizeT());
    _hasher.update(value);
}

void ValueDigest::update(const ByteBlock &value) {
    bytes(value.span());
}

void ValueDigest::update(const time::Date &value) {
    native(value.toDaysSinceEpoch().toRawValue());
}

void ValueDigest::timeParts(const time::Time &value) {
    native(value.toNanosecondsSinceMidnight().toRawValue());
}

void ValueDigest::update(const time::Time &value) {
    native(uint8_t{0});
    timeParts(value);
}

void ValueDigest::update(const time::TimeWithZone &value) {
    native(uint8_t{1});
    timeParts(value.time());
    timeZone(value.timeZone());
}

void ValueDigest::timeZone(const time::TimeZone &value) {
    native(value.isLocalTime());
    native(value.isNamed());
    if (value.isNamed()) {
        native(value.id().toRawValue());
    } else {
        native(value.staticOffset().toSeconds().toRawValue());
    }
}

void ValueDigest::update(const time::DateTime &value) {
    update(value.utcDate());
    timeParts(value.utcTime());
    native(value.timeOffset().toSeconds().toRawValue());
    timeZone(value.timeZone());
}

void ValueDigest::update(const time::CalendarDelta &value) {
    const auto parts = value.parts();
    native(parts.nanoseconds.toRawValue());
    native(parts.microseconds.toRawValue());
    native(parts.milliseconds.toRawValue());
    native(parts.seconds.toRawValue());
    native(parts.minutes.toRawValue());
    native(parts.hours.toRawValue());
    native(parts.days.toRawValue());
    native(parts.weeks.toRawValue());
    native(parts.months.toRawValue());
    native(parts.years.toRawValue());
}

void ValueDigest::update(const re::RegExPtr &value) {
    if (value == nullptr) {
        using namespace text::literals;
        throw err::ParameterFailure{"Cannot digest a null regular expression."_el, "value"_el};
    }
    native(value->flags().toRawValue());
    const auto &pattern = value->pattern();
    const auto kind = pattern.kind().value_or(StringKind::U8);
    native(static_cast<uint8_t>(kind));
    // Select the already stored encoding; these conversions only share the same-width storage.
    switch (kind) {
    case StringKind::U8:
        update(pattern.toU8String());
        break;
    case StringKind::U16: {
        const auto text = pattern.toU16String();
        bytes(toConstByteSpan(std::as_bytes(UnsafeU16StringAccess{text}.dataSpan())));
        break;
    }
    case StringKind::U32: {
        const auto text = pattern.toU32String();
        bytes(toConstByteSpan(std::as_bytes(UnsafeU32StringAccess{text}.dataView().dataSpan())));
        break;
    }
    }
}

}
