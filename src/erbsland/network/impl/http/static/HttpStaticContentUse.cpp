// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStaticContentUse.hpp"

#include "../../../http_server/HttpStaticContentHandler.hpp"

#include <utility>

namespace erbsland::network::impl {

HttpStaticContentUse::HttpStaticContentUse(std::vector<HttpStaticContentHandlerPtr> handlers) noexcept :
    _handlers{std::move(handlers)} {
}

auto HttpStaticContentUse::create(const std::vector<HttpStaticContentHandlerPtr> &handlers)
    -> std::shared_ptr<HttpStaticContentUse> {
    auto frozen = std::vector<HttpStaticContentHandlerPtr>{};
    frozen.reserve(handlers.size());
    try {
        for (const auto &handler : handlers) {
            handler->beginUse();
            frozen.emplace_back(handler);
        }
    } catch (...) {
        for (const auto &handler : frozen) {
            handler->endUse();
        }
        throw;
    }
    return std::shared_ptr<HttpStaticContentUse>{new HttpStaticContentUse{std::move(frozen)}};
}

HttpStaticContentUse::~HttpStaticContentUse() {
    for (const auto &handler : _handlers) {
        handler->endUse();
    }
}

}
