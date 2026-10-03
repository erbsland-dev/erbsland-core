#pragma once

#include "ApplicationData.hpp"

namespace erbsland::core::impl {

/// Retain the scoped application data, or the current process application data when no scope is installed.
/// @return Owning application data; retain a component instead when only that component is needed.
/// @tested{ApplicationTestScopeTest TlsBackgroundOperationTest}
[[nodiscard]] auto applicationData() -> ApplicationDataPtr;

}
