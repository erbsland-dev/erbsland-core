// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

[[nodiscard]] auto writeCellObservationLog(el::TextOutputStream &output) -> std::size_t;

void applyBackPressure() {
    auto settings = el::OutputStreamSettings{};
    settings.setBuffering(el::StreamBuffering::Interactive).setBackBufferLimit(el::ByteLength{512U});
    auto options = el::PathWriteTextOptions{};
    options.setStreamSettings(settings);

    const auto directory = createStreamDemoDirectory("καταγραφή"_el);
    const auto output = (directory->path() / "κύτταρα.log"_el).content().openTextOutputStream(options);
    const auto recordCount = writeCellObservationLog(*output);
    output->close();

    el::io::printLine("Log lines written: "_el, recordCount);
}

/// Apply back pressure while producing a large text log.
/// Wait before preparing each record when earlier output is still queued. This keeps a fast producer close to the
/// destination's pace instead of repeatedly filling the bounded back buffer.
auto writeCellObservationLog(el::TextOutputStream &output) -> std::size_t {
    constexpr auto cRecordCount = std::size_t{25U * 1000U};
    constexpr auto cObservation = ": η κυτταρική μεμβράνη είναι ακέραιη"_el;

    for (auto record = std::size_t{1U}; record <= cRecordCount; ++record) {
        if (!output.isReady() && output.waitForReady().isTimeout()) {
            throw el::RuntimeError{"The log output remained busy."_el};
        }
        if (output.printLine("Παρατήρηση "_el, record, cObservation).isTimeout()) {
            throw el::RuntimeError{"The log record was not accepted in time."_el};
        }
    }
    return cRecordCount;
}

}
