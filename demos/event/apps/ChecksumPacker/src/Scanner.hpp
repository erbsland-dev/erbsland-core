// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constants.hpp"
#include "FileWithDigest.hpp"

#include <erbsland/all_path.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteInputStream.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/LogStream.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathType.hpp>
#include <erbsland/PathWalkOptions.hpp>
#include <erbsland/PathWalkStatus.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/SymlinkMode.hpp>

#include <atomic>
#include <exception>
#include <functional>
#include <thread>
#include <utility>

namespace demo {

using namespace el::text::literals;

/// Scan regular files and calculate SHA3-256 checksums on a worker thread.
/// The owner cancels and joins the worker before releasing the scanner.
class Scanner final {
public:
    /// The callback if the scan is done.
    /// On success, nullptr is passed to indicate that no failures occurred.
    /// On failure, a std::exception_ptr containing an exception is passed to indicate why the scan failed.
    using OnDone = std::function<void(FileWithDigestList, std::exception_ptr)>;

public:
    /// Create a scanner.
    /// @param sourceDirectory The directory to scan recursively.
    /// @param logStream The stream for worker diagnostics.
    Scanner(el::Path sourceDirectory, el::LogStreamPtr logStream) :
        _sourceDirectory{std::move(sourceDirectory)}, _logStream{std::move(logStream)} {}

public: // worker lifecycle
    /// Start the scan operation in a background thread.
    /// @param onDone Callback to invoke when the scanning is finished (either successfully or with an error).
    void scan(OnDone onDone) {
        _workerThread = std::thread{[onDone = std::move(onDone), this]() -> void { scanInThread(onDone); }};
    }
    /// Shutdown the workflow.
    void shutdown() {
        _shutdown = true;
        if (_workerThread.joinable()) {
            _workerThread.join();
        }
    }

private:
    /// Scan all files and create their checksums.
    /// @param onDone Callback to invoke when the scanning is finished (either successfully or with an error).
    void scanInThread(const OnDone &onDone) {
        // This function runs in its own thread.
        std::exception_ptr error;
        try {
            _logStream->trace("Start scanning"_el);
            // Skip symlinks so a package stays within its selected source tree.
            auto options = el::PathWalkOptions{}
                               .setTypes(el::PathTypes{el::PathType::RegularFile})
                               .setSymlinkMode(el::SymlinkMode::Skip);
            _sourceDirectory.walker().walkOrThrow(
                [this](const el::Path &path) -> el::PathWalkStatus { return scanOneFile(path); }, options);
            _logStream->trace("Scanning done"_el);
        } catch (const el::RuntimeError &) {
            error = std::current_exception();
        }
        if (!_shutdown) {
            onDone(_entries, std::move(error));
        }
    }
    /// Read a single file and calculate its checksum.
    auto scanOneFile(const el::Path &path) -> el::PathWalkStatus {
        const static auto cErrorTitle = "Failed to read a file and calculate its checksum"_el;
        const auto relativePath = path.toRelativeOrThrow(_sourceDirectory);
        _logStream->trace("Calculating checksum for file: "_el, relativePath);
        if (_shutdown) {
            return el::PathWalkStatus::Stop;
        }
        try {
            if (relativePath.name() == cChecksumFileName) {
                throw el::ApplicationError{
                    cErrorTitle,
                    "The filename conflicts with the reserved 'checksum.txt' name."_el,
                };
            }
            // prepare the hash algorithm.
            el::Hasher hasher{el::HashAlgorithm::Sha3_256};
            // open the file for reading, using default options.
            auto inputStream = path.content().openByteInputStream();
            while (true) {
                if (_shutdown) {
                    return el::PathWalkStatus::Stop;
                }
                // try to read a good amount of bytes for each iteration.
                auto readResult = inputStream->read(el::ByteCount{0x10000U});
                if (readResult.isTimeout()) {
                    throw el::ApplicationError{"Timed out reading a file."_el}.withSourcePath(path.toString());
                }
                if (readResult.isFinished()) {
                    break;
                }
                hasher.update(readResult.takeData());
            }
            inputStream->close();
            auto digest = hasher.finalize();
            _entries.append(FileWithDigest{path, digest});
            return el::PathWalkStatus::Continue;
        } catch (const el::ApplicationError &error) {
            throw error.withSourcePath(path.toString());
        } catch (const el::RuntimeError &) {
            throw el::ApplicationError{
                cErrorTitle,
                "There was an unexpected error while reading a file."_el,
                std::current_exception(),
            }
                .withSourcePath(path.toString());
        }
    }

private:
    // read-only setup attributes:
    const el::Path _sourceDirectory;   ///< The source directory that is being worked on.
    const el::LogStreamPtr _logStream; ///< The log stream to write output to during the operation.
    // variables owned by this part and worker thread:
    std::thread _workerThread;          ///< The worker thread for scanning and zipping files.
    std::atomic<bool> _shutdown{false}; ///< If a shutdown has been requested.
    FileWithDigestList _entries;        ///< A list of found files and their checksums.
};

}
