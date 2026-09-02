// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EnvironmentVariables_fwd.hpp"
#include "PlatformErrorContext_fwd.hpp"
#include "ProcessId_fwd.hpp"
#include "ProcessInfo_fwd.hpp"
#include "Subprocess_fwd.hpp"
#include "SubprocessOptions_fwd.hpp"
#include "UserLookup_fwd.hpp"

#include "../core/Namespaces.hpp"

#include <cstdint>

namespace erbsland::system {

class CpuArchitecture;
class FileIdentity;
class GroupId;
class GroupName;
class OperatingSystem;
class PlatformError;
class PlatformErrorCategory;
class SubprocessExitStatus;
enum class SubprocessOutputMode : std::uint8_t;
class UserId;
class UserName;

}

namespace erbsland {

using system::CpuArchitecture;
using system::EnvironmentVariables;
using system::EnvironmentVariablesPtr;
using system::FileIdentity;
using system::GroupId;
using system::GroupName;
using system::OperatingSystem;
using system::PlatformError;
using system::PlatformErrorCategory;
using system::PlatformErrorContext;
using system::PlatformErrorContextConstPtr;
using system::PlatformErrorContextPtr;
using system::ProcessId;
using system::ProcessInfo;
using system::Subprocess;
using system::SubprocessExitStatus;
using system::SubprocessOptions;
using system::SubprocessOutputMode;
using system::UserId;
using system::UserLookup;
using system::UserLookupPtr;
using system::UserName;

namespace sys_info = system::info;

}
