// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Container.hpp"
#include "ValueMap.hpp"

#include "../../Document.hpp"

namespace erbsland::conf::impl {

/// Concrete mutable implementation of a configuration document.
class Document final : public conf::Document, public Container {
public:
    // defaults
    Document() = default;
    ~Document() override = default;

public: // implement Value
    [[nodiscard]] auto name() const noexcept -> Name override;
    [[nodiscard]] auto namePath() const noexcept -> NamePath override;
    [[nodiscard]] auto hasParent() const noexcept -> bool override;
    [[nodiscard]] auto parent() const noexcept -> conf::ValuePtr override;
    [[nodiscard]] auto type() const noexcept -> ValueType override;
    [[nodiscard]] auto hasLocation() const noexcept -> bool override;
    [[nodiscard]] auto location() const noexcept -> Location override;
    void setLocation(const Location &newLocation) noexcept override;
    [[nodiscard]] auto wasValidated() const noexcept -> bool override;
    [[nodiscard]] auto validationRule() const noexcept -> vr::RulePtr override;
    [[nodiscard]] auto isDefaultValue() const noexcept -> bool override;
    [[nodiscard]] auto size() const noexcept -> std::size_t override;
    [[nodiscard]] auto hasValue(const NamePathLike &namePath) const noexcept -> bool override;
    [[nodiscard]] auto value(const NamePathLike &namePath) const noexcept -> conf::ValuePtr override;
    [[nodiscard]] auto valueOrThrow(const NamePathLike &namePath) const -> conf::ValuePtr override;
    [[nodiscard]] auto begin() const noexcept -> ValueIterator override;
    [[nodiscard]] auto end() const noexcept -> ValueIterator override;
    [[nodiscard]] auto asInteger() const noexcept -> Integer override;
    [[nodiscard]] auto asBoolean() const noexcept -> bool override;
    [[nodiscard]] auto asFloat() const noexcept -> Float override;
    [[nodiscard]] auto asText() const noexcept -> text::String override;
    [[nodiscard]] auto asDate() const noexcept -> time::Date override;
    [[nodiscard]] auto asTime() const noexcept -> time::Time override;
    [[nodiscard]] auto asTimeWithZone() const noexcept -> time::TimeWithZone override;
    [[nodiscard]] auto asDateTime() const noexcept -> time::DateTime override;
    [[nodiscard]] auto asBytes() const noexcept -> mem::ByteBlock override;
    [[nodiscard]] auto asCalendarDelta() const noexcept -> time::CalendarDelta override;
    [[nodiscard]] auto asRegEx() const noexcept -> re::RegExPtr override;
    [[nodiscard]] auto asValueList() const noexcept -> conf::ValueList override;
    [[nodiscard]] auto asIntegerOrThrow() const -> Integer override;
    [[nodiscard]] auto asBooleanOrThrow() const -> bool override;
    [[nodiscard]] auto asFloatOrThrow() const -> Float override;
    [[nodiscard]] auto asTextOrThrow() const -> text::String override;
    [[nodiscard]] auto asDateOrThrow() const -> time::Date override;
    [[nodiscard]] auto asTimeOrThrow() const -> time::Time override;
    [[nodiscard]] auto asTimeWithZoneOrThrow() const -> time::TimeWithZone override;
    [[nodiscard]] auto asDateTimeOrThrow() const -> time::DateTime override;
    [[nodiscard]] auto asBytesOrThrow() const -> mem::ByteBlock override;
    [[nodiscard]] auto asCalendarDeltaOrThrow() const -> time::CalendarDelta override;
    [[nodiscard]] auto asRegExOrThrow() const -> re::RegExPtr override;
    [[nodiscard]] auto asValueListOrThrow() const -> conf::ValueList override;
    [[nodiscard]] auto toTextRepresentation() const noexcept -> text::String override;

public: // implement `Document`
    [[nodiscard]] auto toFlatValueMap() const noexcept -> FlatValueMap override;

public: // implement `Container`
    void setParent(const conf::ValuePtr &parent) override;
    void addValue(const ValuePtr &childValue) override;

public: // implementation API
    /// Set the validation rule for this value.
    void setValidationRule(RulePtr rule) noexcept;
    /// Remove default values from direct children.
    void removeDefaultValues();

private:
    Location _location; ///< The location of the document.
    ValueMap _children; ///< The map with the child values.
    RulePtr _rule;      ///< The validation rule that was used when this value was validated.
};

}
