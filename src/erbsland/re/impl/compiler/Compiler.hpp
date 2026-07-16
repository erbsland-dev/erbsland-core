// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../engine/Engine.hpp"
#include "../node_data/GroupFlags.hpp"
#include "../parser/PatternNode.hpp"

#include "../../../text/StringCharReader.hpp"
#include "../../Settings.hpp"

namespace erbsland::re::impl {

/// A temporary class representing the compilation process.
class Compiler {
public:
    /// Create a new instance.
    /// @param reader The reader for the pattern to parse.
    /// @param flags The initial group flags for the pattern.
    /// @param settings The settings for the compilation process.
    Compiler(text::StringCharReader reader, const GroupFlags flags, Settings settings) :
        _reader{std::move(reader)}, _flags{flags}, _settings{std::move(settings)} {}

    // defaults and disable copy and move.
    ~Compiler() = default;
    auto operator=(const Compiler &) -> Compiler & = delete;
    auto operator=(Compiler &&) -> Compiler & = delete;
    Compiler(const Compiler &) = delete;
    Compiler(Compiler &&) = default;

public:
    /// Parse, collect the data, and generate the engine.
    [[nodiscard]] auto buildEngine() -> EnginePtr;

private:
    /// Parse the pattern into a tree.
    void parse();
    /// Collect all character and character class data into continuous data segments.
    void collectData();
    /// Generate the engine code from the parsed pattern tree.
    void generateCode();

private:
    text::StringCharReader _reader;
    GroupFlags _flags;
    Settings _settings;
    PatternNodePtr _rootNode;
    EngineDataPtr _engineData;
    EnginePtr _engine;
};

}
