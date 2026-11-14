// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Definitions.hpp>

#if defined(ERBSLAND_OS_WINDOWS)
#include "WindowsEventLoopTestCalls.hpp"

#include <erbsland/event/impl/WindowsEventLoopDriver.hpp>
using NativeTestDriver = el::event::impl::WindowsEventLoopDriver;
using NativeTestCalls = WindowsEventLoopTestCalls;
#elif defined(ERBSLAND_OS_MACOS)
#include "KqueueEventLoopTestCalls.hpp"

#include <erbsland/event/impl/KqueueEventLoopDriver.hpp>
using NativeTestDriver = el::event::impl::KqueueEventLoopDriver;
using NativeTestCalls = KqueueEventLoopTestCalls;
#else
#include "EpollEventLoopTestCalls.hpp"

#include <erbsland/event/impl/EpollEventLoopDriver.hpp>
using NativeTestDriver = el::event::impl::EpollEventLoopDriver;
using NativeTestCalls = EpollEventLoopTestCalls;
#endif
