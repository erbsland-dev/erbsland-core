// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StreamDemoSupport.hpp"

#include <erbsland/text/StringEncoder.hpp>

namespace demo {

auto createStreamDemoDirectory(const el::StringView &prefix) -> el::TempDirectoryPtr {
    auto options = el::PathTempDirectoryOptions{};
    options.setPrefix(prefix).setSuffix("-stream-demo"_el).setRandomLength(el::CpLength{8U});
    return el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(options);
}

auto bytesFromText(const el::StringView &text) -> el::ByteBlock {
    return el::StringEncoder{el::String{text}}.encode(el::StringEncoding::Utf8);
}

}
