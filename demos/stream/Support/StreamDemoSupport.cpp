// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamDemoSupport.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PathTempDirectoryOptions.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringEncoding.hpp>
#include <erbsland/TempDirectory.hpp>

namespace demo {

auto createStreamDemoDirectory(const el::String &prefix) -> el::TempDirectoryPtr {
    auto options = el::PathTempDirectoryOptions{};
    options.setPrefix(prefix).setSuffix("-stream-demo"_el).setRandomLength(el::CpCount{8U});
    return el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(options);
}

auto bytesFromText(const el::String &text) -> el::ByteBlock {
    return el::StringEncoder{text}.encode(el::StringEncoding::Utf8);
}

}
