// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace erbsland::text::impl {

/// Iterative component-glob state shared by filesystem and configuration paths.
/// @tested{PathGlobPatternTest SyntaxFormatTest}
class ComponentGlobMatcher final {
public:
    /// Initialize a program, including zero-length recursive transitions.
    /// @tparam Program A component sequence with recursive flags.
    /// @param program Compiled component sequence.
    template <typename Program>
    void start(const Program &program) {
        _current.assign(program.size() + 1U, 0);
        _next.resize(_current.size());
        _current.front() = 1;
        skip(program);
    }
    /// Consume one candidate component.
    /// @tparam Program A sequence of components with matches(candidate).
    /// @tparam Candidate Candidate component type.
    /// @param program Compiled program.
    /// @param candidate Next candidate component.
    /// @param suffix Whether another match may begin at this boundary.
    template <typename Program, typename Candidate>
    void consume(const Program &program, const Candidate &candidate, bool suffix = false) {
        std::fill(_next.begin(), _next.end(), std::uint8_t{});
        _next.front() = suffix;
        for (std::size_t i = 0; i < program.size(); ++i) {
            if (!_current[i]) {
                continue;
            }
            if (program[i].recursive) {
                _next[i] = 1;
            } else if (program[i].matches(candidate)) {
                _next[i + 1U] = 1;
            }
        }
        _current.swap(_next);
        skip(program);
    }
    /// Test completion or strict descendant feasibility.
    /// @param descendant Whether at least one further component must remain possible.
    /// @return Whether the active states accept the requested operation.
    auto accepts(bool descendant = false) const -> bool {
        if (!descendant) {
            return _current.back() != 0;
        }
        return std::any_of(_current.begin(), _current.end() - 1, [](auto state) { return state != 0; });
    }

private:
    template <typename Program>
    /// Follow zero-component transitions through recursive stars.
    void skip(const Program &program) {
        for (std::size_t i = 0; i < program.size(); ++i) {
            if (_current[i] && program[i].recursive) {
                _current[i + 1U] = 1;
            }
        }
    }
    std::vector<std::uint8_t> _current; ///< Reachable component positions.
    std::vector<std::uint8_t> _next;    ///< Next transition state.
};

}
