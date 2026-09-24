// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveReader.hpp"

#include "ArchiveItem.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"

#include <algorithm>
#include <vector>

namespace erbsland::compression::zip::impl {

using namespace text::literals;

void ArchiveReader::extractToDirectory(const path::Path &root, ArchiveExtractionOptions options, Filter filter) const {
    if (!_isOpen) {
        throw err::LogicError{"The ZIP archive reader is closed."_el};
    }
    if (root.isEmpty()) {
        throw err::ParameterError{"The ZIP extraction root must not be empty."_el, "root"_el};
    }
    auto totalLength = unit::ByteLength{};
    auto selected = std::vector<std::shared_ptr<ArchiveItem>>{};
    for (const auto &itemValue : _items) {
        if (filter && !filter(*itemValue)) {
            continue;
        }
        if (totalLength.wouldAddSaturate(itemValue->uncompressedLength()) ||
            totalLength + itemValue->uncompressedLength() > _options.maximumBulkExtractionLength()) {
            throwArchiveError(
                ZipErrorReason::ResourceLimit,
                ZipOperationPhase::Extraction,
                "ZIP bulk extraction limit exceeded"_el,
                "The declared output of selected entries exceeds the configured total extraction limit."_el);
        }
        totalLength += itemValue->uncompressedLength();
        selected.push_back(itemValue);
    }

    // Prepare explicit directories in parent-first order, independent of central-directory ordering.
    std::stable_sort(selected.begin(), selected.end(), [](const auto &left, const auto &right) {
        if (left->isDirectory() != right->isDirectory()) {
            return left->isDirectory();
        }
        return left->path().toPosix() < right->path().toPosix();
    });
    auto directoriesToRestore = std::vector<std::shared_ptr<ArchiveItem>>{};
    for (const auto &itemValue : selected) {
        if (itemValue->isDirectory()) {
            if (itemValue->prepareDirectory(root, options)) {
                directoriesToRestore.push_back(itemValue);
            }
        } else {
            itemValue->extractToDirectory(root, options);
        }
    }

    // Children change parent timestamps; restore explicit directories from deepest to shallowest last.
    for (auto iterator = directoriesToRestore.rbegin(); iterator != directoriesToRestore.rend(); ++iterator) {
        (*iterator)->restoreDirectoryModificationTime(root, options);
    }
}

}
