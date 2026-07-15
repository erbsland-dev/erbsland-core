// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InputStream.hpp"
#include "StreamReadResult.hpp"
#include "TextInputStream_fwd.hpp"

#include "../text/Char.hpp"
#include "../text/String.hpp"
#include "../text/StringEncoding.hpp"
#include "../unit/CpLength.hpp"
#include "../util/CoAsyncGenerator.hpp"
#include "../util/CoTask.hpp"

namespace erbsland::stream {

/// A stream that reads decoded Unicode text.
/// Chunk reads return available decoded text. Line and aggregate reads retain partial text internally across timeouts,
/// so callers repeat the same operation without joining fragments. Bounded allocating methods require a finite
/// maximum; no-argument convenience methods use `cDefaultTextReadMaximum`.
/// @tested{EncodedTextStreamTest AsyncStreamTest}
class TextInputStream : public InputStream {
public:
    /// The default maximum for no-argument text reads: 10 Mi code points.
    static constexpr auto cDefaultTextReadMaximum = unit::CpLength{10U * 1024U * 1024U};

public:
    ~TextInputStream() override = default;

public: // accessors
    /// Get the encoding configured for the stream.
    [[nodiscard]] virtual auto encoding() const noexcept -> text::StringEncoding = 0;
    /// Get the effective encoding used by the stream.
    /// For BOM-detecting encodings, this returns the resolved concrete byte order after it is known.
    [[nodiscard]] virtual auto effectiveEncoding() const noexcept -> text::StringEncoding = 0;

public: // default interface
    /// Read up to `cDefaultTextReadMaximum` decoded characters.
    /// @return The status and decoded text.
    /// `Timeout` and `Finished` results have no caller-visible payload.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    /// @throws text::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] auto read() -> StreamReadResult<text::String>;
    /// Read one line using `cDefaultTextReadMaximum` as the maximum length.
    /// The returned line includes its line ending when one is present.
    /// Partial text is retained after a timeout. Repeating the call continues the same line operation.
    /// @return `Data` with a line, maximum-length chunk, or final unterminated line; otherwise an empty status.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    /// @throws text::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] auto readLine() -> StreamReadResult<text::String>;
    /// Read all remaining text using `cDefaultTextReadMaximum` as the maximum length.
    /// @return Aggregated text, or an empty `Timeout`/`Finished` result.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    /// @throws text::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] auto readAll() -> StreamReadResult<text::String>;

public: // core interface
    /// Read one decoded character.
    /// @return The status and next character.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    /// @throws text::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] virtual auto readChar() -> StreamReadResult<text::Char> = 0;
    /// Read up to `maximum` decoded characters.
    /// @param maximum The maximum number of decoded characters to read.
    /// @return The status and decoded text.
    /// A data result may contain fewer than `maximum` characters.
    /// @throws err::ParameterError If `maximum` is infinite.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    /// @throws text::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] virtual auto read(unit::CpLength maximum) -> StreamReadResult<text::String> = 0;
    /// Read one line.
    /// The returned line includes its line ending when one is present.
    /// @param maximum The maximum number of decoded characters to read.
    /// @return A complete line, maximum-length chunk, final unterminated line, or an empty status.
    /// @throws err::ParameterError If `maximum` is infinite.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    /// @throws text::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] virtual auto readLine(unit::CpLength maximum) -> StreamReadResult<text::String> = 0;
    /// Read all remaining text.
    /// @param maximum The maximum number of decoded characters to read.
    /// @return Aggregated text up to the finite maximum or end-of-stream, or an empty status.
    /// @throws err::ParameterError If `maximum` is infinite.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    /// @throws text::EncodingError If the stream was configured to throw on decoding errors and decoding fails.
    [[nodiscard]] virtual auto readAll(unit::CpLength maximum) -> StreamReadResult<text::String> = 0;

public: // coroutine interface
    /// Asynchronously read one text block using the default maximum.
    /// @return A task with the same result as `read()`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws stream::StreamError When the task result is observed if the stream or backing source fails.
    /// @throws text::EncodingError When the task result is observed if configured decoding fails.
    [[nodiscard]] auto coRead() -> util::CoTask<StreamReadResult<text::String>>;
    /// Asynchronously read one text block at a code-point boundary.
    /// @param maximum The maximum decoded code-point length.
    /// @return A task with the same result as `read(maximum)`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws err::ParameterError When the task result is observed if `maximum` is infinite.
    /// @throws stream::StreamError When the task result is observed if the stream or backing source fails.
    /// @throws text::EncodingError When the task result is observed if configured decoding fails.
    [[nodiscard]] auto coRead(unit::CpLength maximum) -> util::CoTask<StreamReadResult<text::String>>;
    /// Asynchronously read one line using the default maximum.
    /// @return A task with the same result as `readLine()`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws stream::StreamError When the task result is observed if the stream or backing source fails.
    /// @throws text::EncodingError When the task result is observed if configured decoding fails.
    [[nodiscard]] auto coReadLine() -> util::CoTask<StreamReadResult<text::String>>;
    /// Asynchronously read one line.
    /// @param maximum The maximum decoded code-point length.
    /// @return A task with the same result as `readLine(maximum)`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws err::ParameterError When the task result is observed if `maximum` is infinite.
    /// @throws stream::StreamError When the task result is observed if the stream or backing source fails.
    /// @throws text::EncodingError When the task result is observed if configured decoding fails.
    [[nodiscard]] auto coReadLine(unit::CpLength maximum) -> util::CoTask<StreamReadResult<text::String>>;
    /// Asynchronously read all remaining text using the default maximum.
    /// @return A task with the same result as `readAll()`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws stream::StreamError When the task result is observed if the stream or backing source fails.
    /// @throws text::EncodingError When the task result is observed if configured decoding fails.
    [[nodiscard]] auto coReadAll() -> util::CoTask<StreamReadResult<text::String>>;
    /// Asynchronously read all remaining text.
    /// @param maximum The maximum decoded code-point length.
    /// @return A task with the same result as `readAll(maximum)`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws err::ParameterError When the task result is observed if `maximum` is infinite.
    /// @throws stream::StreamError When the task result is observed if the stream or backing source fails.
    /// @throws text::EncodingError When the task result is observed if configured decoding fails.
    [[nodiscard]] auto coReadAll(unit::CpLength maximum) -> util::CoTask<StreamReadResult<text::String>>;
    /// Asynchronously generate text blocks using the default maximum block length.
    /// Data and timeout results are yielded; end-of-stream completes the generator.
    /// @return A lazy generator for bounded text-block results.
    /// @throws err::LogicError When first advanced if this stream is not shared-owned.
    /// @throws stream::StreamError When advancing if the stream or backing source fails.
    /// @throws text::EncodingError When advancing if configured decoding fails.
    [[nodiscard]] auto coReadBlocks() -> util::CoAsyncGenerator<StreamReadResult<text::String>>;
    /// Asynchronously generate text blocks at code-point boundaries.
    /// @param maximum The positive finite maximum for each block.
    /// Data and timeout results are yielded; end-of-stream completes the generator.
    /// @return A lazy generator for bounded text-block results.
    /// @throws err::ParameterError When first advanced if `maximum` is zero or infinite.
    /// @throws err::LogicError When first advanced if this stream is not shared-owned.
    /// @throws stream::StreamError When advancing if the stream or backing source fails.
    /// @throws text::EncodingError When advancing if configured decoding fails.
    [[nodiscard]] auto coReadBlocks(unit::CpLength maximum) -> util::CoAsyncGenerator<StreamReadResult<text::String>>;
    /// Asynchronously generate lines using the default maximum line length.
    /// Data and timeout results are yielded; end-of-stream completes the generator.
    /// @return A lazy generator for bounded line results.
    /// @throws err::LogicError When first advanced if this stream is not shared-owned.
    /// @throws stream::StreamError When advancing if the stream or backing source fails.
    /// @throws text::EncodingError When advancing if configured decoding fails.
    [[nodiscard]] auto coReadLines() -> util::CoAsyncGenerator<StreamReadResult<text::String>>;
    /// Asynchronously generate lines.
    /// @param maximum The positive finite maximum for each returned line or line fragment.
    /// Data and timeout results are yielded; end-of-stream completes the generator.
    /// @return A lazy generator for bounded line results.
    /// @throws err::ParameterError When first advanced if `maximum` is zero or infinite.
    /// @throws err::LogicError When first advanced if this stream is not shared-owned.
    /// @throws stream::StreamError When advancing if the stream or backing source fails.
    /// @throws text::EncodingError When advancing if configured decoding fails.
    [[nodiscard]] auto coReadLines(unit::CpLength maximum) -> util::CoAsyncGenerator<StreamReadResult<text::String>>;
};

}
