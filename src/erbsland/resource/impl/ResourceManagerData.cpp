// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceManagerData.hpp"

#include "../../text/String.hpp"

namespace erbsland::resource::impl {

ResourceManagerData::ResourceManagerData() {
    auto *node = ResourceStorageRegistration::registry().load(std::memory_order_acquire);
    while (node != nullptr) {
        const auto entry = ResourceStorageInfo::parse(node->dataProvider(), node->infoBlock);
        if (entry.has_value()) {
            const auto identifier = entry->identifier;
            const auto path = entry->path;
            auto paths = _index.get(identifier, PathMap{});
            if (!paths.contains(path)) {
                paths.set(path, std::make_shared<EntryState>(*entry));
                _index.set(identifier, std::move(paths));
            }
        }
        node = node->next;
    }
}

auto ResourceManagerData::find(const text::String &identifier, const text::String &path) const -> EntryStatePtr {
    const auto paths = _index.get(identifier);
    if (!paths.has_value()) {
        return {};
    }
    return paths->get(path, {});
}

}
