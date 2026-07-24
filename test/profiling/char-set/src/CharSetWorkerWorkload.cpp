// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CharSetWorkerWorkload.hpp"

namespace app::charset {

using namespace el::text::literals;

CharSetWorkerWorkload::CharSetWorkerWorkload(const Operation operation, CharSetFixture fixture) :
    _operation{operation}, _fixture{std::move(fixture)} {
}

auto CharSetWorkerWorkload::execute(const erbsland::profiling::WorkerExecutionContext &context)
    -> erbsland::profiling::WorkerMeasurement {
    switch (_operation) {
    case Operation::ConstructDefault:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            const auto value = el::CharSet{};
            consumeSet(sink, value);
        });
    case Operation::ConstructChar:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            const auto value = el::CharSet{_fixture.character};
            consumeSet(sink, value);
        });
    case Operation::ConstructU8String:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            const auto value = el::CharSet{_fixture.u8Text};
            consumeSet(sink, value);
        });
    case Operation::ConstructSet:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            const auto value = el::CharSet{_fixture.characterSet};
            consumeSet(sink, value);
        });
    case Operation::ConstructList:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            const auto value = el::CharSet{_fixture.characterList};
            consumeSet(sink, value);
        });
    case Operation::ConstructInitializerList:
        if (_fixture.caseName == "single"_el) {
            return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
                const auto value = el::CharSet{U'x'};
                consumeSet(sink, value);
            });
        }
        if (_fixture.caseName == "adjacent"_el) {
            return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
                const auto value = el::CharSet{U'a', U'b', U'c', U'd', U'e', U'f'};
                consumeSet(sink, value);
            });
        }
        if (_fixture.caseName == "unicode"_el) {
            return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
                const auto value = el::CharSet{U'A', U'é', U'β', U'中', el::Char{0x1F600U}};
                consumeSet(sink, value);
            });
        }
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            const auto value = el::CharSet{U'a', U'e', U'i', U'o', U'u'};
            consumeSet(sink, value);
        });
    case Operation::CopyConstruct:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            const auto value = _fixture.source;
            consumeSet(sink, value);
        });
    case Operation::MoveConstruct: {
        auto source = _fixture.source;
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            const auto value = std::move(source);
            consumeSet(sink, value);
        });
    }
    case Operation::CopyAssign: {
        auto value = el::CharSet{};
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            value = _fixture.source;
            consumeSet(sink, value);
        });
    }
    case Operation::MoveAssign: {
        auto source = _fixture.source;
        auto value = el::CharSet{};
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            value = std::move(source);
            consumeSet(sink, value);
        });
    }
    case Operation::Equal:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source == _fixture.other);
        });
    case Operation::NotEqual:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source != _fixture.other);
        });
    case Operation::SubsetOperator:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source <= _fixture.other);
        });
    case Operation::SupersetOperator:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source >= _fixture.other);
        });
    case Operation::IsEmpty:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source.isEmpty());
        });
    case Operation::Contains:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source.contains(_fixture.character));
        });
    case Operation::IsSubset:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source.isSubsetOf(_fixture.other));
        });
    case Operation::IsSuperset:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source.isSupersetOf(_fixture.other));
        });
    case Operation::IsEqualCi:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source.isEqualToCI(_fixture.other));
        });
    case Operation::IsSubsetCi:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source.isSubsetOfCI(_fixture.other));
        });
    case Operation::ContainsCaseFoldable:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source.containsCaseFoldableCharacters());
        });
    case Operation::ContainsLowercaseMappable:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source.containsLowercaseMappableCharacters());
        });
    case Operation::ContainsUppercaseMappable:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeBoolean(sink, _fixture.source.containsUppercaseMappableCharacters());
        });
    case Operation::UnitedWith:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source.unitedWith(_fixture.other));
        });
    case Operation::IntersectedWith:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source.intersectedWith(_fixture.other));
        });
    case Operation::SubtractedBy:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source.subtractedBy(_fixture.other));
        });
    case Operation::SymmetricDifferenceWith:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source.symmetricDifferenceWith(_fixture.other));
        });
    case Operation::UnionOperator:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source | _fixture.other);
        });
    case Operation::IntersectionOperator:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source & _fixture.other);
        });
    case Operation::SubtractionOperator:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source - _fixture.other);
        });
    case Operation::SymmetricDifferenceOperator:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source ^ _fixture.other);
        });
    case Operation::UnionAssign:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            auto value = _fixture.source;
            value |= _fixture.other;
            consumeSet(sink, value);
        });
    case Operation::IntersectionAssign:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            auto value = _fixture.source;
            value &= _fixture.other;
            consumeSet(sink, value);
        });
    case Operation::SubtractionAssign:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            auto value = _fixture.source;
            value -= _fixture.other;
            consumeSet(sink, value);
        });
    case Operation::SymmetricDifferenceAssign:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            auto value = _fixture.source;
            value ^= _fixture.other;
            consumeSet(sink, value);
        });
    case Operation::AddSet:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            auto value = _fixture.source;
            value.add(_fixture.other);
            consumeSet(sink, value);
        });
    case Operation::AddRange:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            auto value = _fixture.source;
            value.add(_fixture.range);
            consumeSet(sink, value);
        });
    case Operation::AddChar:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            auto value = _fixture.source;
            value.add(_fixture.character);
            consumeSet(sink, value);
        });
    case Operation::RemoveSet:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            auto value = _fixture.source;
            value.remove(_fixture.other);
            consumeSet(sink, value);
        });
    case Operation::RemoveRange:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            auto value = _fixture.source;
            value.remove(_fixture.range);
            consumeSet(sink, value);
        });
    case Operation::RemoveChar:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            auto value = _fixture.source;
            value.remove(_fixture.character);
            consumeSet(sink, value);
        });
    case Operation::ForEachRange:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            static_cast<void>(_fixture.source.forEach([&](const el::CharRange &range) -> void {
                consumeSize(sink, range.from().toRawValue());
                consumeSize(sink, range.to().toRawValue());
            }));
        });
    case Operation::ForEachChar:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            static_cast<void>(_fixture.source.forEach(
                [&](const el::Char character) -> void { consumeSize(sink, character.toRawValue()); }));
        });
    case Operation::Transform:
        if (_fixture.caseName == "collapse"_el) {
            return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
                consumeSet(sink, _fixture.source.transform([](const el::Char) noexcept -> el::Char { return U'x'; }));
            });
        }
        if (_fixture.caseName == "ascii-fold"_el || _fixture.caseName == "unicode"_el) {
            return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
                consumeSet(sink, _fixture.source.transform([](const el::Char character) noexcept -> el::Char {
                    return character.caseFolded();
                }));
            });
        }
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source.transform([](const el::Char character) noexcept -> el::Char {
                return character;
            }));
        });
    case Operation::CaseFolded:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source.caseFolded());
        });
    case Operation::ToLowercase:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source.toLowercase());
        });
    case Operation::ToUppercase:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, _fixture.source.toUppercase());
        });
    case Operation::ToString:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSize(sink, _fixture.source.toString().length().toRawValue());
        });
    case Operation::ToU8String:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSize(sink, _fixture.source.toU8String().length().toRawValue());
        });
    case Operation::ToU16String:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSize(sink, _fixture.source.toU16String().length().toRawValue());
        });
    case Operation::ToU32String:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSize(sink, _fixture.source.toU32String().length().toRawValue());
        });
    case Operation::ToSet:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSize(sink, _fixture.source.toSet().count().toRawValue());
        });
    case Operation::ToList:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSize(sink, _fixture.source.toList().count().toRawValue());
        });
    case Operation::FromRange:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, el::CharSet::fromRange(_fixture.range.from(), _fixture.range.to()));
        });
    case Operation::FromAsciiCategory:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, el::CharSet::from(_fixture.asciiCategory));
        });
    case Operation::FromUnicodeCategory:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, el::CharSet::from(_fixture.unicodeCategory));
        });
    case Operation::FromUnicodeGroup:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, el::CharSet::from(_fixture.unicodeGroup));
        });
    case Operation::FromPatternU8:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, el::CharSet::fromPattern(_fixture.u8Text));
        });
    case Operation::FromPatternU16:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, el::CharSet::fromPattern(_fixture.u16Text));
        });
    case Operation::FromPatternU32:
        return measure(context, [&](const std::uint64_t, std::uint64_t &sink) -> void {
            consumeSet(sink, el::CharSet::fromPattern(_fixture.u32Text));
        });
    }
    throw el::ApplicationError{"Unsupported character-set profiling operation."_el};
}

void CharSetWorkerWorkload::consumeSet(std::uint64_t &sink, const el::CharSet &value) noexcept {
    auto hasRange = false;
    value.forEach([&sink, &hasRange](const el::CharRange range) noexcept -> el::util::LoopStatus {
        hasRange = true;
        consumeSize(sink, range.from().toRawValue());
        return el::util::LoopStatus::Stop;
    });
    if (!hasRange) {
        consumeSize(sink, 0U);
    }
}

void CharSetWorkerWorkload::consumeBoolean(std::uint64_t &sink, const bool value) noexcept {
    consumeSize(sink, value ? 1U : 0U);
}

void CharSetWorkerWorkload::consumeSize(std::uint64_t &sink, const std::uint64_t value) noexcept {
    sink ^= value + 0x9e3779b97f4a7c15ULL + (sink << 6U) + (sink >> 2U);
}

auto CharSetWorkerWorkload::saturatedMultiply(const std::uint64_t first, const std::uint64_t second) noexcept
    -> std::uint64_t {
    if (second != 0U && first > std::numeric_limits<std::uint64_t>::max() / second) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return first * second;
}

}
