// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathReadTextOptions.hpp"

namespace erbsland::path {

PathReadTextOptions::PathReadTextOptions(unit::ByteLength maximumByteLength) : _maximumByteLength{maximumByteLength} {
}

PathReadTextOptions::PathReadTextOptions(unit::CpLength maximumCpLength) : _maximumCpLength{maximumCpLength} {
}

PathReadTextOptions::PathReadTextOptions(text::StringEncoding encoding) : _encoding{encoding} {
}

auto PathReadTextOptions::setEncoding(text::StringEncoding value) -> PathReadTextOptions & {
    _encoding = value;
    return *this;
}

auto PathReadTextOptions::setBomMode(text::StringBomMode value) -> PathReadTextOptions & {
    _bomMode = value;
    return *this;
}

auto PathReadTextOptions::setEncodingErrorMode(text::EncodingErrorMode value) -> PathReadTextOptions & {
    _encodingErrorMode = value;
    return *this;
}

auto PathReadTextOptions::setMaximumByteLength(unit::ByteLength value) -> PathReadTextOptions & {
    _maximumByteLength = value;
    return *this;
}

auto PathReadTextOptions::setMaximumCpLength(unit::CpLength value) -> PathReadTextOptions & {
    _maximumCpLength = value;
    return *this;
}

}
