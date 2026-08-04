// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Document.hpp"

namespace erbsland::conf::impl {

/// A low-level interface to the storage of the document builder.
/// This class was introduced due to the complexity of the document builder - splitting its functionality into two
/// distinct layers that are individually testable.
class DocumentBuilderStorage {
public:
    // defaults
    DocumentBuilderStorage() = default;
    ~DocumentBuilderStorage() = default;

public:
    /// Reset the storage.
    void reset();

    /// Get the built document and reset the builder.
    [[nodiscard]] auto getDocumentAndReset() noexcept -> std::shared_ptr<Document>;

    /// Update the last updated section.
    void updateLastSection(const ValuePtr &sectionValue, const NamePath &sectionNamePath);

    /// Resolve a name path for adding a section.
    /// When an intermediate section is missing while resolving a name, it is automatically created.
    /// @param namePath The name path to resolve.
    /// @param location The location to set for intermediate paths and errors
    /// @return (parentValue, lastValue)
    [[nodiscard]] auto resolveForSection(const NamePath &namePath, const Location &location)
        -> std::pair<ValuePtr, ValuePtr>;

    /// Resolve a name path for adding a value.
    [[nodiscard]] auto resolveForValue(const NamePath &namePath, const Location &location) -> ValuePtr;

    /// Get a child value from the given container.
    /// @param container The container or nullptr to get the name from the document itself.
    /// @param name The name of the value.
    /// @return The value or nullptr if no such name exists.
    [[nodiscard]] auto getChildValue(const ValuePtr &container, const Name &name) const -> ValuePtr;

    /// Add a child value to the given container.
    /// - This also sets the parent of the value to the container.
    /// - This also sets the passed location to the value.
    /// - If a value has no name, the name is set to the current index in the container.
    /// @param containerValue The container value or nullptr to add the value to the document itself.
    /// @param namePath The name path of the value for error messages.
    /// @param location The location of the name that will be updated in `value` and used for error messages.
    /// @param value The value to add.
    void addChildValue(
        const ValuePtr &containerValue, const NamePath &namePath, const Location &location, const ValuePtr &value);

private:
    /// Validate the arguments accepted by an add operation.
    void validateAddArguments(const NamePath &namePath, const ValuePtr &value) const;
    /// Apply rules governing a value added at the document root.
    void applyRootRules(const NamePath &namePath, const Location &location, const ValuePtr &value);
    /// Apply rules governing a value added to a container.
    void applyContainerRules(
        const ValuePtr &container, const NamePath &namePath, const Location &location, const ValuePtr &value);

private:
    NamePath _lastSectionNamePath;
    ValuePtr _lastSectionValue;
    std::shared_ptr<Document> _document = std::make_shared<Document>();
};

}
