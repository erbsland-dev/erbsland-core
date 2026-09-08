// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationLogData.hpp"

#include "ApplicationTerminalData.hpp"

#include "../../../i18n/DisplayTextMap.hpp"
#include "../../../log/impl/ConsoleLogWriter.hpp"
#include "../../../log/impl/LastErrorsLogWriter.hpp"
#include "../../../log/impl/LogLineFormatter.hpp"
#include "../../../log/LogConfiguration.hpp"
#include "../../../log/LogManager.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/TextDocument.hpp"
#include "../../../text/TextNode.hpp"

#include <memory>
#include <utility>

namespace erbsland::core::impl {

using namespace text::literals;

auto ApplicationLogData::manager(const cterm::TerminalPtr &terminal) -> log::LogManager & {
    const auto lock = std::scoped_lock{_mutex};
    if (_manager == nullptr) {
        _manager = log::LogManager::create();
        auto configuration = log::LogConfiguration{};
        if (terminal != nullptr) {
            _consoleWriter = std::make_shared<log::impl::ConsoleLogWriter>(terminal);
            configuration.addWriter(
                _consoleWriter,
                log::LogWriterFilter{
                    log::LogLevels{log::LogLevel::Information, log::LogLevel::Warning, log::LogLevel::Error}});
        }
        _manager->setConfiguration(std::move(configuration));
    }
    return *_manager;
}

void ApplicationLogData::enableLastErrorDump(log::LogManager &manager, const LastErrorDumpMode mode) {
    const auto lock = std::scoped_lock{_mutex};
    _dumpMode = mode;
    if (_lastErrorsWriter != nullptr) {
        return;
    }
    _lastErrorsWriter = std::make_shared<log::impl::LastErrorsLogWriter>();
    manager.addPersistentWriter(_lastErrorsWriter, log::LogWriterFilter{log::LogLevel::Error});
}

void ApplicationLogData::cleanup(
    const unit::ExitCode exitCode,
    const i18n::DisplayTextMapConstPtr &displayText,
    ApplicationTerminalData *terminalData) noexcept {
    try {
        const auto lock = std::scoped_lock{_mutex};
        if (_manager == nullptr) {
            return;
        }
        _manager->shutdown();
        if (_lastErrorsWriter != nullptr && _consoleWriter != nullptr && terminalData != nullptr &&
            (_dumpMode == LastErrorDumpMode::Always || exitCode.isFailure())) {
            const auto lineFormat = _manager->configuration().lineFormat();
            const auto entries = _lastErrorsWriter->snapshot();
            if (!entries.empty()) {
                auto title = text::TextDocument{};
                title.addHeading(2)->addText(displayText->text("log.LastErrorDumpTitle"_el));
                terminalData->renderSystemOutput(title);
                for (const auto &entry : entries) {
                    _consoleWriter->write(entry, log::impl::LogLineFormatter{*entry, lineFormat}.format());
                }
                _consoleWriter->flush();
            }
        }
        _lastErrorsWriter.reset();
        _consoleWriter.reset();
        _manager.reset();
    } catch (...) {}
}

}
