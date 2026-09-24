// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

namespace demo {

void compiledRules(const el::Path &configurationPath);
void documentRules(const el::Path &configurationPath);
void loadingStrategies();
void manualValidation(const el::Path &configurationPath);
void parseAndAccess();

}
