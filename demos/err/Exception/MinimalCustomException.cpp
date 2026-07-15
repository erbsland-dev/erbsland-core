// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// A custom exception can be as small as a named type derived from the appropriate neutral branch.
/// Inheriting the base constructors preserves the standard reason and optional cause handling.
class InstrumentError final : public el::RuntimeError {
public:
    using RuntimeError::RuntimeError;
    ~InstrumentError() override = default;
};

/// A named exception lets callers recover specifically without parsing an error message.
void minimalCustomException() {
    try {
        throw InstrumentError{"The koto is missing one string."_el};
    } catch (const InstrumentError &error) {
        el::io::printLine("Instrument setup: "_el, error.reason());
    }
}

}
