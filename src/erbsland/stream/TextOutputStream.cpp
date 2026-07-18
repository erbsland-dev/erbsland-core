// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextOutputStream.hpp"

#include "impl/PrintContextToWrite.hpp"

#include <utility>

namespace erbsland::stream {

auto TextOutputStream::createPrintContext() -> TextPrintContextPtr {
    return std::make_unique<impl::PrintContextToWrite>(*this);
}

auto TextOutputStream::coWrite(text::String text) -> util::CoTask<StreamWriteStatus> {
    auto self = std::static_pointer_cast<TextOutputStream>(sharedOutputStream());
    return util::CoTask<StreamWriteStatus>::run(
        [self = std::move(self), text = std::move(text)]() mutable -> StreamWriteStatus {
            return self->write(std::move(text));
        });
}

auto TextOutputStream::coWriteLine() -> util::CoTask<StreamWriteStatus> {
    auto self = std::static_pointer_cast<TextOutputStream>(sharedOutputStream());
    return util::CoTask<StreamWriteStatus>::run(
        [self = std::move(self)]() -> StreamWriteStatus { return self->writeLine(); });
}

auto TextOutputStream::coWriteLine(text::String text) -> util::CoTask<StreamWriteStatus> {
    auto self = std::static_pointer_cast<TextOutputStream>(sharedOutputStream());
    return util::CoTask<StreamWriteStatus>::run(
        [self = std::move(self), text = std::move(text)]() mutable -> StreamWriteStatus {
            return self->writeLine(std::move(text));
        });
}

}
