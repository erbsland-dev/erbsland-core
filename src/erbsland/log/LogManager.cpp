// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogManager.hpp"

#include "impl/LogManagerData.hpp"

#include <utility>

namespace erbsland::log {

LogManager::LogManager(LogManagerOptions options, PrivateTag) :
    _data{std::make_shared<impl::LogManagerData>(std::move(options))} {
    _data->initialize();
}

LogManager::~LogManager() {
    shutdown();
}

auto LogManager::create(LogManagerOptions options) -> LogManagerPtr {
    return std::make_shared<LogManager>(std::move(options), PrivateTag{});
}

auto LogManager::rootStream() const noexcept -> const LogStreamPtr & {
    return _data->rootStream();
}

auto LogManager::createStream(LogPath path, LogTraceSection traceSection) -> LogStreamPtr {
    return _data->createStream(std::move(path), std::move(traceSection));
}

auto LogManager::createStream(const text::String &path, LogTraceSection traceSection) -> LogStreamPtr {
    return createStream(LogPath{path}, std::move(traceSection));
}

void LogManager::setConfiguration(LogConfiguration configuration) {
    _data->setConfiguration(std::move(configuration));
}

auto LogManager::configuration() const -> LogConfiguration {
    return _data->configuration();
}

void LogManager::pause() noexcept {
    _data->pause();
}

void LogManager::resume() noexcept {
    _data->resume();
}

void LogManager::shutdown() noexcept {
    if (_data) {
        _data->shutdown();
    }
}

auto LogManager::statistics() const noexcept -> LogManagerStatistics {
    return _data->statistics();
}

void LogManager::addPersistentWriter(LogWriterPtr writer, LogWriterFilter filter) {
    _data->addPersistentWriter(std::move(writer), std::move(filter));
}

}
