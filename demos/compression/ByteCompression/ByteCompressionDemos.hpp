// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

namespace demo {

void compressOneShot();
void compressStreaming();
void compareBzip2Levels();
void compareDeflateLevels();
void compareLz4Levels();
void compareLzmaLevels();
void compareZstandardLevels();
void decompressOneShot();
void decompressStreaming();

}
