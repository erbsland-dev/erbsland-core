// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpClientConnectionManager.hpp"

#include "../../../../event/Events.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringEditor.hpp"
#include "../../../source/Connection.hpp"
#include "../../../source/ConnectionEventEditor.hpp"
#include "../../ConnectionProtocolAccess.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

HttpClientConnectionManager::HttpClientConnectionManager(event::EventsPtr ownerEvents) :
    _ownerEvents{std::move(ownerEvents)} {
}

HttpClientConnectionManager::~HttpClientConnectionManager() {
    invalidate();
}

auto HttpClientConnectionManager::lease(const Url &url, const std::uint64_t generation) -> std::optional<Lease> {
    const auto origin = originKey(url);
    const auto now = time::TimePoint::now();
    for (auto iterator = _idle.begin(); iterator != _idle.end();) {
        if (iterator->connection->state() != ConnectionState::Active || iterator->deadline <= now ||
            !ConnectionProtocolAccess::takeRetainedInput(*iterator->connection).isEmpty()) {
            iterator->connection->abort();
            iterator = _idle.erase(iterator);
        } else {
            ++iterator;
        }
    }
    const auto iterator = std::ranges::find_if(_idle, [&](const IdleConnection &entry) -> bool {
        return entry.origin == origin && entry.generation == generation;
    });
    if (iterator == _idle.end()) {
        return {};
    }
    auto result = Lease{iterator->connection, iterator->completedTransactions};
    result.connection->events().onData({}).onWritable({}).onClosed({}).onError({}).onFinal({});
    _idle.erase(iterator);
    return result;
}

void HttpClientConnectionManager::returnConnection(
    const Url &url,
    const std::uint64_t generation,
    ConnectionPtr connection,
    const unit::ItemCount completedTransactions,
    const HttpClientSessionOptions &options) {
    if (!options.connectionReuseEnabled() || connection == nullptr || connection->state() != ConnectionState::Active ||
        completedTransactions >= options.maximumTransactionsPerConnection()) {
        if (connection != nullptr) {
            connection->close();
        }
        return;
    }
    const auto origin = originKey(url);
    const auto existing = std::ranges::find(_idle, origin, &IdleConnection::origin);
    if (existing != _idle.end()) {
        existing->connection->close();
        _idle.erase(existing);
    }
    const auto identity = ++_identity;
    const auto weakSelf = weak_from_this();
    connection->events()
        .onData([weakSelf, origin, identity](mem::ByteBlock) -> void {
            if (const auto self = weakSelf.lock()) {
                self->evict(origin, identity);
            }
        })
        .onClosed([weakSelf, origin, identity](const ConnectionCloseContext &) -> void {
            if (const auto self = weakSelf.lock()) {
                self->evict(origin, identity);
            }
        })
        .onError([weakSelf, origin, identity](const NetworkErrorContext &) -> void {
            if (const auto self = weakSelf.lock()) {
                self->evict(origin, identity);
            }
        })
        .onFinal([weakSelf, origin, identity]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->evict(origin, identity);
            }
        });
    _idle.emplace_back(
        IdleConnection{
            origin,
            std::move(connection),
            completedTransactions,
            time::TimePoint::inFuture(options.idleConnectionTimeout()),
            generation,
            identity,
            ++_sequence});
    const auto weak = weak_from_this();
    _ownerEvents->invokeAfter(options.idleConnectionTimeout(), [weak, origin, identity]() -> void {
        if (const auto self = weak.lock()) {
            self->evict(origin, identity);
        }
    });
    while (_idle.size() > options.maximumIdleConnections().toSizeT()) {
        evictOldest();
    }
}

void HttpClientConnectionManager::invalidate() noexcept {
    auto idle = std::move(_idle);
    _idle.clear();
    _ownerEvents->invoke([idle = std::move(idle)]() -> void {
        for (const auto &entry : idle) {
            entry.connection->close();
        }
    });
}

auto HttpClientConnectionManager::originKey(const Url &url) -> text::String {
    auto result = StringEditor{};
    result.append(url.schemeText()).append("://"_el).append(url.endpoint().toString());
    return String{result};
}

void HttpClientConnectionManager::evict(const text::String &origin, const std::uint64_t identity) noexcept {
    const auto iterator = std::ranges::find_if(_idle, [&](const IdleConnection &entry) -> bool {
        return entry.origin == origin && entry.identity == identity;
    });
    if (iterator == _idle.end()) {
        return;
    }
    const auto connection = iterator->connection;
    _idle.erase(iterator);
    if (connection->state() == ConnectionState::Active) {
        connection->abort();
    }
}

void HttpClientConnectionManager::evictOldest() noexcept {
    const auto iterator = std::ranges::min_element(_idle, {}, &IdleConnection::sequence);
    if (iterator == _idle.end()) {
        return;
    }
    const auto connection = iterator->connection;
    _idle.erase(iterator);
    connection->close();
}

}
