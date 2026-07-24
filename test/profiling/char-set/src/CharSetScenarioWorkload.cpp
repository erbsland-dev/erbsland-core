// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CharSetScenarioWorkload.hpp"

#include "CharSetWorkerWorkload.hpp"
#include "OperationDescriptor.hpp"

#include <erbsland/profiling/SampleMeasurement.hpp>

#include <variant>

namespace app::charset {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

void CharSetScenarioWorkload::prepare([[maybe_unused]] const pf::RunConfiguration &run, const pf::Scenario &scenario) {
    auto caseName = el::String{"default"_el};
    auto trackAllocations = false;
    for (const auto &parameter : scenario.parameters) {
        if (parameter.parameter == "case"_el) {
            caseName = std::get<el::String>(parameter.value);
        } else if (parameter.parameter == "track-allocations"_el) {
            trackAllocations = std::get<bool>(parameter.value);
        }
    }
    auto isKnownCase = false;
    for (const auto &descriptor : operationDescriptors()) {
        if (descriptor.operation == _operation) {
            isKnownCase = descriptor.cases.contains(caseName);
            break;
        }
    }
    if (!isKnownCase) {
        throw el::ApplicationError{
            el::StringFormat{"Unsupported case '{}' for character-set functionality '{}'."_el}.build(
                caseName, scenario.functionality)};
    }
    _fixture = {};
    _fixture.caseName = caseName;
    _fixture.trackAllocations = trackAllocations;
    prepareFixture(caseName);
    updateCharacterCount();
}

auto CharSetScenarioWorkload::maximumOperations() const noexcept -> std::uint64_t {
    return 1'000'000U;
}

auto CharSetScenarioWorkload::createWorker([[maybe_unused]] const std::uint32_t worker) -> pf::WorkerWorkloadPtr {
    return std::make_shared<CharSetWorkerWorkload>(_operation, _fixture);
}

void CharSetScenarioWorkload::validate(const pf::SampleMeasurement &measurement) {
    if (measurement.operations == 0U) {
        throw el::ApplicationError{"The character-set workload completed no operations."_el};
    }
}

void CharSetScenarioWorkload::prepareFixture(const el::String &caseName) {
    switch (_operation) {
    case Operation::ConstructDefault:
    case Operation::ConstructChar:
    case Operation::ConstructU8String:
    case Operation::ConstructSet:
    case Operation::ConstructList:
    case Operation::ConstructInitializerList:
        prepareConstruction(caseName);
        break;
    case Operation::Equal:
    case Operation::NotEqual:
    case Operation::SubsetOperator:
    case Operation::SupersetOperator:
    case Operation::IsSubset:
    case Operation::IsSuperset:
        prepareRelation(caseName);
        break;
    case Operation::IsEqualCi:
    case Operation::IsSubsetCi:
        prepareCaseRelation(caseName);
        break;
    case Operation::Contains:
        prepareContains(caseName);
        break;
    case Operation::UnitedWith:
    case Operation::IntersectedWith:
    case Operation::SubtractedBy:
    case Operation::SymmetricDifferenceWith:
    case Operation::UnionOperator:
    case Operation::IntersectionOperator:
    case Operation::SubtractionOperator:
    case Operation::SymmetricDifferenceOperator:
    case Operation::UnionAssign:
    case Operation::IntersectionAssign:
    case Operation::SubtractionAssign:
    case Operation::SymmetricDifferenceAssign:
    case Operation::AddSet:
    case Operation::AddRange:
    case Operation::AddChar:
    case Operation::RemoveSet:
    case Operation::RemoveRange:
    case Operation::RemoveChar:
        prepareSetOperation(caseName);
        break;
    case Operation::ContainsCaseFoldable:
    case Operation::ContainsLowercaseMappable:
    case Operation::ContainsUppercaseMappable:
    case Operation::Transform:
    case Operation::CaseFolded:
    case Operation::ToLowercase:
    case Operation::ToUppercase:
        prepareMapping(caseName);
        break;
    case Operation::FromRange:
        prepareRangeFactory(caseName);
        break;
    case Operation::FromAsciiCategory:
        prepareAsciiCategory(caseName);
        break;
    case Operation::FromUnicodeCategory:
        prepareUnicodeCategory(caseName);
        break;
    case Operation::FromUnicodeGroup:
        prepareUnicodeGroup(caseName);
        break;
    case Operation::FromPatternU8:
    case Operation::FromPatternU16:
    case Operation::FromPatternU32:
        preparePattern(caseName);
        break;
    default:
        prepareSource(caseName);
        break;
    }
}

void CharSetScenarioWorkload::prepareConstruction(const el::String &caseName) {
    if (_operation == Operation::ConstructDefault) {
        return;
    }
    if (_operation == Operation::ConstructChar) {
        if (caseName == "supplementary"_el) {
            _fixture.character = el::Char{0x1F600U};
        } else if (caseName == "invalid"_el) {
            _fixture.character = el::Char{0x110000U};
        } else {
            _fixture.character = U'x';
        }
        return;
    }
    if (caseName == "single"_el) {
        setCharacterInputs("x"_el);
    } else if (caseName == "adjacent"_el) {
        setCharacterInputs("abcdef"_el);
    } else if (caseName == "unicode"_el) {
        setCharacterInputs(u8"Aéβ中😀"_el);
    } else {
        setCharacterInputs("aeiou"_el);
    }
}

void CharSetScenarioWorkload::prepareSource(const el::String &caseName) {
    if (caseName == "empty"_el) {
        _fixture.source = {};
    } else if (caseName == "single"_el) {
        _fixture.source = el::CharSet{U'x'};
    } else if (caseName == "range"_el) {
        _fixture.source = el::CharSet::fromRange(U'a', U'z');
    } else if (caseName == "unicode-small"_el) {
        _fixture.source = el::CharSet{u8"Aéβ中😀"_el};
    } else if (caseName == "unicode-category"_el) {
        _fixture.source = el::CharSet::from(el::UnicodeCategory::DecimalNumber);
    } else {
        _fixture.source = el::CharSet{"aeiou"_el};
    }
}

void CharSetScenarioWorkload::prepareContains(const el::String &caseName) {
    if (caseName.startsWith("single"_el)) {
        _fixture.source = el::CharSet{U'x'};
        _fixture.character = caseName == "single-hit"_el ? el::Char{U'x'} : el::Char{U'y'};
    } else if (caseName.startsWith("range"_el)) {
        _fixture.source = el::CharSet::fromRange(U'a', U'z');
        _fixture.character = caseName == "range-hit"_el ? el::Char{U'm'} : el::Char{U'0'};
    } else if (caseName.startsWith("unicode"_el)) {
        _fixture.source = el::CharSet::from(el::UnicodeCategoryGroup::Letter);
        _fixture.character = caseName == "unicode-hit"_el ? el::Char{U'β'} : el::Char{U'7'};
    } else {
        _fixture.source = el::CharSet{"aeiou"_el};
        if (caseName == "sparse-first"_el) {
            _fixture.character = U'a';
        } else if (caseName == "sparse-last"_el) {
            _fixture.character = U'u';
        } else {
            _fixture.character = U'z';
        }
    }
}

void CharSetScenarioWorkload::prepareRelation(const el::String &caseName) {
    if (caseName == "equal-single"_el) {
        _fixture.source = el::CharSet{U'x'};
        _fixture.other = _fixture.source;
    } else if (caseName == "equal-sparse"_el || caseName == "equal"_el) {
        _fixture.source = el::CharSet{"aeiou"_el};
        _fixture.other = _fixture.source;
    } else if (caseName == "different-early"_el) {
        _fixture.source = el::CharSet{"aeiou"_el};
        _fixture.other = el::CharSet{"beiou"_el};
    } else if (caseName == "different-late"_el) {
        _fixture.source = el::CharSet{"aeiou"_el};
        _fixture.other = el::CharSet{"aeioz"_el};
    } else if (caseName == "subset"_el) {
        _fixture.source = el::CharSet::fromRange(U'd', U'm');
        _fixture.other = el::CharSet::fromRange(U'a', U'z');
    } else if (caseName == "overlap"_el) {
        _fixture.source = el::CharSet::fromRange(U'a', U'm');
        _fixture.other = el::CharSet::fromRange(U'h', U'z');
    } else {
        _fixture.source = el::CharSet::fromRange(U'a', U'f');
        _fixture.other = el::CharSet::fromRange(U'x', U'z');
    }
}

void CharSetScenarioWorkload::prepareCaseRelation(const el::String &caseName) {
    if (caseName == "ascii-equal"_el) {
        _fixture.source = el::CharSet{"ABC"_el};
        _fixture.other = _fixture.source;
    } else if (caseName == "ascii-folded"_el || caseName == "ascii-subset"_el) {
        _fixture.source = el::CharSet{"ABC"_el};
        _fixture.other = caseName == "ascii-subset"_el ? el::CharSet{"abcdef"_el} : el::CharSet{"abc"_el};
    } else if (caseName == "unicode-folded"_el) {
        _fixture.source = el::CharSet{u8"ÄÖΣ"_el};
        _fixture.other = el::CharSet{u8"äöσ"_el};
    } else {
        _fixture.source = el::CharSet{"ABC"_el};
        _fixture.other = el::CharSet{"xyz"_el};
    }
}

void CharSetScenarioWorkload::prepareSetOperation(const el::String &caseName) {
    if (caseName == "merge"_el) {
        _fixture.source = el::CharSet::fromRange(U'a', U'f');
        _fixture.other = el::CharSet::fromRange(U'g', U'm');
        _fixture.range = el::CharRange{U'g', U'm'};
        _fixture.character = U'g';
    } else if (caseName == "overlap"_el) {
        _fixture.source = el::CharSet::fromRange(U'a', U'm');
        _fixture.other = el::CharSet::fromRange(U'h', U'z');
        _fixture.range = el::CharRange{U'h', U'z'};
    } else if (caseName == "equal"_el || caseName == "existing"_el) {
        _fixture.source = el::CharSet::fromRange(U'a', U'z');
        _fixture.other = _fixture.source;
        _fixture.range = el::CharRange{U'a', U'z'};
        _fixture.character = U'm';
    } else if (caseName == "split"_el) {
        _fixture.source = el::CharSet::fromRange(U'a', U'z');
        _fixture.other = el::CharSet{U'm'};
        _fixture.range = el::CharRange{U'm'};
        _fixture.character = U'm';
    } else if (caseName == "edge"_el) {
        _fixture.source = el::CharSet::fromRange(U'a', U'z');
        _fixture.other = el::CharSet{U'a'};
        _fixture.range = el::CharRange{U'a'};
        _fixture.character = U'a';
    } else if (caseName == "empty"_el) {
        _fixture.source = el::CharSet::fromRange(U'a', U'z');
        _fixture.other = {};
        _fixture.range = {};
    } else if (caseName == "invalid"_el) {
        _fixture.source = el::CharSet::fromRange(U'a', U'z');
        _fixture.character = el::Char{0x110000U};
    } else if (caseName == "unicode"_el) {
        _fixture.source = el::CharSet::from(el::UnicodeCategoryGroup::Letter);
        _fixture.other = el::CharSet::from(el::UnicodeCategoryGroup::Number);
        _fixture.range = el::CharRange{U'α', U'ω'};
        _fixture.character = U'β';
    } else {
        _fixture.source = el::CharSet::fromRange(U'a', U'f');
        _fixture.other = el::CharSet::fromRange(U'x', U'z');
        _fixture.range = el::CharRange{U'x', U'z'};
        _fixture.character = U'z';
    }
}

void CharSetScenarioWorkload::prepareMapping(const el::String &caseName) {
    if (caseName == "single-false"_el || caseName == "unchanged"_el || caseName == "identity"_el) {
        _fixture.source = el::CharSet{U'7'};
    } else if (caseName == "single-true"_el) {
        _fixture.source = el::CharSet{U'A'};
    } else if (caseName == "sparse-late"_el) {
        _fixture.source = el::CharSet{U'1', U'3', U'5', U'7', U'A'};
    } else if (caseName == "unicode-category"_el) {
        _fixture.source = el::CharSet::from(el::UnicodeCategory::UppercaseLetter);
    } else if (caseName == "ascii"_el || caseName == "ascii-fold"_el) {
        _fixture.source = el::CharSet{"AbCdEf"_el};
    } else if (caseName == "collapse"_el) {
        _fixture.source = el::CharSet{"AaBbCc"_el};
    } else {
        _fixture.source = el::CharSet{u8"ÄäÖöΣσς"_el};
    }
}

void CharSetScenarioWorkload::prepareRangeFactory(const el::String &caseName) {
    if (caseName == "single"_el) {
        _fixture.range = el::CharRange{U'x'};
    } else if (caseName == "reversed"_el) {
        _fixture.range = el::CharRange{U'z', U'a'};
    } else if (caseName == "supplementary"_el) {
        _fixture.range = el::CharRange{el::Char{0x1F600U}, el::Char{0x1F64FU}};
    } else if (caseName == "invalid"_el) {
        _fixture.range = el::CharRange{el::Char{0x110000U}, U'x'};
    } else {
        _fixture.range = el::CharRange{U'a', U'z'};
    }
    _fixture.source = el::CharSet::fromRange(_fixture.range.from(), _fixture.range.to());
}

void CharSetScenarioWorkload::prepareAsciiCategory(const el::String &caseName) {
    if (caseName == "whitespace"_el) {
        _fixture.asciiCategory = el::AsciiCategory::Whitespace;
    } else if (caseName == "control"_el) {
        _fixture.asciiCategory = el::AsciiCategory::Control;
    } else if (caseName == "punctuation"_el) {
        _fixture.asciiCategory = el::AsciiCategory::Punctuation;
    } else if (caseName == "alphanumeric"_el) {
        _fixture.asciiCategory = el::AsciiCategory::Alphanumeric;
    } else {
        _fixture.asciiCategory = el::AsciiCategory::Digit;
    }
    _fixture.source = el::CharSet::from(_fixture.asciiCategory);
}

void CharSetScenarioWorkload::prepareUnicodeCategory(const el::String &caseName) {
    if (caseName == "uppercase-letter"_el) {
        _fixture.unicodeCategory = el::UnicodeCategory::UppercaseLetter;
    } else if (caseName == "other-symbol"_el) {
        _fixture.unicodeCategory = el::UnicodeCategory::OtherSymbol;
    } else if (caseName == "control"_el) {
        _fixture.unicodeCategory = el::UnicodeCategory::Control;
    } else if (caseName == "unassigned"_el) {
        _fixture.unicodeCategory = el::UnicodeCategory::Unassigned;
    } else {
        _fixture.unicodeCategory = el::UnicodeCategory::DecimalNumber;
    }
    _fixture.source = el::CharSet::from(_fixture.unicodeCategory);
}

void CharSetScenarioWorkload::prepareUnicodeGroup(const el::String &caseName) {
    if (caseName == "number"_el) {
        _fixture.unicodeGroup = el::UnicodeCategoryGroup::Number;
    } else if (caseName == "punctuation"_el) {
        _fixture.unicodeGroup = el::UnicodeCategoryGroup::Punctuation;
    } else if (caseName == "symbol"_el) {
        _fixture.unicodeGroup = el::UnicodeCategoryGroup::Symbol;
    } else if (caseName == "other"_el) {
        _fixture.unicodeGroup = el::UnicodeCategoryGroup::Other;
    } else {
        _fixture.unicodeGroup = el::UnicodeCategoryGroup::Letter;
    }
    _fixture.source = el::CharSet::from(_fixture.unicodeGroup);
}

void CharSetScenarioWorkload::preparePattern(const el::String &caseName) {
    if (caseName == "single"_el) {
        _fixture.u8Text = "x"_el;
    } else if (caseName == "literals"_el) {
        _fixture.u8Text = "aeiou"_el;
    } else if (caseName == "ranges"_el) {
        _fixture.u8Text = "a-z"_el;
    } else if (caseName == "unicode"_el) {
        _fixture.u8Text = u8"α-ω😀"_el;
    } else {
        _fixture.u8Text = "-_a-zA-Z0-9"_el;
    }
    _fixture.u16Text = el::StringConverter{_fixture.u8Text}.toU16String();
    _fixture.u32Text = el::StringConverter{_fixture.u8Text}.toU32String();
    _fixture.source = el::CharSet::fromPattern(_fixture.u8Text);
}

void CharSetScenarioWorkload::setCharacterInputs(const el::U8String &text) {
    _fixture.u8Text = text;
    _fixture.characterSet = {};
    _fixture.characterList = {};
    for (const auto character : text) {
        _fixture.characterSet.insert(character);
        _fixture.characterList.append(character);
    }
    _fixture.source = el::CharSet{text};
}

void CharSetScenarioWorkload::updateCharacterCount() {
    _fixture.characterCount = 0U;
    _fixture.source.forEach([&](const el::Char) -> void { ++_fixture.characterCount; });
    if (_fixture.characterCount == 0U && _operation == Operation::ConstructChar &&
        _fixture.character.isValidUnicode()) {
        _fixture.characterCount = 1U;
    }
}

}
