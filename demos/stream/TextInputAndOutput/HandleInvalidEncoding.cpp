// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

void rejectInvalidEncoding(const el::Path &path);

void handleInvalidEncoding() {
    const auto directory = createStreamDemoDirectory("felkodning"_el);
    const auto path = directory->path() / "skadad.txt"_el;
    const auto bytes = el::ByteBlock({0x56U, 0xc3U, 0x28U});
    path.content().writeDataOrThrow(bytes);
    rejectInvalidEncoding(path);
}

/// Reject malformed UTF-8 when silently repairing the input could change its meaning.
/// Bound each aggregate read, repeat it after timeout, and distinguish decoding errors from failed I/O.
void rejectInvalidEncoding(const el::Path &path) {
    constexpr auto cMaximumAttempts = 3U;
    auto options = el::PathReadTextOptions{el::StringEncoding::Utf8};
    options.setEncodingMode(el::EncodingMode::Strict);
    options.setTimeout(el::TimeDelta::seconds(1));
    try {
        const auto input = path.content().openTextInputStream(options);
        for (auto attempt = 0U; attempt < cMaximumAttempts; ++attempt) {
            if (!input->readAll(el::CpLength{100U}).isTimeout()) {
                throw el::RuntimeError{"The damaged text was unexpectedly accepted."_el};
            }
        }
        throw el::RuntimeError{"The damaged text timed out."_el};
    } catch (const el::EncodingError &) {
        el::io::printLine("Invalid UTF-8 was rejected."_el);
    } catch (const el::PathError &) {
        throw el::RuntimeError{"The encoded text could not be opened."_el, std::current_exception()};
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The encoded text could not be read."_el, std::current_exception()};
    }
}

}
