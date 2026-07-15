// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

[[nodiscard]] auto readGeometryAngle(el::ByteInputStream &input) -> uint16_t;
void writeGeometryOffset(el::ByteOutputStream &output, int32_t offset);

void readAndWriteIntegers() {
    auto input = ScriptedByteInputStream{{0x01U, 0x2cU}, 1U, 1U};
    auto output = ScriptedByteOutputStream{1U};
    const auto angle = readGeometryAngle(input);
    constexpr auto offset = int32_t{-720};
    writeGeometryOffset(output, offset);

    el::io::printLine("Angle units: "_el, angle);
    el::io::printLine("Coordinate offset: "_el, offset);
}

/// Read a fixed-width integer with the byte order defined by the binary format.
/// Integer reads use exact-read semantics.
/// Retry the same helper after timeout and handle premature end separately.
auto readGeometryAngle(el::ByteInputStream &input) -> uint16_t {
    constexpr auto cMaximumAttempts = 3U;
    input.setEndianness(el::Endianness::Big);

    try {
        for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
            const auto result = input.readUInt16();
            if (result.hasData()) {
                return result.data();
            }
            if (result.isFinished()) {
                throw el::RuntimeError{"The geometry angle is incomplete."_el};
            }
            el::io::printLine("Reading the angle timed out; retrying."_el);
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The geometry angle could not be read."_el, std::current_exception()};
    }
    throw el::RuntimeError{"Waiting for the geometry angle took too long."_el};
}

/// Write a fixed-width integer as one atomic request.
/// A timeout accepts none of the encoded bytes, so retry the same value without changing it.
void writeGeometryOffset(el::ByteOutputStream &output, const int32_t offset) {
    constexpr auto cMaximumAttempts = 3U;
    output.setEndianness(el::Endianness::Big);

    try {
        for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
            if (output.writeInt32(offset).isSuccess()) {
                return;
            }
            el::io::printLine("Writing the offset timed out; retrying."_el);
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The geometry offset could not be written."_el, std::current_exception()};
    }

    throw el::RuntimeError{"Waiting for the geometry offset output took too long."_el};
}

}
