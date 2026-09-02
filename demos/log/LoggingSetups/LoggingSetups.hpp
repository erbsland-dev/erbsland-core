// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace demo {

auto runApplicationConfiguration(int argc, char *argv[]) -> int;
auto runDeferredConfiguration(int argc, char *argv[]) -> int;
auto runLastErrorDump(int argc, char *argv[]) -> int;
auto runMinimalLogging(int argc, char *argv[]) -> int;
auto runMediumLogging(int argc, char *argv[]) -> int;
auto runLargeLogging(int argc, char *argv[]) -> int;

}
