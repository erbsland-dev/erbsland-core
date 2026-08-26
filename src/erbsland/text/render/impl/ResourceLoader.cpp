// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceLoader.hpp"

#include "../../../core/Application.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../resource/ResourceError.hpp"
#include "../../../resource/Resources.hpp"
#include "../../../text/AsciiCategory.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../text/StringList.hpp"
#include "../../../text/StringSide.hpp"
#include "../../../text/ToString.hpp"

namespace erbsland::text::render::impl {

using namespace text::literals;

ResourceLoader::ResourceLoader(resource::ResourcesConstPtr resources, String identifier, String pathPrefix) :
    _retainedResources{std::move(resources)},
    _resources{_retainedResources != nullptr ? _retainedResources.get() : &core::application().resources()},
    _identifier{std::move(identifier)},
    _pathPrefix{std::move(pathPrefix)} {
    validateIdentifier(_identifier);
    validatePathPrefix(_pathPrefix);
}

auto ResourceLoader::load(const String &layout) -> std::optional<LayoutSource> {
    const auto path = resourcePath(layout);
    if (!_resources->getInfo(_identifier, path).has_value()) {
        return std::nullopt;
    }
    auto sourceText = _resources->getTextOrThrow(_identifier, path);
    if (!sourceText.isValidUtf8()) {
        throw resource::ResourceError{
            resource::ResourceErrorCategory::InvalidData, "Compiled layout resource is not valid UTF-8."_el};
    }
    const auto origin = StringFormat{"resource:{}/{}"_el}.build(_identifier, path);
    const auto revision = text::toString(static_cast<uint64_t>(sourceText.toHash()));
    return LayoutSource{std::move(sourceText), origin, revision};
}

void ResourceLoader::validateIdentifier(const String &identifier) {
    if (identifier.isEmpty()) {
        throw err::ParameterError{"A layout resource identifier must be a portable ASCII token."_el, "identifier"_el};
    }
    const auto [first, rest] = identifier.slice(text::StringSide::Front);
    if (!first.isAsciiAlphanumeric() || !rest.containsOnly(text::AsciiCategory::DottedName)) {
        throw err::ParameterError{"A layout resource identifier must be a portable ASCII token."_el, "identifier"_el};
    }
}

void ResourceLoader::validatePathPrefix(const String &pathPrefix) {
    if (pathPrefix.isEmpty()) {
        return;
    }
    if (!pathPrefix.isValidUtf8()) {
        throw err::ParameterError{"A layout resource path prefix must be valid UTF-8."_el, "pathPrefix"_el};
    }
    auto componentBegin = unit::ByteIndex::zero();
    auto position = unit::ByteIndex::zero();
    const auto end = unit::ByteIndex::end(pathPrefix.length());
    while (position < end) {
        const auto character = pathPrefix.charAt(position);
        if (character == U'\\') {
            throw err::ParameterError{
                "A layout resource path prefix must use portable '/' separators."_el, "pathPrefix"_el};
        }
        if (character == U'/') {
            const auto component = pathPrefix.slice(unit::ByteRange{componentBegin, position});
            if (component.isEmpty() || component == "."_el || component == ".."_el) {
                throw err::ParameterError{
                    "A layout resource path prefix must not contain empty, '.' or '..' components."_el,
                    "pathPrefix"_el};
            }
            pathPrefix.advance(position);
            componentBegin = position;
            continue;
        }
        pathPrefix.advance(position);
    }
    const auto lastComponent = pathPrefix.slice(unit::ByteRange{componentBegin, end});
    if (lastComponent.isEmpty() || lastComponent == "."_el || lastComponent == ".."_el) {
        throw err::ParameterError{
            "A layout resource path prefix must not end with an empty, '.' or '..' component."_el, "pathPrefix"_el};
    }
}

auto ResourceLoader::resourcePath(const String &layout) const -> String {
    if (_pathPrefix.isEmpty()) {
        return layout;
    }
    return StringList{_pathPrefix, "/"_el, layout}.join();
}

}
