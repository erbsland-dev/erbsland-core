// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constants.hpp"
#include "FileWithDigest.hpp"

#include <erbsland/all_compression_zip.hpp>
#include <erbsland/all_path.hpp>
#include <erbsland/AnyStringBuilder.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/LogStream.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringEncoding.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/zip/ArchiveWriter.hpp>

#include <atomic>
#include <exception>
#include <functional>
#include <thread>
#include <utility>

namespace demo {

using namespace el::text::literals;

/// Verify scanned file contents and publish a ZIP with a SHA3-256 manifest.
/// Each file is buffered within the demo limit so its checksum describes the exact bytes archived.
class Packer final {
public:
    /// The callback if the packing is done.
    /// On success, nullptr is passed to indicate that no failures occurred.
    /// On failure, a std::exception_ptr containing an exception is passed to indicate why packing failed.
    using OnDone = std::function<void(std::exception_ptr)>;

public:
    /// Create a new packer instance.
    /// @param sourceDirectory The source directory, to get the relative paths for the zip entries.
    /// @param zipPath The path of the new zip file.
    /// @param logStream The logging stream to output messages to.
    Packer(el::Path sourceDirectory, el::Path zipPath, el::LogStreamPtr logStream) :
        _sourceDirectory{std::move(sourceDirectory)}, _zipPath{std::move(zipPath)}, _logStream{std::move(logStream)} {}

public: // worker lifecycle
    /// Pack a list of files into a given ZIP file, adding `checksum.txt` to each ZIP.
    /// This runs in a separate thread.
    /// @param fileList The list of files and their checksums.
    /// @param onDone Callback if the packing is done.
    void pack(FileWithDigestList fileList, OnDone onDone) {
        _workerThread = std::thread{[fileList = std::move(fileList), onDone = std::move(onDone), this]() -> void {
            packInThread(fileList, onDone);
        }};
    }
    /// Shutdown the workflow.
    void shutdown() {
        _shutdown = true;
        if (_workerThread.joinable()) {
            _workerThread.join();
        }
    }

private:
    /// Run packing in a separate thread.
    /// @param fileList The list of files and their checksums.
    /// @param onDone Callback if the packing is done.
    void packInThread(const FileWithDigestList &fileList, const OnDone &onDone) {
        if (_shutdown) {
            return;
        }
        const auto mainfestLineFormat = el::StringFormat{"{:bytes:} {}\n"};
        std::exception_ptr error;
        try {
            _logStream->trace("Start packing files"_el);
            auto archiveWriter = el::zip::ArchiveWriter::create(_zipPath);
            auto manifest = el::AnyStringBuilder::u8();
            for (const auto &entry : fileList) {
                if (_shutdown) {
                    return;
                }
                archiveWriter->addFile(entry.filePath);
                mainfestLineFormat.appendTo(manifest, entry.digest, entry.filePath.toRelative(_sourceDirectory));
            }
            archiveWriter->addData(
                el::StringEncoder{manifest.takeString()}.encode(el::StringEncoding::Utf8), el::Path{cChecksumFileName});
            if (_shutdown) {
                return;
            }
            archiveWriter->finalize();
            _logStream->trace("Packing finished"_el);
        } catch (const el::RuntimeError &) {
            error = std::current_exception();
        }
        if (!_shutdown) {
            onDone(std::move(error));
        }
    }

private:
    // read-only setup attributes:
    const el::Path _sourceDirectory;   ///< The source directory that is being worked on.
    const el::Path _zipPath;           ///< The path of the zip file.
    const el::LogStreamPtr _logStream; ///< The logging stream to output messages to.
    // variables owned by this part and worker thread:
    std::thread _workerThread;          ///< The worker thread for scanning and zipping files.
    std::atomic<bool> _shutdown{false}; ///< If a shutdown has been requested.
};

}
