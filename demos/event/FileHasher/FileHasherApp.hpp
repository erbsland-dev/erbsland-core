// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashResult.hpp"

#include <erbsland/all_core.hpp>
#include <erbsland/all_event.hpp>
#include <erbsland/all_options.hpp>
#include <erbsland/all_path.hpp>
#include <erbsland/all_stream.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/ManagedEventThread.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/system/SystemInfo.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace demo {

using namespace el::text::literals;

/// Combine a procedural main with worker loops and one result-receiver loop.
/// Only the receiver updates results while workers run; main reads them after joining it.
class FileHasherApp final : public el::Application {
    /// The identifier for the directory command line option.
    constexpr static auto cDirectoryOption = "directory"_el;

public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const erbsland::OptionValuesPtr &values) override;
    [[nodiscard]] auto main() -> el::ExitCode override {
        collectFiles();
        if (_totalFiles == 0) {
            el::io::printLine("No files to hash."_el);
            return el::ExitCode::success();
        }
        startLoops();
        el::io::printLine("Hashing "_el, _totalFiles, " files with "_el, _workers.size(), " worker loops."_el);
        distributePaths();
        _resultReceiver->join();
        if (_completedFiles != _totalFiles) {
            return el::ExitCode::failure();
        }
        printResults();
        return el::ExitCode::success();
    }

private:
    /// Start all worker loops and the receiver loop.
    void startLoops() {
        /// Prepare the result receiver.
        auto options = el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::FailApplication);
        _resultReceiver = createEventThread(options);
        _resultReceiver->start();
        /// Reserve either enough workers for all CPU cores or for every file if there are less.
        const auto count = std::min<std::size_t>(el::system::info::logicalCpuCount(), _paths.count().toSizeT());
        _workers.reserve(count);
        for (auto index = std::uint32_t{0}; index < count; ++index) {
            auto worker = createEventThread(options);
            _workers.push_back(worker);
            worker->start();
        }
    }
    /// Distribute the collected paths to all worker loops.
    void distributePaths() {
        for (auto index = el::ItemIndex{}; index.isWithin(_paths.count()); ++index) {
            _workers[index.toSizeT() % _workers.size()]->events()->invoke([this, path = _paths.get(index)]() -> void {
                auto result = hashFile(path);
                _resultReceiver->events()->invoke(
                    [this, result = std::move(result)]() mutable -> void { addResult(std::move(result)); });
            });
        }
    }
    /// Add a hash result to the results list and display progress.
    void addResult(HashResult result) {
        _results.push_back(std::move(result));
        ++_completedFiles;
        if (_completedFiles % 10U == 0) {
            el::io::print("."_el);
            el::stdOut()->flush();
        }
        if (_completedFiles == _totalFiles) {
            _resultReceiver->quit();
        }
    }
    /// Collect all paths from the selected root path.
    void collectFiles();
    /// Print the results.
    void printResults();
    /// Collect a list of files to process by recursive traversing the given directory.
    static auto collectFiles(const el::Path &root) -> el::PathList;
    /// Compute the hash result for a single file.
    static auto hashFile(const el::Path &path) -> HashResult;

private:
    std::vector<el::ManagedEventThreadPtr> _workers; ///< Workers retained throughout the batch.
    el::ManagedEventThreadPtr _resultReceiver;       ///< Sole owner of mutable result state while running.
    el::Path _root;                                  ///< The root path to be scanned.
    el::PathList _paths;                             ///< The list of paths to process.
    std::size_t _totalFiles{0};                      ///< Fixed before any thread starts.
    std::size_t _completedFiles{0};                  ///< Updated only on the receiver thread.
    std::vector<HashResult> _results;                ///< Read by main only after the receiver is joined.
};

}
