// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnmanagedEventThread.hpp"

#include "impl/UnmanagedEventThread.hpp"

namespace erbsland::event {

auto UnmanagedEventThread::create() -> UnmanagedEventThreadPtr {
    return std::make_shared<impl::UnmanagedEventThread>();
}

}
