// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Replacer.hpp"

#include "EnvironmentSource.hpp"
#include "ReplacerError.hpp"
#include "TextFilter.hpp"
#include "VariableSource.hpp"

#include "impl/Name.hpp"

#include "../CharSet.hpp"
#include "../Literals.hpp"
#include "../StringEditor.hpp"
#include "../StringFormat.hpp"
#include "../StringList.hpp"
#include "../StringSide.hpp"

#include "../../err/LogicError.hpp"
#include "../../unit/CpRange.hpp"

#include <memory>
#include <utility>

namespace erbsland::text::placeholder {

using namespace literals;

Replacer::Replacer(ReplacerOptions options) :
    _options{std::move(options)},
    _beginFirst{_options.frameBegin().charAt(StringSide::Front)},
    _endFirst{_options.frameEnd().charAt(StringSide::Front)},
    _filterFirst{_options.filterSeparator().charAt(StringSide::Front)},
    _nameFirst{_options.nameSeparator().charAt(StringSide::Front)},
    _endLength{_options.frameEnd().characterLength()},
    _filterLength{_options.filterSeparator().characterLength()},
    _nameLength{_options.nameSeparator().characterLength()},
    _stopSet{_options.escapeMode() == EscapeMode::Backslash ? CharSet{_beginFirst, U'\\'} : CharSet{_beginFirst}} {
    _options.validate();
}

auto Replacer::create(ReplacerOptions options) -> ReplacerPtr {
    return std::make_shared<Replacer>(std::move(options));
}

void Replacer::addSource(const SourcePtr &source) {
    _registry.addSource(source);
}

void Replacer::removeSource(const SourcePtr &source) noexcept {
    _registry.removeSource(source);
}

auto Replacer::source(const String &name) const -> SourcePtr {
    return _registry.source(impl::normalizeName(name));
}

void Replacer::addFilter(const FilterPtr &filter) {
    _registry.addFilter(filter);
}

void Replacer::removeFilter(const FilterPtr &filter) noexcept {
    _registry.removeFilter(filter);
}

auto Replacer::filter(const String &name) const -> FilterPtr {
    return _registry.filter(impl::normalizeName(name));
}

void Replacer::addEnvironmentSource(const String &name) {
    addSource(std::make_shared<EnvironmentSource>(name));
}

void Replacer::setVariableSource(StringMap<String> variables, const String &name) {
    const auto effectiveName = name.isEmpty() ? "var"_el : name;
    if (const auto current = source(effectiveName); current != nullptr) {
        const auto builtIn = std::dynamic_pointer_cast<VariableSource>(current);
        if (builtIn == nullptr) {
            throw err::LogicError{"The placeholder source name belongs to another provider."_el};
        }
        builtIn->setVariables(std::move(variables));
        return;
    }
    addSource(std::make_shared<VariableSource>(std::move(variables), effectiveName));
}

void Replacer::addTextFilters() {
    addFilter(std::make_shared<TextFilter>());
}

void Replacer::requireSources() const {
    if (!_registry.hasSources()) {
        throw err::LogicError{"At least one placeholder source must be registered."_el};
    }
}

auto Replacer::validate(const String &input) const -> bool {
    requireSources();
    try {
        [[maybe_unused]] const auto result = process(input, true, false);
        return true;
    } catch (const ReplacerError &) {
        return false;
    }
}

void Replacer::validateOrThrow(const String &input) const {
    requireSources();
    [[maybe_unused]] const auto result = process(input, true, false);
}

auto Replacer::replace(const String &input) const -> String {
    requireSources();
    return process(input, false, true);
}

auto Replacer::replaceOrThrow(const String &input) const -> String {
    requireSources();
    return process(input, false, false);
}

auto Replacer::matches(const StringCharReader &reader, const String &sequence) -> bool {
    if (sequence.isEmpty()) {
        return false;
    }
    auto probe = reader;
    return probe.advanceIf(sequence);
}

auto Replacer::advanceDoubledBegin(StringCharReader &reader) const -> bool {
    const auto state = reader.save();
    if (reader.advanceIf(_beginFirst) && reader.advanceIf(_options.frameBegin())) {
        return true;
    }
    reader.restore(state);
    return false;
}

auto Replacer::delimiterAt(const StringCharReader &reader, const bool isName) const -> Delimiter {
    auto result = Delimiter::None;
    auto bestLength = unit::CpLength::zero();
    const auto character = reader.peek();
    if (character == _endFirst && matches(reader, _options.frameEnd())) {
        result = Delimiter::End;
        bestLength = _endLength;
    }
    if (_filterLength > bestLength && character == _filterFirst && matches(reader, _options.filterSeparator())) {
        result = Delimiter::Filter;
        bestLength = _filterLength;
    }
    if (isName && _nameLength > bestLength && character == _nameFirst && matches(reader, _options.nameSeparator())) {
        result = Delimiter::Name;
    }
    return result;
}

auto Replacer::parseContent(StringCharReader &reader, const bool isName) const -> String {
    auto result = StringEditor{};
    while (true) {
        if (reader.isAtEnd()) {
            throw ReplacerError{ReplacerErrorCategory::UnexpectedEnd, "A placeholder is missing its closing frame."_el};
        }
        const auto character = reader.peek();
        if (character == U'\n' || character == U'\r') {
            throw ReplacerError{ReplacerErrorCategory::Syntax, "A placeholder cannot cross a physical line."_el};
        }
        if (_options.escapeMode() == EscapeMode::Backslash && character == U'\\') {
            reader.advance();
            const auto following = reader.peek();
            if (following == U'\\' || following == _endFirst ||
                (!_filterLength.isZero() && following == _filterFirst)) {
                result.append(reader.read());
            } else {
                result.append(U'\\');
            }
            continue;
        }
        if (_options.escapeMode() == EscapeMode::Double &&
            (character == _endFirst || (!_filterLength.isZero() && character == _filterFirst))) {
            auto probe = reader;
            probe.advance();
            if (probe.peek() == character) {
                reader.advance();
                reader.advance();
                result.append(character);
                continue;
            }
        }
        if (delimiterAt(reader, isName) != Delimiter::None) {
            return String{result};
        }
        if (character == _beginFirst && matches(reader, _options.frameBegin())) {
            throw ReplacerError{ReplacerErrorCategory::Syntax, "Nested placeholders are not allowed."_el};
        }
        result.append(reader.read());
    }
}

auto Replacer::parsePart(StringCharReader &reader) const -> Part {
    auto result = Part{};
    result.name = impl::normalizeName(parseContent(reader, true));
    if (delimiterAt(reader, true) == Delimiter::Name) {
        reader.advanceIf(_options.nameSeparator());
        result.parameter = parseContent(reader, false);
    }
    return result;
}

auto Replacer::parseExpression(StringCharReader &reader, const bool validation, bool &closed) const -> String {
    auto parts = std::vector<Part>{};
    parts.emplace_back(parsePart(reader));
    while (delimiterAt(reader, false) == Delimiter::Filter) {
        reader.advanceIf(_options.filterSeparator());
        if (parts.size() > 16U) {
            throw ReplacerError{
                ReplacerErrorCategory::LimitExceeded, "A placeholder can contain at most 16 filters."_el};
        }
        parts.emplace_back(parsePart(reader));
    }
    if (delimiterAt(reader, false) != Delimiter::End) {
        throw ReplacerError{ReplacerErrorCategory::Syntax, "Expected the placeholder closing frame."_el};
    }
    reader.advanceIf(_options.frameEnd());
    closed = true;

    const auto &sourcePart = parts.front();
    const auto source = _registry.source(sourcePart.name);
    if (source == nullptr) {
        throw ReplacerError{
            ReplacerErrorCategory::Unsupported,
            StringFormat{"Unknown placeholder source: {}"_el}.build(sourcePart.name)};
    }
    if (validation) {
        if (!source->validate(sourcePart.name, sourcePart.parameter)) {
            throw ReplacerError{
                ReplacerErrorCategory::Syntax,
                StringFormat{"Placeholder source '{}' rejected its parameter."_el}.build(sourcePart.name)};
        }
    }
    auto value = validation ? String{} : source->resolve(sourcePart.name, sourcePart.parameter);
    for (auto index = std::size_t{1U}; index < parts.size(); ++index) {
        const auto &part = parts[index];
        const auto filter = _registry.filter(part.name);
        if (filter == nullptr) {
            throw ReplacerError{
                ReplacerErrorCategory::Unsupported, StringFormat{"Unknown placeholder filter: {}"_el}.build(part.name)};
        }
        if (validation) {
            if (!filter->validate(part.name, part.parameter)) {
                throw ReplacerError{
                    ReplacerErrorCategory::Syntax,
                    StringFormat{"Placeholder filter '{}' rejected its parameter."_el}.build(part.name)};
            }
        } else {
            value = filter->apply(part.name, part.parameter, value);
        }
    }
    return value;
}

void Replacer::recover(StringCharReader &reader) const {
    while (!reader.isAtEnd() && reader.peek() != U'\n' && reader.peek() != U'\r') {
        if (reader.advanceIf(_options.frameEnd())) {
            return;
        }
        reader.advance();
    }
}

auto Replacer::process(const String &input, const bool validation, const bool tolerant) const -> String {
    auto reader = StringCharReader{input};
    auto pieces = StringList{};
    auto changed = false;
    reader.startCapture();
    while (!reader.isAtEnd()) {
        reader.advanceUntil(_stopSet);
        if (reader.isAtEnd()) {
            break;
        }
        if (_options.escapeMode() == EscapeMode::Backslash && reader.peek() == U'\\') {
            auto probe = reader;
            probe.advance();
            const auto doubled = probe.peek() == U'\\';
            const auto escapedBegin = !doubled && matches(probe, _options.frameBegin());
            if (doubled || escapedBegin) {
                if (!validation) {
                    pieces.append(reader.takeCapture().toString());
                }
                reader.advance();
                if (doubled) {
                    reader.advance();
                    if (!validation) {
                        pieces.append("\\"_el);
                    }
                } else {
                    reader.advanceIf(_options.frameBegin());
                    if (!validation) {
                        pieces.append(_options.frameBegin());
                    }
                }
                changed = true;
                reader.startCapture();
                continue;
            }
            reader.advance();
            continue;
        }
        if (_options.escapeMode() == EscapeMode::Double) {
            auto probe = reader;
            if (advanceDoubledBegin(probe)) {
                if (!validation) {
                    pieces.append(reader.takeCapture().toString());
                    pieces.append(_options.frameBegin());
                }
                reader = probe;
                changed = true;
                reader.startCapture();
                continue;
            }
        }
        if (!matches(reader, _options.frameBegin())) {
            reader.advance();
            continue;
        }
        const auto start = reader.position();
        if (!validation) {
            pieces.append(reader.takeCapture().toString());
        }
        reader.advanceIf(_options.frameBegin());
        auto closed = false;
        try {
            const auto value = parseExpression(reader, validation, closed);
            if (!validation) {
                pieces.append(value);
            }
            changed = true;
        } catch (const ReplacerError &error) {
            if (!tolerant) {
                throw error.offset().has_value() ? error : error.withOffset(start);
            }
            if (!closed) {
                recover(reader);
            }
            const auto end = reader.position();
            pieces.append(input.slice(unit::CpRange{start, end}));
        }
        reader.startCapture();
    }
    if (validation || !changed) {
        return input;
    }
    pieces.append(reader.takeCapture().toString());
    return pieces.join();
}

}
