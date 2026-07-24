// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProfileTypes.hpp"

namespace app::stream {

using namespace el::text::literals;

auto toString(const RunMode value) -> el::String {
    return value == RunMode::Profile ? "profile"_el : "benchmark"_el;
}
auto toString(const Direction value) -> el::String {
    return value == Direction::Read ? "read"_el : "write"_el;
}

auto toString(const FileType value) -> el::String {
    switch (value) {
    case FileType::Binary:
        return "binary"_el;
    case FileType::Utf8:
        return "utf8"_el;
    case FileType::Utf16:
        return "utf16"_el;
    case FileType::Utf32:
        return "utf32"_el;
    }
    return {};
}

auto toString(const Locality value) -> el::String {
    return value == Locality::Hot ? "hot"_el : "rotating"_el;
}

auto toString(const ChunkMode value) -> el::String {
    switch (value) {
    case ChunkMode::Fixed:
        return "fixed"_el;
    case ChunkMode::Incrementing:
        return "incrementing"_el;
    case ChunkMode::Random:
        return "random"_el;
    }
    return {};
}

auto toString(const Method value) -> el::String {
    switch (value) {
    case Method::ReadByte:
        return "read-byte"_el;
    case Method::ReadSpan:
        return "read-span"_el;
    case Method::ReadBlock:
        return "read-block"_el;
    case Method::ReadExact:
        return "read-exact"_el;
    case Method::ReadAll:
        return "read-all"_el;
    case Method::WriteByte:
        return "write-byte"_el;
    case Method::WriteSpan:
        return "write-span"_el;
    case Method::WriteBlock:
        return "write-block"_el;
    case Method::ReadChar:
        return "read-char"_el;
    case Method::ReadText:
        return "read-text"_el;
    case Method::ReadLine:
        return "read-line"_el;
    case Method::ReadAllText:
        return "read-all"_el;
    case Method::WriteChar:
        return "write-char"_el;
    case Method::WriteText:
        return "write-text"_el;
    case Method::WriteLine:
        return "write-line"_el;
    }
    return {};
}

auto toString(const el::StreamBuffering value) -> el::String {
    switch (value) {
    case el::StreamBuffering::MinimalMemory:
        return "minimal-memory"_el;
    case el::StreamBuffering::Interactive:
        return "interactive"_el;
    case el::StreamBuffering::Balanced:
        return "balanced"_el;
    case el::StreamBuffering::Throughput:
        return "throughput"_el;
    case el::StreamBuffering::Bulk:
        return "bulk"_el;
    }
    return {};
}

auto encodingFor(const FileType value) -> el::StringEncoding {
    switch (value) {
    case FileType::Utf8:
        return el::StringEncoding::Utf8;
    case FileType::Utf16:
        return el::StringEncoding::Utf16;
    case FileType::Utf32:
        return el::StringEncoding::Utf32;
    case FileType::Binary:
        return el::StringEncoding::Utf8;
    }
    return el::StringEncoding::Utf8;
}

auto isTextFile(const FileType value) noexcept -> bool {
    return value != FileType::Binary;
}
auto isTextMethod(const Method value) noexcept -> bool {
    return value >= Method::ReadChar;
}

auto methodDirection(const Method value) noexcept -> Direction {
    switch (value) {
    case Method::ReadByte:
    case Method::ReadSpan:
    case Method::ReadBlock:
    case Method::ReadExact:
    case Method::ReadAll:
    case Method::ReadChar:
    case Method::ReadText:
    case Method::ReadLine:
    case Method::ReadAllText:
        return Direction::Read;
    case Method::WriteByte:
    case Method::WriteSpan:
    case Method::WriteBlock:
    case Method::WriteChar:
    case Method::WriteText:
    case Method::WriteLine:
        return Direction::Write;
    }
    return Direction::Read;
}

}
