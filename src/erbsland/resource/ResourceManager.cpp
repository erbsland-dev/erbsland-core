// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceManager.hpp"

#include "ResourceError.hpp"

#include "impl/ResourceManagerData.hpp"

#include "../mem/ByteCompressionError.hpp"
#include "../mem/ByteDecompressor.hpp"
#include "../text/Literals.hpp"
#include "../text/u8/impl/U8StringLiteralFactory.hpp"

#include <string_view>

namespace erbsland::resource {

using namespace text::literals;

ResourceManager::ResourceManager() = default;

ResourceManager::~ResourceManager() = default;

auto ResourceManager::contains(const text::String &identifier, const text::String &path) const -> bool {
    return data().find(identifier, path) != nullptr;
}

auto ResourceManager::getStoredData(const text::String &identifier, const text::String &path) const
    -> std::optional<mem::ConstByteSpan> {
    const auto state = data().find(identifier, path);
    if (state == nullptr) {
        return std::nullopt;
    }
    return state->entry.storedData;
}

auto ResourceManager::getStoredDataOrThrow(const text::String &identifier, const text::String &path) const
    -> mem::ConstByteSpan {
    const auto result = getStoredData(identifier, path);
    if (!result.has_value()) {
        throw ResourceError{ResourceErrorCategory::NotFound, "Compiled resource was not found."_el};
    }
    return *result;
}

auto ResourceManager::getData(const text::String &identifier, const text::String &path) const
    -> std::optional<mem::ByteBlock> {
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
            const auto decompressor = mem::ByteDecompressor{*state->entry.compressionAlgorithm};
            state->data = decompressor.decompress(state->entry.storedData, state->entry.originalSize);
        } else {
            state->data = mem::ByteBlock::fromSpan(state->entry.storedData);
        }
    } catch (const mem::ByteCompressionError &) {
        return std::nullopt;
    }
    return state->data;
}

auto ResourceManager::getDataOrThrow(const text::String &identifier, const text::String &path) const -> mem::ByteBlock {
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

auto ResourceManager::getText(const text::String &identifier, const text::String &path) const
    -> std::optional<text::String> {
    const auto state = data().find(identifier, path);
    if (state == nullptr) {
        return std::nullopt;
    }
    const auto lock = std::lock_guard{state->mutex};
    if (state->text.has_value()) {
        return state->text;
    }
    if (!state->entry.compressionAlgorithm.has_value()) {
        if (state->entry.storedData.empty()) {
            state->text = text::String{};
            return state->text;
        }
        const auto *characters = reinterpret_cast<const char *>(state->entry.storedData.data());
        state->text = text::String{text::impl::createU8StringLiteral(characters, state->entry.storedData.size())};
        return state->text;
    }
    try {
        const auto decompressor = mem::ByteDecompressor{*state->entry.compressionAlgorithm};
        const auto bytes = decompressor.decompress(state->entry.storedData, state->entry.originalSize);
        const auto span = bytes.span();
        state->text = text::String{std::string_view{reinterpret_cast<const char *>(span.data()), span.size()}};
    } catch (const mem::ByteCompressionError &) {
        return std::nullopt;
    }
    return state->text;
}

auto ResourceManager::getTextOrThrow(const text::String &identifier, const text::String &path) const -> text::String {
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

auto ResourceManager::getInfo(const text::String &identifier, const text::String &path) const
    -> std::optional<ResourceInfo> {
    const auto state = data().find(identifier, path);
    if (state == nullptr) {
        return std::nullopt;
    }
    const auto &entry = state->entry;
    return ResourceInfo{
        entry.originalSize,
        unit::ByteLength::fromSizeT(entry.storedData.size()),
        entry.compressionAlgorithm,
        entry.hashAlgorithm,
        entry.hash,
        entry.encrypted};
}

auto ResourceManager::getInfoOrThrow(const text::String &identifier, const text::String &path) const -> ResourceInfo {
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
