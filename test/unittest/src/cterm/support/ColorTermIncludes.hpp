// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

// import all color term headers and libraries for the unittests.

#include <erbsland/block/all.hpp>
#include <erbsland/cterm/all.hpp>
#include <erbsland/cterm/impl/paragraph/LayoutResult.hpp>

// import everything into the global namespace
using namespace erbsland::cterm;

// Aliases
namespace block = erbsland::block;
namespace geometry = erbsland::geometry;
namespace paragraph = erbsland::cterm::impl::paragraph;
namespace termimpl = erbsland::cterm::impl;
