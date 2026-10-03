// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DataSource.hpp"

#include <erbsland/LogicFailure.hpp>
#include <erbsland/String.hpp>

#include <utility>

namespace tool::frame {

using namespace el::text::literals;

auto DataSource::configurationRules() const -> el::conf::vr::RulesPtr {
    return nullptr;
}

void DataSource::configure([[maybe_unused]] const el::conf::ValuePtr &configuration) {
}

auto DataSource::log() const -> const el::LogStreamPtr & {
    if (!_log) {
        throw el::LogicFailure{"Data source logging has not been initialized."_el};
    }
    return _log;
}

void DataSource::setLog(el::LogStreamPtr log) {
    _log = std::move(log);
}

}
