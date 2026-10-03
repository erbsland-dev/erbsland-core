// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ProfileTypes.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StringSide.hpp>

#include <algorithm>
#include <optional>
#include <utility>

namespace app::string::impl {

/// Prepare and validate equivalent native and public-API separator splitting workloads.
/// @tparam tTraits The string width traits.
/// @tparam tValue The read-only or editor result type.
template <typename tTraits, typename tValue>
class SeparatorSplitWorkload final {
private:
    using String = typename tTraits::String;
    using Editor = typename tTraits::Editor;
    using Index = typename tTraits::DataIndex;
    using Result = std::optional<std::pair<tValue, tValue>>;

public:
    /// Prepare the separator and source outside the measured loop.
    /// @param source The deterministic source shared by paired scenarios.
    /// @param scenario The selected direction, separator kind, and input size.
    SeparatorSplitWorkload(const String &source, const Scenario &scenario) {
        using namespace el::text::literals;
        _manual = scenario.variant.startsWith("manual-"_el);
        _last = scenario.variant.contains("last-"_el);
        _set = scenario.variant.contains("-set"_el);
        const auto miss = scenario.variant.endsWith("-miss"_el);
        auto separator = el::Char{U'/'};
        if (scenario.contentProfile != ContentProfile::Ascii) {
            separator = el::Char{U'🧭'}; // Outside every generated content profile's alphabet.
        }
        _separators = el::CharSet{separator};
        auto count = std::uint64_t{1U};
        if (!_set) {
            count = std::max<std::uint64_t>(1U, scenario.operandSize);
        }
        _needle = String::fromCharacter(separator, el::CpCount::fromSizeT(static_cast<std::size_t>(count)));
        auto editor = Editor{source};
        if (!miss && source.characterLength().toSizeT() >= count) {
            const auto start = (source.characterLength().toSizeT() - count) / 2U;
            editor.replace(
                el::CpRange{el::CpIndex::fromSizeT(start), el::CpCount::fromSizeT(static_cast<std::size_t>(count))},
                _needle);
        }
        _value = tValue{editor};
        const auto native = splitNative();
        const auto manual = splitManual();
        if (native.has_value() != manual.has_value() ||
            (native &&
                (native->first.compareBinary(manual->first) != 0 ||
                    native->second.compareBinary(manual->second) != 0))) {
            throw el::ApplicationError{"Native and manual separator splitting disagree."_el};
        }
    }

public: // measurement
    /// Execute the selected operation and expose both output lengths to the result sink.
    /// @return A checksum that distinguishes misses and both result boundaries.
    [[nodiscard]] auto run() const -> std::uint64_t {
        auto result = Result{};
        if (_manual) {
            result = splitManual();
        } else {
            result = splitNative();
        }
        if (!result) {
            return 0U;
        }
        return 1U + tTraits::dataLength(result->first) * 65537U + tTraits::dataLength(result->second);
    }

private:
    /// Split using the native boundary-returning search.
    /// @return Shared halves, or no value when the separator is absent.
    [[nodiscard]] auto splitNative() const -> Result {
        if (_set) {
            if (_last) {
                return _value.splitAtLast(_separators);
            }
            return _value.splitAtFirst(_separators);
        }
        if (_last) {
            return _value.splitAtLast(_needle);
        }
        return _value.splitAtFirst(_needle);
    }

    /// Split using the equivalent sequence of public search and slice calls.
    /// @return Shared halves, or no value when the separator is absent.
    [[nodiscard]] auto splitManual() const -> Result {
        auto begin = Index::noIndex();
        if (_set) {
            if (_last) {
                begin = _value.findLastOf(_separators);
            } else {
                begin = _value.findFirstOf(_separators);
            }
        } else {
            begin = _value.find(_needle);
            if (_last && !begin.isNoIndex()) {
                auto position = begin;
                while (_value.advance(position)) {
                    const auto next = _value.find(_needle, position);
                    if (next.isNoIndex()) {
                        break;
                    }
                    begin = next;
                    position = next;
                }
            }
        }
        if (begin.isNoIndex()) {
            return std::nullopt;
        }
        // Fixtures use exact native separators, so their native length is also the match length.
        auto end = begin + _needle.length();
        if (_set) {
            end = begin;
            _value.advance(end);
        }
        return std::pair{_value.slice(el::StringSide::Front, begin), _value.slice(el::StringSide::Back, end)};
    }

private:
    tValue _value;           ///< The prepared source value.
    String _needle;          ///< The exact separator text.
    el::CharSet _separators; ///< The single-character separator set.
    bool _manual{};          ///< Use the public-API baseline.
    bool _last{};            ///< Select the rightmost occurrence.
    bool _set{};             ///< Use character-set membership.
};

}
