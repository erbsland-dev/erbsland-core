// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceManager.hpp"

#include "ResourceError.hpp"

#include "impl/ResourceManagerData.hpp"

#include "../compression/ByteDecompressor.hpp"
#include "../compression/CompressionError.hpp"
#include "../text/impl/UnsafeU8StringBuffer.hpp"
#include "../text/Literals.hpp"
#include "../text/u8/impl/U8StringLiteralFactory.hpp"

namespace erbsland::resource {

using namespace mem;
using namespace text;
using namespace text::literals;

ResourceManager::ResourceManager() = default;

ResourceManager::~ResourceManager() = default;

auto ResourceManager::contains(const String &identifier, const String &path) const -> bool {
    return data().find(identifier, path) != nullptr;
}

auto ResourceManager::getStoredData(const String &identifier, const String &path) const
    -> std::optional<ConstByteSpan> {
    const auto state = data().find(identifier, path);
    if (state == nullptr) {
        return std::nullopt;
    }
    return state->entry.storedData.span();
}

auto ResourceManager::getStoredDataOrThrow(const String &identifier, const String &path) const -> ConstByteSpan {
    const auto result = getStoredData(identifier, path);
    if (!result.has_value()) {
        throw ResourceError{ResourceErrorCategory::NotFound, "Compiled resource was not found."_el};
    }
    return *result;
}

auto ResourceManager::getData(const String &identifier, const String &path) const -> std::optional<ByteBlock> {
    const auto state = data().find(identifier, path);
    if (state == nullptr) {
        return std::nullopt;
    }
    const auto lock = std::lock_guard{state->mutex};
    if (state->data.has_value()) {
        return state->data;
    }
    try {
        if (state->entry.compressionAlgorithm.has_value()) {
            const auto options = compression::DecompressionOptions{}
                                     .setExpectedOutputLength(state->entry.originalSize)
                                     .setMaximumOutputLength(state->entry.originalSize);
            const auto decompressor = compression::ByteDecompressor{
                *state->entry.compressionAlgorithm, compression::CompressionFormat::Raw, options};
            state->data = decompressor.decompress(state->entry.storedData);
        } else {
            state->data = state->entry.storedData;
        }
    } catch (const compression::CompressionError &) {
        return std::nullopt;
    }
    return state->data;
}

auto ResourceManager::getDataOrThrow(const String &identifier, const String &path) const -> ByteBlock {
    const auto state = data().find(identifier, path);
    if (state == nullptr) {
        throw ResourceError{ResourceErrorCategory::NotFound, "Compiled resource was not found."_el};
    }
    const auto result = getData(identifier, path);
    if (!result.has_value()) {
        throw ResourceError{ResourceErrorCategory::InvalidData, "Compiled resource data is invalid."_el};
    }
    return *result;
}

auto ResourceManager::getText(const String &identifier, const String &path) const -> std::optional<String> {
    const auto state = data().find(identifier, path);
    if (state == nullptr) {
        return std::nullopt;
    }
    const auto lock = std::lock_guard{state->mutex};
    if (state->text.has_value()) {
        return state->text;
    }
    if (!state->entry.compressionAlgorithm.has_value()) {
        if (state->entry.storedData.isEmpty()) {
            state->text = String{};
            return state->text;
        }
        const auto storedData = state->entry.storedData.span();
        const auto *characters = reinterpret_cast<const char *>(storedData.data());
        state->text = String{text::impl::createU8StringLiteral(characters, storedData.size())};
        return state->text;
    }
    try {
        const auto options = compression::DecompressionOptions{}
                                 .setExpectedOutputLength(state->entry.originalSize)
                                 .setMaximumOutputLength(state->entry.originalSize);
        const auto decompressor = compression::ByteDecompressor{
            *state->entry.compressionAlgorithm, compression::CompressionFormat::Raw, options};
        const auto bytes = decompressor.decompress(state->entry.storedData);
        state->text = text::impl::UnsafeU8StringBuffer{bytes.span()}.takeString();
    } catch (const compression::CompressionError &) {
        return std::nullopt;
    }
    return state->text;
}

auto ResourceManager::getTextOrThrow(const String &identifier, const String &path) const -> String {
    const auto state = data().find(identifier, path);
    if (state == nullptr) {
        throw ResourceError{ResourceErrorCategory::NotFound, "Compiled resource was not found."_el};
    }
    const auto result = getText(identifier, path);
    if (!result.has_value()) {
        throw ResourceError{ResourceErrorCategory::InvalidData, "Compiled resource text is invalid."_el};
    }
    return *result;
}

auto ResourceManager::getInfo(const String &identifier, const String &path) const -> std::optional<ResourceInfo> {
    const auto state = data().find(identifier, path);
    if (state == nullptr) {
        return std::nullopt;
    }
    const auto &entry = state->entry;
    return ResourceInfo{
        entry.originalSize,
        entry.storedData.length(),
        entry.compressionAlgorithm,
        entry.hashAlgorithm,
        entry.hash,
        entry.encrypted};
}

auto ResourceManager::getInfoOrThrow(const String &identifier, const String &path) const -> ResourceInfo {
    const auto result = getInfo(identifier, path);
    if (!result.has_value()) {
        throw ResourceError{ResourceErrorCategory::NotFound, "Compiled resource was not found."_el};
    }
    return *result;
}

auto ResourceManager::data() const -> impl::ResourceManagerData & {
    const auto lock = std::lock_guard{_mutex};
    if (_data == nullptr) {
        _data = std::make_unique<impl::ResourceManagerData>();
    }
    return *_data;
}

}
