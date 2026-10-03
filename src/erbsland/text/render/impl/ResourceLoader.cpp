// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceLoader.hpp"

#include "../../../core/impl/application_data/ApplicationResourceData.hpp"
#include "../../../core/impl/ApplicationDataAccess.hpp"
#include "../../../err/ParameterFailure.hpp"
#include "../../../resource/ResourceError.hpp"
#include "../../../resource/Resources.hpp"
#include "../../AsciiCategory.hpp"
#include "../../Literals.hpp"
#include "../../StringFormat.hpp"
#include "../../StringList.hpp"
#include "../../StringSide.hpp"
#include "../../ToString.hpp"

namespace erbsland::text::render::impl {

using namespace err;
using namespace unit;
using namespace text::literals;

ResourceLoader::ResourceLoader(resource::ResourcesConstPtr resources, String identifier, String pathPrefix) :
    _resources{std::move(resources)}, _identifier{std::move(identifier)}, _pathPrefix{std::move(pathPrefix)} {
    validateIdentifier(_identifier);
    validatePathPrefix(_pathPrefix);
    if (_resources == nullptr) {
        const auto resourceData = core::impl::applicationData()->resources().get();
        _resources = resource::ResourcesConstPtr{resourceData, &resourceData->resources()};
    }
}

auto ResourceLoader::load(const String &layout) -> std::optional<LayoutSource> {
    const auto path = resourcePath(layout);
    if (!_resources->getInfo(_identifier, path).has_value()) {
        return std::nullopt;
    }
    auto sourceText = _resources->getTextOrThrow(_identifier, path);
    if (!sourceText.isEncodingValid()) {
        throw resource::ResourceError{
            resource::ResourceErrorCategory::InvalidData, "Compiled layout resource is not valid UTF-8."_el};
    }
    const auto origin = StringFormat{"resource:{}/{}"_el}.build(_identifier, path);
    const auto revision = text::toString(static_cast<uint64_t>(sourceText.toHash()));
    return LayoutSource{std::move(sourceText), origin, revision};
}

void ResourceLoader::validateIdentifier(const String &identifier) {
    if (identifier.isEmpty()) {
        throw ParameterFailure{"A layout resource identifier must be a portable ASCII token."_el, "identifier"_el};
    }
    const auto [first, rest] = identifier.slice(text::StringSide::Front);
    if (!first.isAsciiAlphanumeric() || !rest.containsOnly(text::AsciiCategory::DottedName)) {
        throw ParameterFailure{"A layout resource identifier must be a portable ASCII token."_el, "identifier"_el};
    }
}

void ResourceLoader::validatePathPrefix(const String &pathPrefix) {
    if (pathPrefix.isEmpty()) {
        return;
    }
    if (!pathPrefix.isEncodingValid()) {
        throw ParameterFailure{"A layout resource path prefix must be valid UTF-8."_el, "pathPrefix"_el};
    }
    auto componentBegin = ByteIndex::zero();
    auto position = ByteIndex::zero();
    const auto end = ByteIndex::end(pathPrefix.length());
    while (position < end) {
        const auto character = pathPrefix.charAt(position);
        if (character == U'\\') {
            throw ParameterFailure{
                "A layout resource path prefix must use portable '/' separators."_el, "pathPrefix"_el};
        }
        if (character == U'/') {
            const auto component = pathPrefix.slice(ByteRange{componentBegin, position});
            if (component.isEmpty() || component == "."_el || component == ".."_el) {
                throw ParameterFailure{
                    "A layout resource path prefix must not contain empty, '.' or '..' components."_el,
                    "pathPrefix"_el};
            }
            pathPrefix.advance(position);
            componentBegin = position;
            continue;
        }
        pathPrefix.advance(position);
    }
    const auto lastComponent = pathPrefix.slice(ByteRange{componentBegin, end});
    if (lastComponent.isEmpty() || lastComponent == "."_el || lastComponent == ".."_el) {
        throw ParameterFailure{
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
