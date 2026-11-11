// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/LogicFailure.hpp>
#include <erbsland/LogStream.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringFormat.hpp>

#include <cstdint>
#include <utility>

namespace demo {

using namespace el::text::literals;

/// The workflow state, accessed only by callbacks on the main event loop.
class State {
public:
    /// The actual state value.
    enum Value : std::uint8_t {
        Initialized, ///< Initialized but not started yet.
        Scanning,    ///< Started the workflow.
        Packing,     ///< Running the compression workflow.
        Failed,      ///< The workflow failed and can't continue.
        Done,        ///< Successfully completed the workflow.
    };

public:
    /// Create a new state instance.
    /// @param logStream The log stream to write state changes.
    explicit State(el::LogStreamPtr logStream) noexcept : _logStream{std::move(logStream)} {}

public: // transitions
    /// Require a given state and change it.
    void requireAndSet(const Value requiredState, const Value newState) {
        if (_value != requiredState) {
            throw el::LogicFailure{el::StringFormat{"Expected state {} but encountered state {}."_el}.build(
                stateToString(requiredState), stateToString(_value))};
        }
        set(newState);
    }
    /// Change the state.
    void set(const Value newState) {
        if (_value != newState) {
            _logStream->trace("State changed from "_el, stateToString(_value), " to "_el, stateToString(newState));
            _value = newState;
        }
    }
    /// Test the state.
    [[nodiscard]] auto is(Value value) const noexcept -> bool { return value == _value; }

private:
    /// Get a string from a state enum.
    [[nodiscard]] static auto stateToString(const Value state) -> el::String {
        switch (state) {
        case Initialized:
            return "Initialized"_el;
        case Scanning:
            return "Scanning"_el;
        case Packing:
            return "Packing"_el;
        case Failed:
            return "Failed"_el;
        case Done:
            return "Done"_el;
        default:
            break;
        }
        return {};
    }

private:
    const el::LogStreamPtr _logStream; ///< The log stream to write output to during the operation.
    Value _value = Initialized;        ///< The actual state value.
};

}
