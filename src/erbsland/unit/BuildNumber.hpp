// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "VersionComponent.hpp"

namespace erbsland::unit {

/// The buildnumber component of a version.
using BuildNumber = VersionComponent<VersionPart::Build>;

}
