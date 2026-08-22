// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../path/Path.hpp"
#include "../../../http_server/HttpStaticContent.hpp"

#include <mutex>

namespace erbsland::network::impl {

/// Static content backed by one securely resolved regular file.
/// @notest{Internal implementation covered by HttpStaticContentTest and HttpServerLiveTest.}
class HttpStaticFileContent final : public HttpStaticContent {
public:
    /// Capture one exact file and its probed length.
    HttpStaticFileContent(path::Path path, unit::ByteLength length) noexcept;

    // defaults
    ~HttpStaticFileContent() override = default;

public:
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength override { return _length; }
    [[nodiscard]] auto retainedMemoryLength() const noexcept -> unit::ByteLength override { return {}; }
    [[nodiscard]] auto open() -> stream::ByteInputStreamPtr override;

private:
    path::Path _path;         ///< Resolved file path captured by the probe.
    unit::ByteLength _length; ///< Exact length captured by the probe.
    std::mutex _mutex;        ///< Protects one-shot opening.
    bool _opened{};           ///< Whether opening was already attempted.
};

}
