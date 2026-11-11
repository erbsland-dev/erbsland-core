// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Packer.hpp"
#include "Scanner.hpp"
#include "State.hpp"
#include "Workflow_fwd.hpp"

#include <erbsland/CurrentLoop.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/LogStream.hpp>
#include <erbsland/Path.hpp>

#include <exception>
#include <functional>
#include <memory>
#include <utility>

namespace demo {

using namespace el::text::literals;

/// Coordinate scanning and packing while keeping all state transitions on the main event loop.
/// Workers return captured results through events; the application owns the workflow until both workers join.
class Workflow final : public std::enable_shared_from_this<Workflow> {
public:
    /// The callback if the workflow is done.
    /// On success, nullptr is passed to indicate that no failures occurred.
    /// On failure, a std::exception_ptr containing an exception is passed to indicate why the workflow failed.
    using OnDone = std::function<void(std::exception_ptr)>;

public:
    /// Create a new workflow from a source directory.
    /// @param sourceDirectory Path to the source directory.
    /// @param zipPath The path for the final ZIP file.
    /// @param logStream The log stream to write output to.
    /// @param onDone Called on the main event loop after success or failure.
    explicit Workflow(el::Path sourceDirectory, el::Path zipPath, el::LogStreamPtr logStream, OnDone onDone) :
        _logStream{std::move(logStream)},
        _onDone{std::move(onDone)},
        _state{_logStream},
        _scanner{sourceDirectory, _logStream},
        _packer{std::move(sourceDirectory), std::move(zipPath), _logStream} {}

public: // events
    /// Start the workflow for this working set.
    void start() {
        _logStream->trace("Starting workflow"_el);
        _state.requireAndSet(State::Initialized, State::Scanning);
        auto events = el::currentLoop();
        auto workflow = shared_from_this();
        _scanner.scan([events, workflow](FileWithDigestList fileList, std::exception_ptr error) -> void {
            events->invoke([workflow, fileList = std::move(fileList), error = std::move(error)]() mutable -> void {
                workflow->onScanned(std::move(fileList), std::move(error));
            });
        });
    }
    /// Test if the workflow is done.
    [[nodiscard]] auto isDone() const noexcept -> bool { return _state.is(State::Done); }
    /// Shutdown the workflow.
    void shutdown() {
        _scanner.shutdown();
        _packer.shutdown();
    }

private:
    /// Receive the scan result on the main loop, joining the worker before starting packing.
    void onScanned(FileWithDigestList fileList, std::exception_ptr error) {
        _scanner.shutdown();
        if (error != nullptr) {
            _state.requireAndSet(State::Scanning, State::Failed);
            _onDone(std::move(error));
            return;
        }
        _state.requireAndSet(State::Scanning, State::Packing);
        auto events = el::currentLoop();
        auto workflow = shared_from_this();
        _packer.pack(std::move(fileList), [events, workflow](std::exception_ptr error) -> void {
            events->invoke(
                [workflow, error = std::move(error)]() mutable -> void { workflow->onPacked(std::move(error)); });
        });
    }
    /// Receive one packing result and notify the application after the worker has joined.
    void onPacked(std::exception_ptr error) {
        _packer.shutdown();
        if (error != nullptr) {
            _state.requireAndSet(State::Packing, State::Failed);
        } else {
            _state.requireAndSet(State::Packing, State::Done);
        }
        _onDone(std::move(error));
    }

private:
    // readonly setup data:
    const el::LogStreamPtr _logStream; ///< The log stream to write output to during the operation.
    const OnDone _onDone;              ///< Callback for when the workflow is done.
    // owned by this working set and its events:
    State _state; ///< The state of the working set.
    // the workflow components
    Scanner _scanner; ///< The scanner logic.
    Packer _packer;   ///< The packer logic.
};

}
