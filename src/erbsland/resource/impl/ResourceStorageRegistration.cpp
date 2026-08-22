// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceStorageRegistration.hpp"

namespace erbsland::resource::impl {

ResourceStorageRegistration::ResourceStorageRegistration(
    const ResourceDataProvider dataProvider, const std::span<const std::uint8_t> infoBlock) noexcept :
    _node{dataProvider, infoBlock, nullptr} {
    auto &head = registry();
    _node.next = head.load(std::memory_order_relaxed);
    while (!head.compare_exchange_weak(_node.next, &_node, std::memory_order_release, std::memory_order_relaxed)) {}
}

auto ResourceStorageRegistration::registry() noexcept -> std::atomic<Node *> & {
    static auto instance = std::atomic<Node *>{nullptr};
    return instance;
}

}
