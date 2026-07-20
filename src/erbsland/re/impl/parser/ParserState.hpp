// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PatternNode.hpp"

#include "../error/InternalError.hpp"

#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../text/StringSet.hpp"
#include "../../RegExError.hpp"
#include "../../Settings.hpp"

#include <unordered_set>

namespace erbsland::re::impl {

/// A class with the parser state to allow splitting the implementation into regular functions.
class ParserState {
public:
    /// Create a new parser state for the given pattern.
    explicit ParserState(text::StringCharReader reader, const GroupFlags flags, Settings &&settings) :
        _reader{std::move(reader)}, _settings{std::move(settings)} {

        // Create the root structure with a group and sequence.
        _rootNode = std::make_shared<PatternNode>(0, node_data::Group::createNonCapturing(flags));
        pushNode(_rootNode);
    }

    // defaults: allow move, disallow copy.
    ParserState(const ParserState &) = delete;
    ParserState(ParserState &&) = default;
    auto operator=(const ParserState &) -> ParserState & = delete;
    auto operator=(ParserState &&) -> ParserState & = default;
    ~ParserState() = default;

public: // accessors
    /// Access the settings.
    [[nodiscard]] auto settings() const noexcept -> const Settings & { return _settings; }

    /// Test if a feature is enabled.
    [[nodiscard]] auto hasFeature(const Feature feature) const noexcept -> bool {
        return _settings.hasFeature(feature);
    }

public: // reading characters
    /// Throw a parsing error.
    [[noreturn]] void throwParsingError(text::String description) const {
        throw RegExError{
            ErrorCategory::Parser,
            "Failed to parse regular expression"_el,
            std::move(description),
            unit::CodeLocation{}.setPosition(currentCharPosition())};
    }

    /// Reject an unsupported character in the pattern at the current location.
    void validatePatternCharacter(const text::Char character) const {
        validatePatternCharacter(character, currentCharPosition());
    }

    /// Get the next character from the pattern.
    [[nodiscard]] auto next() -> text::Char {
        if (!_reader.position().isWithin(_settings.maximumPatternLength())) {
            // important: This isn't redundant.
            throwParsingError("Maximum pattern length exceeded"_el);
        }
        const auto position = _reader.position();
        const auto character = _reader.read();
        validatePatternCharacter(character, position);
        return character;
    }

    /// Read the next character and make it the current one.
    void readNext() { _currentChar = next(); }

    /// Read the next character, returning the current one.
    [[nodiscard]] auto readNextAndExchange() -> text::Char {
        const auto result = _currentChar;
        readNext();
        return result;
    }

    /// Access the current character.
    [[nodiscard]] auto currentChar() const noexcept -> text::Char { return _currentChar; }

    /// Access the current character position
    [[nodiscard]] auto currentCharPosition() const noexcept -> unit::CpIndex {
        auto result = _reader.position();
        if (!_currentChar.isSignal()) {
            result -= unit::CpLength::one();
        }
        return result;
    }

    /// Test if at least the given number of pattern characters are available.
    [[nodiscard]] auto canRead(const unit::CpLength count) const noexcept -> bool { return _reader.canRead(count); }

    /// Access the current sequence.
    [[nodiscard]] auto currentSequence() const noexcept -> const PatternNodePtr & { return _currentSequence; }

    /// Access the current group.
    [[nodiscard]] auto currentGroup() const noexcept -> const PatternNodePtr & { return _groupStack.back(); }

    /// Access the current active flags.
    [[nodiscard]] auto currentFlags() const noexcept -> GroupFlags {
        return std::get<node_data::Group>(currentGroup()->data()).flags;
    }

    // Inherit the group flags for a nested group.
    [[nodiscard]] auto inheritGroupFlags() const noexcept -> GroupFlags {
        auto flags = currentFlags();
        flags.clear(GroupFlag::Atomic);
        return flags;
    }

    /// Access the root node.
    [[nodiscard]] auto rootNode() const noexcept -> const PatternNodePtr & { return _rootNode; }

    /// Test if we reached the end.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool { return _currentChar.isEndOfData(); }

public: // Node and group handling.
    /// Get the next capture group index.
    /// @return The capture group index (starting at 1).
    [[nodiscard]] auto nextCaptureGroupIndex() -> uint32_t {
        // Capture group indices start at 1. Index 0 in node_data::Group means "no capture".
        _groupIndex += 1;
        if (_groupIndex > _settings.maximumCaptureGroupCount()) {
            throwParsingError(
                text::StringFormat{"Maximum capture group count of {} exceeded"}.build(
                    _settings.maximumCaptureGroupCount()));
        }
        return _groupIndex;
    }

    /// Get the next available atomic group index.
    auto nextAtomicGroupId() -> AtomicGroupId {
        if (_atomicGroupIndex == cNoAtomicGroupId) {
            _atomicGroupIndex = 0;
        } else {
            _atomicGroupIndex += 1;
        }

        if (_atomicGroupIndex >= limits::maximumAtomicGroupCount) {
            throwParsingError(
                text::StringFormat{
                    "Maximum atomic group count of {} exceeded. Please reduce the complexity of the pattern."}
                    .build(limits::maximumAtomicGroupCount));
        }
        return _atomicGroupIndex;
    }

    /// Add a group name and throw an error on duplicates.
    void addGroupName(const text::String &name) {
        if (_groupNames.contains(name)) {
            throwParsingError("Duplicate group name"_el);
        }
        _groupNames.insert(name);
    }

    // The low-level method to push a node on the group stack.
    void pushNode(const PatternNodePtr &node) {
        // As _groupStack also contains the implicit root group, add one to the maximum.
        if (_groupStack.size() >= (_settings.maximumGroupNestingDepth() + 1)) {
            throwParsingError(
                text::StringFormat{"Maximum group nesting depth of {} levels exceeded"}.build(
                    _settings.maximumGroupNestingDepth()));
        }
        _groupStack.push_back(node);
        if (_currentSequence != nullptr) {
            _currentSequence->addChild(node);
        }
        // Add the first sequence to the new group.
        _currentSequence = createNode(node_data::Sequence{});
        node->addChild(_currentSequence);
    }

    /// Push a new group to the stack and add it to the current sequence.
    template <typename Fwd>
        requires std::derived_from<std::remove_cvref_t<Fwd>, node_data::Group>
    void pushGroup(Fwd &&groupData) {
        pushNode(createNode(std::forward<Fwd>(groupData)));
    }

    auto popGroup() -> PatternNodePtr {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(!_groupStack.empty(), "Popping group from empty group stack"_el);
        auto result = currentGroup();
        _groupStack.pop_back();
        _currentSequence = currentGroup()->children().back();
        return result;
    }

    /// Access the last added node of the sequence.
    [[nodiscard]] auto lastNode() const noexcept -> PatternNodePtr {
        if (_currentSequence->isEmpty()) {
            return nullptr;
        }
        return _currentSequence->children().back();
    }

    /// Replace the last node
    void replaceLastNode(const PatternNodePtr &node) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(lastNode() != nullptr, "Cannot replace last node in empty sequence"_el);
        if (lastNode() != node) {
            _currentSequence->replaceLastChild(node);
        }
    }

    /// Add a node to the current sequence
    void addNode(const PatternNodePtr &node) { _currentSequence->addChild(node); }

    template <typename Fwd>
        requires std::derived_from<std::remove_cvref_t<Fwd>, node_data::NodeData>
    [[nodiscard]] auto createNode(Fwd &&data) -> PatternNodePtr {
        _nextNodeId += 1;
        return std::make_shared<PatternNode>(_nextNodeId, PatternNode::Data{std::forward<Fwd>(data)});
    }

    /// Add data to the current sequence.
    template <typename Fwd>
        requires std::derived_from<std::remove_cvref_t<Fwd>, node_data::NodeData>
    void addNodeData(Fwd &&data) {
        addNode(createNode(std::forward<Fwd>(data)));
    }

    /// Add a new sequence to the current group
    void addSequence() {
        _currentSequence = createNode(node_data::Sequence{});
        currentGroup()->addChild(_currentSequence);
    }

    /// Check if the current alternative is empty and throw an error if not allowed.
    /// @param allowEmptyGroup If true, an empty alternative is allowed if it is the only one in the group (empty
    /// group).
    void checkEmptyAlternative(bool allowEmptyGroup = false) const {
        if (_currentSequence->isEmpty() && !hasFeature(Feature::EmptyAlternatives)) {
            if (allowEmptyGroup && currentGroup()->size() == 1) {
                return;
            }
            throwParsingError(
                "Empty alternatives are not allowed. Use '?' for optionality instead (e.g., '(?:a|b)?')"_el);
        }
    }

    /// Check if the current group is empty and throw an error if not allowed.
    void checkEmptyGroup() const {
        if (currentGroup()->size() == 1 && _currentSequence->isEmpty() && !hasFeature(Feature::EmptyGroups)) {
            throwParsingError("Empty groups are not allowed"_el);
        }
    }

private:
    void validatePatternCharacter(const text::Char character, const unit::CpIndex position) const {
        if (character.isNull() && !hasFeature(Feature::AcceptNullInPattern)) {
            throw RegExError{
                ErrorCategory::Parser,
                "Failed to parse regular expression"_el,
                "Null characters are disabled in regular expression patterns."_el,
                unit::CodeLocation{}.setPosition(position)};
        }
    }

    PatternNodeId _nextNodeId{0};
    text::StringCharReader _reader;
    Settings _settings;
    text::Char _currentChar{text::Char::noCodePoint()};
    PatternNodePtr _rootNode;                          ///< The root node of the parsed pattern.
    PatternNodePtr _currentSequence;                   ///< The current sequence.
    std::vector<PatternNodePtr> _groupStack;           ///< A stack with all open groups.
    uint32_t _groupIndex{0};                           ///< The current group index.
    AtomicGroupId _atomicGroupIndex{cNoAtomicGroupId}; ///< The current atomic group index.
    text::StringSet _groupNames;                       ///< All group names.
};

}
