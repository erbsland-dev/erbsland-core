// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

void writeGrowthReport(el::TextOutputStream &output);

void writeGrowthReport() {
    const auto output = el::StringBuilderStream::create();
    writeGrowthReport(*output);
    el::io::print(output->takeString());
}

/// Write a report with a bounded retry policy for every atomic text operation.
/// Retry the complete unchanged call after timeout because the stream accepted none of it.
/// Convert a stream failure into an application error where useful report context is available.
void writeGrowthReport(el::TextOutputStream &output) {
    constexpr auto cMaximumAttempts = 3U;
    const auto writeWithRetry = [&](const auto &operation) -> void {
        for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
            if (operation().isSuccess()) {
                return;
            }
        }
        throw el::RuntimeError{"The growth report timed out."_el};
    };

    try {
        writeWithRetry([&]() { return output.writeLine("Tillväxtrapport"_el); });
        writeWithRetry([&]() { return output.write("Mätserie: "_el); });
        writeWithRetry([&]() { return output.writeLine("björkplantor"_el); });
        writeWithRetry([&]() { return output.printLine("Dag "_el, 21, ": "_el, 18, " cm 🌿"_el); });
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The growth report could not be written."_el, std::current_exception()};
    }
}

}
