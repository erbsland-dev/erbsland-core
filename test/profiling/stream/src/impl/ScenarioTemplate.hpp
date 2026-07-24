// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ScenarioTemplate_fwd.hpp"

#include "../ProfileTypes.hpp"

namespace app::stream::impl {

/// Unexpanded stream-profiler scenario configuration.
/// @notest{Covered by stream profiler configuration CTest entries.}
struct ScenarioTemplate {
    el::String name;
    std::vector<Direction> directions{Direction::Read};
    std::vector<FileType> fileTypes{FileType::Binary};
    std::vector<Method> methods;
    std::vector<ChunkMode> chunkModes{ChunkMode::Fixed};
    std::vector<Locality> localities{Locality::Hot};
    std::vector<el::StreamBuffering> buffering{el::StreamBuffering::Balanced};
    std::uint64_t fileSizeMinimum{8ULL * 1024ULL * 1024ULL};
    std::uint64_t fileSizeMaximum{8ULL * 1024ULL * 1024ULL};
    std::uint64_t chunkSizeMinimum{64ULL * 1024ULL};
    std::uint64_t chunkSizeMaximum{64ULL * 1024ULL};
    std::uint64_t backBufferLimit{};
    std::uint32_t weight{1U};
    bool allMethods{true};
};

}
