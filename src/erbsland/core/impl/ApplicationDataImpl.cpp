// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationDataImpl.hpp"

#include "application_data/ApplicationCryptologyData.hpp"
#include "application_data/ApplicationEventData.hpp"
#include "application_data/ApplicationLifecycleData.hpp"
#include "application_data/ApplicationLogData.hpp"
#include "application_data/ApplicationOptionsData.hpp"
#include "application_data/ApplicationPartsData.hpp"
#include "application_data/ApplicationRandomData.hpp"
#include "application_data/ApplicationResourceData.hpp"
#include "application_data/ApplicationRuntimeData.hpp"
#include "application_data/ApplicationSystemData.hpp"
#include "application_data/ApplicationTerminalData.hpp"

namespace erbsland::core::impl {

ApplicationDataImpl::ApplicationDataImpl() = default;

ApplicationDataImpl::~ApplicationDataImpl() = default;

auto ApplicationDataImpl::runtime() const noexcept -> const ApplicationRuntimeDataAccessor & {
    return _runtime;
}

auto ApplicationDataImpl::options() const noexcept -> const ApplicationOptionsDataAccessor & {
    return _options;
}

auto ApplicationDataImpl::lifecycle() const noexcept -> const ApplicationLifecycleDataAccessor & {
    return _lifecycle;
}

auto ApplicationDataImpl::parts() const noexcept -> const ApplicationPartsDataAccessor & {
    return _parts;
}

auto ApplicationDataImpl::events() const noexcept -> const ApplicationEventDataAccessor & {
    return _events;
}

auto ApplicationDataImpl::terminal() const noexcept -> const ApplicationTerminalDataAccessor & {
    return _terminal;
}

auto ApplicationDataImpl::logging() const noexcept -> const ApplicationLogDataAccessor & {
    return _logging;
}

auto ApplicationDataImpl::random() const noexcept -> const ApplicationRandomDataAccessor & {
    return _random;
}

auto ApplicationDataImpl::cryptology() const noexcept -> const ApplicationCryptologyDataAccessor & {
    return _cryptology;
}

auto ApplicationDataImpl::resources() const noexcept -> const ApplicationResourceDataAccessor & {
    return _resources;
}

auto ApplicationDataImpl::system() const noexcept -> const ApplicationSystemDataAccessor & {
    return _system;
}

}
