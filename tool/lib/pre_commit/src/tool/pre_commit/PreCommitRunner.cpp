// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PreCommitRunner.hpp"

#include "impl/TaskDisplay.hpp"
#include "impl/TaskLogWriter.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ElapsedTimer.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/PlainTextRenderer.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/TextDocument.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::pre_commit {

using namespace el::text::literals;

auto PreCommitRunner::run(const el::List<frame::PreCommitTaskPtr> &tasks) const -> el::ExitCode {
    auto &application = frame::app();
    auto display = impl::TaskDisplay{application.terminal()};
    const auto elapsed = el::ElapsedTimer{};
    const auto verbose = application.optionValues()->getFlag("verbose"_el);
    auto passed = std::size_t{};
    auto failed = std::size_t{};
    for (const auto &task : tasks) {
        auto writer = std::make_shared<impl::TaskLogWriter>(verbose);
        const auto manager = el::LogManager::create();
        const auto progress = frame::TaskProgress::create();
        const auto timer = el::ElapsedTimer{};
        auto diagnostic = el::DiagnosticConstPtr{};
        auto diagnosticText = el::String{};
        const auto stream = manager->createStream(el::join("pre_commit/"_el, task->identifier()));
        auto success = true;
        display.begin(task->title(), progress);
        try {
            manager->setConfiguration(application.loggingConfiguration(writer));
            application.preparePreCommitTask(*task, {stream, progress});
            task->execute();
            if (const auto *tool = dynamic_cast<const frame::Tool *>(task.get())) {
                success = tool->exitCode().isSuccess();
            }
        } catch (const el::RuntimeError &error) {
            diagnostic = error.diagnostic();
            diagnosticText = el::PlainTextRenderer{diagnostic->toTextDocument()}.build();
            success = false;
            stream->error(diagnosticText);
        }
        manager->shutdown();
        const auto statistics = manager->statistics();
        const auto incompleteLog =
            writer->hasTruncation() || statistics.droppedEntries != 0 || statistics.writerFailures != 0;
        success = success && !writer->hasErrors() && !incompleteLog;
        display.finish(success, writer->hasWarnings(), timer.elapsed());
        for (const auto &entry : writer->entries()) {
            // The retained structured exception is rendered once, with its full source context below.
            if (!diagnostic || entry->message() != diagnosticText) {
                display.message(*entry);
            }
        }
        if (diagnostic) {
            display.diagnostic(*diagnostic);
        }
        if (incompleteLog) {
            display.diagnostic(*el::ApplicationError{
                "Incomplete Task Log"_el, "Task output was truncated, dropped, or could not be delivered."_el}
                    .diagnostic());
        }
        if (success) {
            ++passed;
        } else {
            ++failed;
        }
    }
    display.summary(passed, failed, elapsed.elapsed());
    if (failed != 0) {
        return el::ExitCode::failure();
    }
    return el::ExitCode::success();
}

}
