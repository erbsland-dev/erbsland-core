// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../engine/Engine.hpp"
#include "../error/InternalError.hpp"
#include "../Limits.hpp"
#include "../parser/PatternNode.hpp"
#include "../text/CharRange.hpp"
#include "../text/CharSequence.hpp"

#include <unordered_map>

namespace erbsland::re::impl {

using namespace text::literals;

/// The data collector for the compiler.
class DataCollector {
public:
    /// Create a new data collector.
    explicit DataCollector(PatternNodePtr rootNode, EngineDataPtr engineData) :
        _rootNode{std::move(rootNode)}, _data{std::move(engineData)} {

        ERBSLAND_CORE_RE_REQUIRE_SAFETY(_rootNode != nullptr, "Root node must not be null"_el);
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(_data != nullptr, "Engine data must not be null"_el);
    }

    // defaults and disable copy and move.
    ~DataCollector() = default;
    auto operator=(const DataCollector &) -> DataCollector & = delete;
    auto operator=(DataCollector &&) -> DataCollector & = delete;
    DataCollector(const DataCollector &) = delete;
    DataCollector(DataCollector &&) = default;

public: // api
    /// Collect all data from the pattern nodes and update the indexs in the tree.
    void collect() {
        _rootNode->traverse([this](PatternNode &node, int) -> void { collectDataFromNode(node); });
    }

private:
    /// Collect data from the pattern node for the engine.
    void collectDataFromNode(PatternNode &node) {
        if (node.isCharacterSequence()) {
            auto &data = std::get<node_data::CharacterSequence>(node.data());
            const auto &sequence = data.chars;
            // Only collect sequences that are longer than the maximum compiled length.
            if (sequence.size() > limits::minimumCharacterSequenceLength) {
                const auto it = _sequenceIndexMap.find(sequence);
                std::size_t index;
                auto &sequenceData = _data->sequenceData;
                if (it == _sequenceIndexMap.end()) {
                    if ((sequenceData.size() + sequence.size()) > limits::maximumCharacterSequenceLength) {
                        throw RegExError{
                            ErrorCategory::Limit,
                            "Failed to compile regular expression"_el,
                            "The compiled pattern exceeds the maximum character sequence length."_el};
                    }
                    index = sequenceData.size();
                    sequenceData.insert(sequenceData.end(), sequence.begin(), sequence.end());
                    _sequenceIndexMap.emplace(sequence, index);
                } else {
                    index = it->second;
                }
                data.dataIndex = static_cast<int32_t>(index);
            } else {
                data.dataIndex = -1; // not collected
            }
        } else if (node.isCharacterClass()) {
            auto &data = std::get<node_data::CharacterClass>(node.data());
            const auto &characterClass = data.characterClass;
            const auto it = _rangesIndexMap.find(characterClass);
            int32_t index = -1; // not collected
            if (!characterClass.isSingleChar()) {
                auto &charClassData = _data->charClassData;
                if (it == _rangesIndexMap.end()) {
                    if (charClassData.size() >= limits::maximumCharacterClassCount) {
                        throw RegExError{
                            ErrorCategory::Limit,
                            "Failed to compile regular expression"_el,
                            "The compiled pattern exceeds the maximum character class count."_el};
                    }
                    index = static_cast<int32_t>(charClassData.size());
                    charClassData.emplace_back(characterClass);
                    _rangesIndexMap.emplace(characterClass, index);
                } else {
                    index = static_cast<int32_t>(it->second);
                }
            }
            data.dataIndex = index;
        } else if (node.isGroup()) {
            auto &data = std::get<node_data::Group>(node.data());
            auto &captureGroupNames = _data->captureGroupNames;
            if (data.index > 0) {
                const auto groupIndex = static_cast<uint32_t>(captureGroupNames.size() + 1);
                // Overwrite the index in the node tree - this shouldn't change anything.
                data.index = groupIndex;
                // Add the group name or an empty string for each capture group in the pattern.
                if (data.name.isEmpty()) {
                    captureGroupNames.emplace_back();
                } else {
                    captureGroupNames.emplace_back(data.name);
                }
            }
            if (data.atomicGroupId != cNoAtomicGroupId) {
                _data->hasAtomicGroups = true;
            }
        } else if (node.isQuantifier()) {
            auto &data = std::get<node_data::Quantifier>(node.data());
            if (data.atomicGroupId != cNoAtomicGroupId) {
                _data->hasAtomicGroups = true;
            }
        }
    }

private:
    PatternNodePtr _rootNode; ///< The root node from the parser.
    std::unordered_map<CharSequence, std::size_t> _sequenceIndexMap;
    std::unordered_map<CharClass, std::size_t> _rangesIndexMap;
    EngineDataPtr _data; ///< Then engine data.
};

}
