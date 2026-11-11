// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constants.hpp"
#include "SourceAndZipFile.hpp"

#include <erbsland/all_log.hpp>
#include <erbsland/all_path.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/LogPath.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathResolveMode.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringTransformations.hpp>

namespace demo {

using namespace el::text::literals;

/// This class manages all the path preparations and own the prepared source and ZIP file information.
class Preparation {
public:
    // defaults
    Preparation() = default;

public:
    /// Prepare the source and zip file list from the command line options.
    void prepare(const el::OptionValuesPtr &values) {
        const auto outputDirectory = validateOutputDirectory(values);
        const auto sourceDirectories = validateSourceDirectories(values);
        buildSourceAndZipFileList(outputDirectory, sourceDirectories);
        if (!outputDirectory.isEmpty()) {
            validateAllDestinations();
        }
    }
    /// Access the prepared list of source directories and ZIP file paths.
    [[nodiscard]] auto sourceAndZipFiles() const noexcept { return _sourceAndZipFiles; }

private:
    /// Validate the output directory.
    [[nodiscard]] auto validateOutputDirectory(const el::OptionValuesPtr &values) const -> el::Path {
        const static auto cErrorTitle = "Output directory validation failed"_el;
        if (auto text = values->getText(cOutputDirectoryOption); !text.isEmpty()) {
            try {
                auto outputDirectory = el::Path::fromNativeOrThrow(text).resolveOrThrow(el::PathResolveMode::Physical);
                if (!outputDirectory.info().isDirectory()) {
                    throw el::ApplicationError{cErrorTitle, "The provided output directory is not a directory"_el};
                }
                return outputDirectory;
            } catch (const el::ApplicationError &error) {
                throw error.withSourcePath(text); // we add the source path to each error.
            } catch (const el::PathError &) {
                throw el::ApplicationError{
                    // we wrap the path error in our more descriptive application error.
                    cErrorTitle,
                    "The path either does not exist or could not be resolved to a directory."_el,
                    std::current_exception()}
                    .withSourcePath(text);
            }
        }
        return {};
    }
    /// Validate all source directories.
    [[nodiscard]] auto validateSourceDirectories(const el::OptionValuesPtr &values) const -> el::PathList {
        const static auto cErrorTitle = "Source directory validation failed"_el;
        el::PathList sourceDirectories; // we keep track of all source directories to avoid duplicates.
        for (const auto &sourceText : values->getTextList(cSourceDirectoryOption)) {
            try {
                // resolving the path makes sure we know the directory exists and have its canonical path.
                auto sourcePath = el::Path::fromNativeOrThrow(sourceText).resolveOrThrow(el::PathResolveMode::Physical);
                // then we check if there is a directory at the resolved path.
                if (!sourcePath.info().isDirectory()) {
                    throw el::ApplicationError{cErrorTitle, "The provided path is not a directory"_el};
                }
                // finally we check if the directory is unique and add it to our set.
                if (sourceDirectories.contains(sourcePath)) {
                    throw el::ApplicationError{cErrorTitle, "This path was specified twice."_el};
                }
                sourceDirectories.append(sourcePath);
            } catch (const el::ApplicationError &error) {
                throw error.withSourcePath(sourceText); // we add the source path to each error.
            } catch (const el::PathError &) {
                // we wrap the path error in our more descriptive application error.
                throw el::ApplicationError{
                    cErrorTitle,
                    "The path either does not exist or could not be resolved to a directory."_el,
                    std::current_exception()}
                    .withSourcePath(sourceText);
            }
        }
        return sourceDirectories;
    }
    /// Validate every destination to be outside the source directories
    void validateAllDestinations() {
        const static auto cErrorTitle = "ZIP destination validation failed"_el;
        for (const auto &entry : _sourceAndZipFiles) {
            for (const auto &otherEntry : _sourceAndZipFiles) {
                if (entry.zipPath.isRelativeTo(otherEntry.source)) {
                    throw el::ApplicationError{
                        cErrorTitle,
                        "The resulting ZIP file path must be outside all specified source directories. "
                        "We found one ZIP file that is inside one of the source directories."_el}
                        .withSourcePath(entry.zipPath.toString());
                }
            }
        }
    }
    /// Build the list of source and ZIP file paths.
    void buildSourceAndZipFileList(const el::Path &outputDirectory, const el::PathList &sourceDirectories) {
        for (const auto &sourceDirectory : sourceDirectories) {
            auto basePath = outputDirectory;
            if (basePath.isEmpty()) {
                basePath = sourceDirectory.parent();
            }
            auto zipPath = uniqueZipName(basePath, sourceDirectory.name());
            auto logPath = logPathFromZipFile(zipPath);
            _sourceAndZipFiles.append({
                sourceDirectory,
                std::move(zipPath),
                std::move(logPath),
            });
        }
    }

    /// Create a log name from a ZIP name.
    /// @param zipPath The ZIP file path to base the name on.
    /// @return A valid log file name that does not contain invalid characters.
    [[nodiscard]] static auto logPathFromZipFile(const el::Path &zipPath) -> el::LogPath {
        static const auto invalidLogChars = el::CharSet::fromPattern("^-_a-z0-9"_el);
        auto name = el::toIdentifierNormalized(zipPath.stem()).removedAll(invalidLogChars);
        if (name.characterLength() < el::CpCount{3}) {
            static auto count = 1;
            static const auto genericResultFormat = el::StringFormat("result_{:number:w3,fl=0}"_el);
            name = genericResultFormat.build(count++);
        }
        return el::LogPath{name};
    }
    /// Create a unique name for the zip file based on the source and counter.
    /// @param baseDirectory The base directory where the ZIP file will be created.
    /// @param sourceName The name of the source file.
    /// @return A unique ZIP path that does not exist yet.
    [[nodiscard]] auto uniqueZipName(const el::Path &baseDirectory, const el::String &sourceName) -> el::Path {
        const static auto zipNameFormat = el::StringFormat("{}_{:number:w3,fl=0}.zip"_el);
        auto zipPath = baseDirectory.joinedNative(el::join(sourceName, ".zip"_el));
        int counter = 1;
        while (hasZipPath(zipPath) || zipPath.info().exists()) {
            zipPath = baseDirectory.joinedNative(zipNameFormat.build(sourceName, counter));
            ++counter;
            if (counter >= 1000) {
                throw el::ApplicationError{
                    "Failed to find a unique ZIP file name"_el,
                    "All tested paths existed at the destination. "
                    "This message has the last tried path attached."_el}
                    .withSourcePath(zipPath.toString());
            }
        }
        return zipPath;
    }
    /// Test if a ZIP path was already defined.
    /// @param zipPath The ZIP path to test for.
    /// @return True if the ZIP file already exists, false otherwise.
    [[nodiscard]] auto hasZipPath(const el::Path &zipPath) -> bool {
        for (const auto &entry : _sourceAndZipFiles) {
            if (entry.zipPath == zipPath) {
                return true;
            }
        }
        return false;
    }

private:
    SourceAndZipFileList _sourceAndZipFiles; ///< The prepared list of source and zip files.
};

}
