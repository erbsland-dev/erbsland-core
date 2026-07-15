// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <memory>

namespace erbsland::event {

class EventThread;
using EventThreadPtr = std::shared_ptr<EventThread>;
using EventThreadWeakPtr = std::weak_ptr<EventThread>;

class ManagedEventThread;
using ManagedEventThreadPtr = std::shared_ptr<ManagedEventThread>;
using ManagedEventThreadWeakPtr = std::weak_ptr<ManagedEventThread>;

class UnmanagedEventThread;
using UnmanagedEventThreadPtr = std::shared_ptr<UnmanagedEventThread>;
using UnmanagedEventThreadWeakPtr = std::weak_ptr<UnmanagedEventThread>;

}
