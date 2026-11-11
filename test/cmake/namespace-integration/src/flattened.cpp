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

static_assert(sizeof(el::RegEx) > 0);
static_assert(sizeof(el::TlsClientConnection) > 0);
static_assert(std::is_same_v<el::String, erbsland::text::String>);
static_assert(std::is_same_v<el::Application, erbsland::core::Application>);
static_assert(std::is_same_v<el::OperatingSystem, erbsland::system::OperatingSystem>);
static_assert(std::is_same_v<decltype(el::sys_info::operatingSystem()), el::OperatingSystem>);

static_assert(std::is_base_of_v<el::EventTarget, el::EventLoop>);
static_assert(std::is_same_v<decltype(el::currentLoop()), el::EventTargetPtr>);
static_assert(std::is_same_v<decltype(std::declval<el::Application &>().loop()), el::EventTargetPtr>);
static_assert(std::is_same_v<decltype(std::declval<const el::ApplicationPart &>().loop()), el::EventTargetPtr>);
static_assert(std::is_same_v<decltype(std::declval<el::EventThread &>().loop()), el::EventTargetPtr>);
static_assert(std::is_same_v<decltype(std::declval<el::EventThread &>().workerEventLoop()), el::EventLoop &>);
static_assert(
    std::is_same_v<decltype(std::declval<const el::EventSource &>().ownerLoop()), const el::EventTargetPtr &>);

struct BindingRecipient {
    void receive(int);
};

static_assert(std::is_invocable_v<decltype(el::bindEvent(std::declval<el::EventTargetPtr>(), []() -> void {}))>);
static_assert(std::is_invocable_v<
    decltype(el::bindEvent(
        std::declval<el::EventTargetPtr>(),
        std::declval<std::shared_ptr<BindingRecipient>>(),
        &BindingRecipient::receive)),
    int>);
static_assert(std::is_invocable_v<
    decltype(el::bindWeakEvent(
        std::declval<el::EventTargetPtr>(),
        std::declval<std::weak_ptr<BindingRecipient>>(),
        &BindingRecipient::receive)),
    int>);
