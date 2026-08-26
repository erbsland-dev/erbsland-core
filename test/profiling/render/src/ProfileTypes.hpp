// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/EnvironmentOptions.hpp>
#include <erbsland/text/render/impl/CompiledLayout_fwd.hpp>
#include <erbsland/text/render/LayoutSource.hpp>

#include <cstdint>
#include <memory>

namespace app::render {

/// The measured renderer compilation stage.
enum class ProfileStage : std::uint8_t {
    Tokenize, ///< Tokenize the complete source.
    Compile,  ///< Compile the complete source into bytecode.
    Render,   ///< Render a precompiled immutable layout graph.
};

/// Checked expectations for one expanded corpus layout.
/// @notest{Covered by the render profiler coverage and smoke tests.}
struct CorpusValidation {
    std::uint64_t sourceBytes{};    ///< Expanded UTF-8 source bytes.
    std::uint64_t tokens{};         ///< Complete token count, including the end token.
    std::uint64_t bytecodeBytes{};  ///< Compiled program bytes.
    std::uint64_t tokenizeDigest{}; ///< Stable digest of the complete token stream.
    std::uint64_t compileDigest{};  ///< Stable digest of compiled program size and constants.
    std::uint64_t outputBytes{};    ///< Deterministic rendered output bytes.
    std::uint64_t renderDigest{};   ///< Stable digest of rendered output.
};

/// One stable checked-in layout-corpus descriptor.
/// @notest{Covered by the render profiler coverage and smoke tests.}
struct CorpusEntry {
    erbsland::String id;               ///< Stable corpus identifier.
    erbsland::String group;            ///< Human-readable workload group.
    erbsland::String sourcePath;       ///< Path relative to the corpus directory.
    erbsland::StringList features;     ///< Language features represented by this source.
    erbsland::StringList dependencies; ///< Additional graph source paths and logical names.
    std::uint32_t repetitions{1U};     ///< Source repetitions used to create a substantial workload.
    bool customDelimiters{false};      ///< Whether this source uses the custom benchmark delimiters.
    CorpusValidation validation;       ///< Checked deterministic expectations after expansion.
};

/// One loaded and expanded layout ready for measured work.
/// @notest{Covered by the render profiler smoke test.}
struct PreparedLayout {
    erbsland::String id;                         ///< Stable corpus identifier.
    erbsland::text::render::LayoutSource source; ///< Expanded immutable layout source.
    erbsland::StringMap<std::shared_ptr<const erbsland::text::render::LayoutSource>> sources; ///< Named sources.
    erbsland::text::render::Context context;            ///< Deterministic local render context.
    erbsland::text::render::EnvironmentOptions options; ///< Syntax options for this layout.
    CorpusValidation validation;                        ///< Checked deterministic expectations.
};

/// Deterministic measurements for one compiled dependency graph.
/// @notest{Covered by the render profiler smoke and determinism tests.}
struct CompiledGraphValidation {
    std::uint64_t layouts{};          ///< Unique compiled layouts.
    std::uint64_t bytecodeBytes{};    ///< Aggregate unique setup, body, and block bytecode bytes.
    std::uint64_t includes{};         ///< Static include descriptors.
    std::uint64_t inheritanceEdges{}; ///< Static inheritance edges.
    std::uint64_t blockPrograms{};    ///< Unique compiled block programs.
    std::uint64_t digest{};           ///< Stable graph digest.
};

}
