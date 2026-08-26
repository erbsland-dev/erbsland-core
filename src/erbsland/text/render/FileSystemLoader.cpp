// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileSystemLoader.hpp"

#include "impl/FileSystemLoader.hpp"

namespace erbsland::text::render {

auto FileSystemLoader::create(path::Path searchPath, FileSystemLoaderOptions options) -> LoaderPtr {
    return create(util::List{std::move(searchPath)}, std::move(options));
}

auto FileSystemLoader::create(util::List<path::Path> searchPaths, FileSystemLoaderOptions options) -> LoaderPtr {
    return std::make_shared<impl::FileSystemLoader>(std::move(searchPaths), std::move(options));
}

}
