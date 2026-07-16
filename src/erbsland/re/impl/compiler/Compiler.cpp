// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Compiler.hpp"

#include "CodeGenerator.hpp"
#include "DataCollector.hpp"

#include "../error/InternalError.hpp"
#include "../parser/Parser.hpp"

#include <ranges>
#include <type_traits>
#include <unordered_map>

namespace erbsland::re::impl {

auto Compiler::buildEngine() -> EnginePtr {
    parse();
    _engineData = std::make_shared<EngineData>();
    collectData();
    generateCode();
    return Engine::create(_engineData, _settings);
}

void Compiler::parse() {
    Parser parser{_reader, _flags, _settings};
    _rootNode = parser.parse();
}

void Compiler::collectData() {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_engineData != nullptr, "No engine data"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_rootNode != nullptr, "No root node"_el);
    auto dataCollector = DataCollector{_rootNode, _engineData};
    dataCollector.collect();
}

void Compiler::generateCode() {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_engineData != nullptr, "No engine data"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_rootNode != nullptr, "No root node"_el);
    auto codeGenerator = CodeGenerator{_rootNode, _engineData};
    codeGenerator.generateCode();
}

}
