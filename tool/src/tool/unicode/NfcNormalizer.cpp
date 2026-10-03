// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NfcNormalizer.hpp"

#include "UcdReader.hpp"
#include "UnicodeData.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/IntegerParseOptions.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/U8StringConstIterator.hpp>

#include <vector>

namespace tool::unicode {

using namespace el::text::literals;

NfcNormalizer::NfcNormalizer(const UcdDataset &dataset) : NfcNormalizer{dataset, UnicodeData{dataset}} {
}

NfcNormalizer::NfcNormalizer(const UcdDataset &dataset, const UnicodeData &unicodeData) :
    _source{dataset.sourcePath("UnicodeData.txt"_el).toString()} {
    const auto reader = UcdReader{dataset.text("UnicodeData.txt"_el), _source};
    for (const auto &record : unicodeData.records()) {
        readUnicodeRecord(record, reader);
    }
    validateGraph();
    readCompositions(dataset);
}

void NfcNormalizer::readUnicodeRecord(const UcdRecord &record, const UcdReader &reader) {
    try {
        const auto combining =
            record.fields.get(el::ItemIndex{3})
                .toIntegerOrThrow<unsigned>(el::IntegerParseOptions{}.setFixedBase(el::IntegerBase::Decimal));
        if (combining > 255U) {
            throwError("A combining class exceeds 255."_el, _source, record.lineIndex);
        }
        const auto mark = record.fields.get(el::ItemIndex{2}).startsWith("M"_el);
        for (auto value = record.first; value <= record.last; ++value) {
            if (combining != 0U) {
                _classes.set(value, combining);
            }
            if (mark) {
                _marks.insert(value);
            }
        }
        readDecomposition(record, reader);
    } catch (const el::ApplicationError &) {
        throw;
    } catch (const el::ParseError &) {
        throw el::ApplicationError{
            "Malformed Normalization Source"_el,
            "A UnicodeData field could not be parsed."_el,
            std::current_exception()}
            .withSourcePath(_source)
            .withCodeLocation(el::CodeLocation{record.lineIndex});
    }
}

void NfcNormalizer::readDecomposition(const UcdRecord &record, const UcdReader &reader) {
    auto fields = el::StringList{};
    auto split = el::StringSplitter{record.fields.get(el::ItemIndex{5}), U' '};
    while (!split.isAtEnd()) {
        const auto field = split.next();
        if (!field.isEmpty()) {
            fields.append(field);
        }
    }
    if (fields.isEmpty()) {
        return;
    }
    auto compatibility = false;
    auto mapping = el::List<char32_t>{};
    for (const auto &field : fields) {
        if (field.startsWith("<"_el)) {
            if (compatibility || !mapping.isEmpty() || !field.endsWith(">"_el)) {
                throwError("Malformed decomposition tag."_el, _source, record.lineIndex);
            }
            compatibility = true;
        } else {
            const auto scalar = reader.codePoint(field);
            if (scalar >= 0xD800U && scalar <= 0xDFFFU) {
                throwError("A decomposition contains a surrogate."_el, _source, record.lineIndex);
            }
            mapping.append(scalar);
        }
    }
    if (mapping.isEmpty() || record.first != record.last) {
        throwError("Empty decomposition or decomposition on a source range."_el, _source, record.lineIndex);
    }
    if (!compatibility) {
        _mappings.set(record.first, mapping);
        _locations.set(record.first, record.lineIndex);
    }
}

void NfcNormalizer::readCompositions(const UcdDataset &dataset) {
    auto exclusions = el::Set<char32_t>{};
    for (
        const auto &record :
        UcdReader{
            dataset.text("CompositionExclusions.txt"_el), dataset.sourcePath("CompositionExclusions.txt"_el).toString()}
            .parse(el::ItemCount{1}, true)) {
        for (auto value = record.first; value <= record.last; ++value) {
            exclusions.insert(value);
        }
    }
    for (const auto &[value, mapping] : _mappings) {
        if (mapping.count() != el::ItemCount{2} || exclusions.contains(value) || _classes.get(value, 0U) != 0U ||
            _classes.get(mapping.first(), 0U) != 0U) {
            continue;
        }
        const auto pair = std::pair{mapping.first(), mapping.last()};
        if (_compositions.contains(pair)) {
            throwError("Duplicate canonical composition pair."_el, _source);
        }
        _compositions.set(pair, value);
    }
}

void NfcNormalizer::validateGraph() const {
    auto completed = el::Set<char32_t>{};
    auto active = el::Set<char32_t>{};
    for (const auto &[root, mapping] : _mappings) {
        auto stack = std::vector<std::pair<char32_t, bool>>{{root, false}};
        while (!stack.empty()) {
            const auto [value, exiting] = stack.back();
            stack.pop_back();
            if (exiting) {
                active.remove(value);
                completed.insert(value);
                continue;
            }
            if (completed.contains(value) || !_mappings.contains(value)) {
                continue;
            }
            if (active.contains(value)) {
                throwError("Cyclic canonical decomposition mapping."_el, _source, _locations.get(value, {}));
            }
            active.insert(value);
            stack.emplace_back(value, true);
            for (const auto child : _mappings.get(value, {})) {
                stack.emplace_back(child, false);
            }
        }
    }
}

auto NfcNormalizer::decomposeHangul(const char32_t value) -> el::List<char32_t> {
    const auto index = static_cast<unsigned>(value) - 0xAC00U;
    if (index >= 11172U) {
        return {};
    }
    auto result = el::List<char32_t>{
        static_cast<char32_t>(0x1100U + index / 588U), static_cast<char32_t>(0x1161U + (index % 588U) / 28U)};
    if (index % 28U != 0U) {
        result.append(static_cast<char32_t>(0x11A7U + index % 28U));
    }
    return result;
}

auto NfcNormalizer::compose(const char32_t first, const char32_t second) const -> char32_t {
    if (first >= 0x1100U && first < 0x1113U && second >= 0x1161U && second < 0x1176U) {
        return 0xAC00U + ((first - 0x1100U) * 21U + second - 0x1161U) * 28U;
    }
    if (first >= 0xAC00U && first < 0xD7A4U && (first - 0xAC00U) % 28U == 0U && second > 0x11A7U && second < 0x11C3U) {
        return first + second - 0x11A7U;
    }
    return _compositions.get({first, second}, char32_t{});
}

auto NfcNormalizer::normalize(const el::String &text) const -> el::String {
    if (!text.isEncodingValid()) {
        throwError("Normalization input is not valid UTF-8."_el);
    }
    auto decomposed = std::vector<char32_t>{};
    for (const auto character : text) {
        decompose(character.toRawValue(), decomposed);
    }
    return composeSequence(decomposed);
}

void NfcNormalizer::decompose(const char32_t character, std::vector<char32_t> &decomposed) const {
    auto stack = std::vector<char32_t>{character};
    while (!stack.empty()) {
        const auto value = stack.back();
        stack.pop_back();
        auto mapping = decomposeHangul(value);
        if (mapping.isEmpty()) {
            mapping = _mappings.get(value, {});
        }
        if (!mapping.isEmpty()) {
            for (auto index = mapping.count().toSizeT(); index > 0U; --index) {
                stack.push_back(mapping.get(el::ItemIndex::fromSizeTOrThrow(index - 1U)));
            }
            continue;
        }
        appendOrdered(decomposed, value);
    }
}

void NfcNormalizer::appendOrdered(std::vector<char32_t> &decomposed, const char32_t value) const {
    auto position = decomposed.size();
    decomposed.push_back(value);
    const auto combining = _classes.get(value, 0U);
    if (combining != 0U) {
        while (position > 0U && _classes.get(decomposed[position - 1U], 0U) > combining) {
            decomposed[position] = decomposed[position - 1U];
            --position;
        }
        decomposed[position] = value;
    }
}

auto NfcNormalizer::composeSequence(const std::span<const char32_t> decomposed) const -> el::String {
    auto composed = std::vector<char32_t>{};
    auto starter = std::size_t{};
    auto hasStarter = false;
    auto lastClass = 0U;
    for (const auto value : decomposed) {
        const auto combining = _classes.get(value, 0U);
        auto composite = char32_t{};
        if (hasStarter && (lastClass == 0U || lastClass < combining)) {
            composite = compose(composed[starter], value);
        }
        if (composite != 0U) {
            composed[starter] = composite;
            continue;
        }
        if (combining == 0U) {
            starter = composed.size();
            hasStarter = true;
        }
        composed.push_back(value);
        lastClass = combining;
    }
    auto result = el::StringEditor{};
    for (const auto value : composed) {
        result.append(el::Char{value});
    }
    return result;
}

void NfcNormalizer::throwError(const el::String &description, const el::String &source, const el::LineIndex line) {
    throw el::ApplicationError{"Invalid Normalization Data"_el, description}.withSourcePath(source).withCodeLocation(
        el::CodeLocation{line});
}

}
