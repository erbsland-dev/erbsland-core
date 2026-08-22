// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../resource/Resources_fwd.hpp"
#include "../../../../text/String.hpp"
#include "../../../http_server/HttpStaticContent.hpp"

#include <mutex>

namespace erbsland::network::impl {

/// Static content backed by one exact resource lookup.
/// @notest{Internal implementation covered by HttpStaticContentTest and HttpServerLiveTest.}
class HttpStaticResourceContent final : public HttpStaticContent {
public:
    /// Capture one resource provider, exact key, and logical length.
    HttpStaticResourceContent(
        resource::ResourcesConstPtr retainedResources,
        const resource::Resources &resources,
        text::String identifier,
        text::String path,
        unit::ByteLength length) noexcept;

    // defaults
    ~HttpStaticResourceContent() override = default;

public:
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength override { return _length; }
    [[nodiscard]] auto retainedMemoryLength() const noexcept -> unit::ByteLength override { return _length; }
    [[nodiscard]] auto open() -> stream::ByteInputStreamPtr override;

private:
    resource::ResourcesConstPtr _retainedResources; ///< Optional retained provider ownership.
    const resource::Resources *_resources{};        ///< Active retained or application provider.
    text::String _identifier;                       ///< Exact resource identifier.
    text::String _path;                             ///< Exact portable resource path.
    unit::ByteLength _length;                       ///< Expected logical data length.
    std::mutex _mutex;                              ///< Protects one-shot opening.
    bool _opened{};                                 ///< Whether opening was already attempted.
};

}
