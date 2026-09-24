// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveReader.hpp"

#include "ZipError.hpp"

#include "impl/ArchiveReader.hpp"

#include "../../err/ParameterError.hpp"
#include "../../path/PathContent.hpp"
#include "../../path/PathError.hpp"
#include "../../path/PathReadDataOptions.hpp"
#include "../../path/SymlinkMode.hpp"
#include "../../stream/ByteBlockInputStream.hpp"
#include "../../text/Literals.hpp"

#include <memory>
#include <utility>

namespace erbsland::compression::zip {

using namespace text::literals;

auto ArchiveReader::create(const path::Path &source, ArchiveReaderOptions options) -> ArchiveReaderPtr {
    if (source.isEmpty()) {
        throw err::ParameterError{"The ZIP archive source path must not be empty."_el, "source"_el};
    }
    try {
        auto stream = source.content().openByteInputStream(
            path::PathReadDataOptions{}
                .setStreamSettings(options.inputStreamSettings())
                .setSymlinkMode(path::SymlinkMode::Follow));
        auto result = std::make_shared<impl::ArchiveReader>(std::move(stream), source, std::move(options));
        result->open();
        return result;
    } catch (const ZipError &) {
        throw;
    } catch (const path::PathError &) {
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::PathFailure,
                ZipOperationPhase::Open,
                "ZIP archive could not be opened"_el,
                "The source path could not be opened as an owned byte stream."_el}
                .setSourcePath(source),
            std::current_exception()};
    }
}

auto ArchiveReader::create(mem::ByteBlock source, ArchiveReaderOptions options) -> ArchiveReaderPtr {
    auto stream = std::make_shared<stream::ByteBlockInputStream>(std::move(source));
    auto result = std::make_shared<impl::ArchiveReader>(std::move(stream), path::Path{}, std::move(options));
    result->open();
    return result;
}

auto ArchiveReader::create(stream::ByteInputStreamPtr source, ArchiveReaderOptions options) -> ArchiveReaderPtr {
    auto result = std::make_shared<impl::ArchiveReader>(std::move(source), path::Path{}, std::move(options));
    result->open();
    return result;
}

}
