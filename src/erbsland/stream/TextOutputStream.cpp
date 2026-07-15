// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextOutputStream.hpp"

#include "impl/PrintContextToWrite.hpp"

namespace erbsland::stream {

auto TextOutputStream::createPrintContext() -> TextPrintContextPtr {
    return std::make_unique<impl::PrintContextToWrite>(*this);
}

auto TextOutputStream::coWrite(text::String text) -> util::CoTask<StreamWriteStatus> {
    auto self = std::static_pointer_cast<TextOutputStream>(sharedOutputStream());
    return util::CoTask<StreamWriteStatus>::run(
        [self = std::move(self), text = std::move(text)]() -> StreamWriteStatus {
            return self->write(text::StringView{text});
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
        [self = std::move(self), text = std::move(text)]() -> StreamWriteStatus {
            return self->writeLine(text::StringView{text});
        });
}

}
