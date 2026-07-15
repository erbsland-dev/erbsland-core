// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

void writeEncodedMeasurement(const el::Path &path);

void configureEncoding() {
    const auto directory = createStreamDemoDirectory("kodning"_el);
    const auto path = directory->path() / "prov.txt"_el;
    writeEncodedMeasurement(path);
}

/// Open a bounded UTF-16 text stream and require a byte-order mark.
/// Retry the unchanged atomic write after timeout, verify the resolved byte order, and close the stream explicitly.
void writeEncodedMeasurement(const el::Path &path) {
    constexpr auto cMaximumAttempts = 3U;
    auto options = el::PathWriteTextOptions{el::StringEncoding::Utf16};
    options.setBomMode(el::StringBomMode::Require);
    options.setTimeout(el::TimeDelta::seconds(1));

    try {
        const auto output = path.content().openTextOutputStream(options);
        for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
            if (output->writeLine("Rötter: 12 cm"_el).isSuccess()) {
                el::io::printLine("Configured UTF-16: "_el, output->encoding() == el::StringEncoding::Utf16);
                el::io::printLine(
                    "Effective little-endian: "_el,
                    output->effectiveEncoding() == el::StringEncoding::Utf16LittleEndian);
                if (output->close().isTimeout()) {
                    output->abort();
                    throw el::RuntimeError{"Closing the encoding sample timed out."_el};
                }
                return;
            }
        }
        output->abort();
        throw el::RuntimeError{"The encoding sample timed out."_el};
    } catch (const el::PathError &) {
        throw el::RuntimeError{"The encoding sample could not be opened."_el, std::current_exception()};
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The encoding sample could not be written."_el, std::current_exception()};
    }
}

}
