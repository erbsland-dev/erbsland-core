// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextPreviewApp.hpp"

#include <erbsland/all_stream.hpp>
#include <erbsland/all_time.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/EventLoopErrorAction.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringTransformations.hpp>

#include <exception>
#include <utility>

namespace demo {

using namespace el::text::literals;

void TextPreviewApp::initialize() {
    info().setApplicationName("Text Preview"_el);
}

void TextPreviewApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->addOption({"--scenario"_el, "scenario"_el})
        .setType(el::OptionType::Text)
        .setDefaultValue("replacement"_el)
        .setHelp("Scenario: single, replacement, failure, or missing."_el);
}

auto TextPreviewApp::main() -> el::ExitCode {
    _scenario = optionValues()->getText("scenario"_el);
    if (_scenario != "single"_el && _scenario != "replacement"_el && _scenario != "failure"_el &&
        _scenario != "missing"_el) {
        throw el::ApplicationError{"Unknown preview scenario."_el};
    }
    _worker = createEventThread(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication));
    _worker->start();
    eventLoop().setErrorHandler(
        [](std::exception_ptr) -> el::EventLoopErrorAction { return el::EventLoopErrorAction::Stop; });
    events()->invoke([this]() -> void { begin(); });
    const auto exitCode = runEventLoop();
    if (const auto error = eventLoop().takeError()) {
        std::rethrow_exception(error);
    }
    // Missing completion must not look like a successful application exit.
    auto expectedReplies = 1;
    auto expectedPreview = "FIRST DRAFT"_el;
    if (_scenario == "replacement"_el) {
        expectedReplies = 2;
        expectedPreview = "REVISED DRAFT"_el;
    }
    if (_received != expectedReplies || _accepted != 1 || _preview != expectedPreview) {
        throw el::ApplicationError{"The preview exchange did not complete correctly."_el};
    }
    return exitCode;
}

void TextPreviewApp::begin() {
    _deadline = events()->createTimer(
        []() -> void { throw el::ApplicationError{"Timed out waiting for preview replies."_el}; });
    _deadline->startOnce(el::Seconds{1});
    if (_scenario == "failure"_el) {
        _worker->events()->invoke([]() -> void { throw el::ApplicationError{"Preview worker failed."_el}; });
        return;
    }
    if (_scenario == "missing"_el) {
        return;
    }
    // Submit initial input, then optionally replace it before main can dispatch a reply.
    requestPreview("First draft"_el);
    if (_scenario == "replacement"_el) {
        requestPreview("Revised draft"_el);
    }
}

void TextPreviewApp::requestPreview(el::String text) {
    const auto requestId = ++_requestId;
    // Bind the reply on main before sending work to another loop.
    auto reply =
        el::bindEvent([this, requestId](el::String preview) -> void { receivePreview(requestId, std::move(preview)); });
    _worker->events()->invoke([reply = std::move(reply), text = std::move(text)]() -> void {
        // Worker uses only captured input; main owns every application-state update.
        reply(el::toUppercase(text));
    });
}

void TextPreviewApp::receivePreview(const std::uint64_t requestId, el::String preview) {
    if (el::currentEvents() != events()) {
        throw el::LogicFailure{"Preview delivered to the wrong loop."_el};
    }
    ++_received;
    if (requestId == _requestId) {
        _preview = std::move(preview);
        ++_accepted;
        el::io::printLine("Accepted preview: "_el, _preview);
    } else {
        el::io::printLine("Ignored obsolete preview."_el);
    }
    if (_received == static_cast<int>(_requestId)) {
        _deadline->stop();
        quit();
    }
}

void TextPreviewApp::cleanup() noexcept {
    _deadline.reset();
}

}
