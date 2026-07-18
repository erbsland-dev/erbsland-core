// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

void writeAtomicRecord(const el::Path &path);

void writeAtomicRecord() {
    const auto directory = createStreamDemoDirectory("原子记录"_el);
    const auto path = directory->path() / "geometry.bin"_el;
    writeAtomicRecord(path);
    el::io::printLine("Written bytes: "_el, path.content().readDataOrThrow().length().toSizeT());
}

/// Retry a complete byte write for at most one minute.
/// A timeout accepts no bytes, so keep the record unchanged and repeat the complete call.
void writeAtomicRecord(const el::Path &path) {
    constexpr auto cMaximumAttempts = 60U;

    auto options = el::PathWriteDataOptions{};
    options.setTimeout(el::TimeDelta::seconds(1));
    const auto output = path.content().openByteOutputStream(options);
    const auto record = el::ByteBlock{std::vector<uint8_t>{4U, 8U, 15U, 16U, 23U, 42U}};

    try {
        for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
            if (!output->isReady()) {
                el::io::printLine("The output is still processing earlier data."_el);
            }
            if (output->write(record).isSuccess()) {
                if (output->close().isTimeout()) {
                    output->abort();
                    throw el::RuntimeError{"Closing the geometry-record file timed out."_el};
                }
                return;
            }
        }
    } catch (const el::StreamError &) {
        output->abort();
        throw el::RuntimeError{"The geometry record could not be written."_el, std::current_exception()};
    }

    output->abort();
    throw el::RuntimeError{"Waiting for the geometry-record output took longer than one minute."_el};
}

}
