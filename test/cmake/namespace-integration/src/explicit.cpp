// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all_core.hpp>
#include <erbsland/BindEvent.hpp>
#include <erbsland/CurrentLoop.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventSource.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/EventThread.hpp>
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

static_assert(std::is_base_of_v<el::event::EventTarget, el::event::EventLoop>);
static_assert(std::is_same_v<decltype(el::event::currentLoop()), el::event::EventTargetPtr>);
static_assert(std::is_same_v<decltype(std::declval<el::core::Application &>().loop()), el::event::EventTargetPtr>);
static_assert(
    std::is_same_v<decltype(std::declval<const el::core::ApplicationPart &>().loop()), el::event::EventTargetPtr>);
static_assert(std::is_same_v<decltype(std::declval<el::event::EventThread &>().loop()), el::event::EventTargetPtr>);
static_assert(
    std::is_same_v<decltype(std::declval<el::event::EventThread &>().workerEventLoop()), el::event::EventLoop &>);
static_assert(std::is_same_v<
    decltype(std::declval<const el::event::EventSource &>().ownerLoop()),
    const el::event::EventTargetPtr &>);

struct BindingRecipient {
    void receive(int);
};

static_assert(
    std::is_invocable_v<decltype(el::event::bindEvent(std::declval<el::event::EventTargetPtr>(), []() -> void {}))>);
static_assert(std::is_invocable_v<
    decltype(el::event::bindEvent(
        std::declval<el::event::EventTargetPtr>(),
        std::declval<std::shared_ptr<BindingRecipient>>(),
        &BindingRecipient::receive)),
    int>);
static_assert(std::is_invocable_v<
    decltype(el::event::bindWeakEvent(
        std::declval<el::event::EventTargetPtr>(),
        std::declval<std::weak_ptr<BindingRecipient>>(),
        &BindingRecipient::receive)),
    int>);
