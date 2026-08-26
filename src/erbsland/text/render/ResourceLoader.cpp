// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceLoader.hpp"

#include "impl/ResourceLoader.hpp"

namespace erbsland::text::render {

auto ResourceLoader::create(String identifier, String pathPrefix) -> LoaderPtr {
    return create({}, std::move(identifier), std::move(pathPrefix));
}

auto ResourceLoader::create(resource::ResourcesConstPtr resources, String identifier, String pathPrefix) -> LoaderPtr {
    return std::make_shared<impl::ResourceLoader>(std::move(resources), std::move(identifier), std::move(pathPrefix));
}

}
