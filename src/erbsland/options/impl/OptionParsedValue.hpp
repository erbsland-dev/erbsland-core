// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Option_fwd.hpp"
#include "../OptionValueStorage.hpp"

#include "../../unit/ArgumentUnit.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace erbsland::options::impl {

class OptionParsedValue;
using OptionParsedValuePtr = std::shared_ptr<OptionParsedValue>;

class OptionParsedValue {
public:
    OptionParsedValue(OptionPtr option, OptionValueStorage storage, unit::ArgumentCount count) :
        option{std::move(option)}, storage{std::move(storage)}, count{count} {}
    OptionParsedValue(
        OptionPtr option,
        OptionValueStorage storage,
        unit::ArgumentCount count,
        std::vector<unit::ArgumentIndex> argumentIndexes) :
        option{std::move(option)},
        storage{std::move(storage)},
        count{count},
        argumentIndexes{std::move(argumentIndexes)} {}

    [[nodiscard]] static auto create(const OptionPtr &option, OptionValueStorage storage, unit::ArgumentCount count)
        -> OptionParsedValuePtr {
        return std::make_shared<OptionParsedValue>(option, std::move(storage), count);
    }
    [[nodiscard]] static auto create(
        const OptionPtr &option,
        OptionValueStorage storage,
        unit::ArgumentCount count,
        std::vector<unit::ArgumentIndex> argumentIndexes) -> OptionParsedValuePtr {
        return std::make_shared<OptionParsedValue>(option, std::move(storage), count, std::move(argumentIndexes));
    }

    OptionPtr option;
    OptionValueStorage storage;
    unit::ArgumentCount count{unit::ArgumentCount::zero()};
    std::vector<unit::ArgumentIndex> argumentIndexes;
};

}
