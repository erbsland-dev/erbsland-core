// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionParsedValue_fwd.hpp"

#include "../Option_fwd.hpp"
#include "../OptionValueStorage.hpp"

#include "../../unit/ArgumentUnit.hpp"

#include <utility>
#include <vector>

namespace erbsland::options::impl {

/// Mutable intermediate representation of a parsed option value.
class OptionParsedValue {
public:
    /// Create a parsed value with no recorded argument locations.
    OptionParsedValue(OptionPtr option, OptionValueStorage storage, unit::ArgumentCount count) :
        option{std::move(option)}, storage{std::move(storage)}, count{count} {}
    /// Create a parsed value with recorded argument locations.
    OptionParsedValue(
        OptionPtr option,
        OptionValueStorage storage,
        unit::ArgumentCount count,
        std::vector<unit::ArgumentIndex> argumentIndexes,
        const bool explicitFlagValue = false) :
        option{std::move(option)},
        storage{std::move(storage)},
        count{count},
        argumentIndexes{std::move(argumentIndexes)},
        explicitFlagValue{explicitFlagValue} {}

    /// Create shared parsed storage without argument locations.
    [[nodiscard]] static auto create(const OptionPtr &option, OptionValueStorage storage, unit::ArgumentCount count)
        -> OptionParsedValuePtr {
        return std::make_shared<OptionParsedValue>(option, std::move(storage), count);
    }
    /// Create shared parsed storage with argument locations.
    [[nodiscard]] static auto create(
        const OptionPtr &option,
        OptionValueStorage storage,
        unit::ArgumentCount count,
        std::vector<unit::ArgumentIndex> argumentIndexes,
        const bool explicitFlagValue = false) -> OptionParsedValuePtr {
        return std::make_shared<OptionParsedValue>(
            option, std::move(storage), count, std::move(argumentIndexes), explicitFlagValue);
    }

    OptionPtr option;
    OptionValueStorage storage;
    unit::ArgumentCount count{unit::ArgumentCount::zero()};
    std::vector<unit::ArgumentIndex> argumentIndexes;
    bool explicitFlagValue{false};
};

}
