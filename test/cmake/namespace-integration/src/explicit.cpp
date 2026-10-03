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

static_assert(sizeof(erbsland::re::RegEx) > 0);
static_assert(sizeof(erbsland::network::TlsClientConnection) > 0);
static_assert(std::is_same_v<el::text::String, erbsland::text::String>);
static_assert(std::is_same_v<el::core::Application, erbsland::core::Application>);
static_assert(std::is_same_v<el::system::OperatingSystem, erbsland::system::OperatingSystem>);
static_assert(std::is_same_v<decltype(el::system::info::operatingSystem()), el::system::OperatingSystem>);

struct BindingRecipient {
    void receive(int);
};

static_assert(
    std::is_invocable_v<decltype(el::event::bindEvent(std::declval<el::event::EventsPtr>(), []() -> void {}))>);
static_assert(std::is_invocable_v<
    decltype(el::event::bindEvent(
        std::declval<el::event::EventsPtr>(),
        std::declval<std::shared_ptr<BindingRecipient>>(),
        &BindingRecipient::receive)),
    int>);
static_assert(std::is_invocable_v<
    decltype(el::event::bindWeakEvent(
        std::declval<el::event::EventsPtr>(),
        std::declval<std::weak_ptr<BindingRecipient>>(),
        &BindingRecipient::receive)),
    int>);
