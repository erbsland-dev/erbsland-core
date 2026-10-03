// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all_core.hpp>
#include <erbsland/BindEvent.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/String.hpp>
#include <erbsland/system/SystemInfo.hpp>
#include <erbsland/TlsClientConnection.hpp>

#include <memory>
#include <type_traits>
#include <utility>

static_assert(sizeof(el::RegEx) > 0);
static_assert(sizeof(el::TlsClientConnection) > 0);
static_assert(std::is_same_v<el::String, erbsland::text::String>);
static_assert(std::is_same_v<el::Application, erbsland::core::Application>);
static_assert(std::is_same_v<el::OperatingSystem, erbsland::system::OperatingSystem>);
static_assert(std::is_same_v<decltype(el::sys_info::operatingSystem()), el::OperatingSystem>);

struct BindingRecipient {
    void receive(int);
};

static_assert(std::is_invocable_v<decltype(el::bindEvent(std::declval<el::EventsPtr>(), []() -> void {}))>);
static_assert(std::is_invocable_v<
    decltype(el::bindEvent(
        std::declval<el::EventsPtr>(), std::declval<std::shared_ptr<BindingRecipient>>(), &BindingRecipient::receive)),
    int>);
static_assert(std::is_invocable_v<
    decltype(el::bindWeakEvent(
        std::declval<el::EventsPtr>(), std::declval<std::weak_ptr<BindingRecipient>>(), &BindingRecipient::receive)),
    int>);
