// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathReadTextOptions.hpp"

namespace erbsland::path {

using namespace text;
using namespace unit;

PathReadTextOptions::PathReadTextOptions(ByteCount maximumByteLength) : _maximumByteLength{maximumByteLength} {
}

PathReadTextOptions::PathReadTextOptions(CpCount maximumCpLength) : _maximumCpLength{maximumCpLength} {
}

PathReadTextOptions::PathReadTextOptions(StringEncoding encoding) : _encoding{encoding} {
}

auto PathReadTextOptions::setEncoding(StringEncoding value) -> PathReadTextOptions & {
    _encoding = value;
    return *this;
}

auto PathReadTextOptions::setBomMode(StringBomMode value) -> PathReadTextOptions & {
    _bomMode = value;
    return *this;
}

auto PathReadTextOptions::setEncodingMode(EncodingMode value) -> PathReadTextOptions & {
    _encodingMode = value;
    return *this;
}

auto PathReadTextOptions::setMaximumByteLength(ByteCount value) -> PathReadTextOptions & {
    _maximumByteLength = value;
    return *this;
}

auto PathReadTextOptions::setMaximumCpLength(CpCount value) -> PathReadTextOptions & {
    _maximumCpLength = value;
    return *this;
}

auto PathReadTextOptions::setTimeout(const time::TimeDelta value) noexcept -> PathReadTextOptions & {
    _streamSettings.setTimeout(value);
    return *this;
}

auto PathReadTextOptions::setBuffering(const stream::StreamBuffering value) noexcept -> PathReadTextOptions & {
    _streamSettings.setBuffering(value);
    return *this;
}

auto PathReadTextOptions::setSensitive(bool value) noexcept -> PathReadTextOptions & {
    _streamSettings.setSensitive(value);
    return *this;
}

auto PathReadTextOptions::setStreamSettings(const stream::InputStreamSettings &value) noexcept
    -> PathReadTextOptions & {
    _streamSettings = value;
    return *this;
}

}
