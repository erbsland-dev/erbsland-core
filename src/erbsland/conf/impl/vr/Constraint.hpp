// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint_fwd.hpp"
#include "ValidationContext_fwd.hpp"

#include "../../Value.hpp"
#include "../../vr/Constraint.hpp"

namespace erbsland::conf::impl {

/// The implementation of the constraint interface.
class Constraint : public vr::Constraint {
public:
    /// Create a constraint implementation with its immutable kind.
    /// @param type The public constraint kind.
    explicit Constraint(vr::ConstraintType type) : _type{type} {}

    // defaults
    ~Constraint() override = default;

public: // implement vr::Constraint
    [[nodiscard]] auto name() const -> text::String override;
    [[nodiscard]] auto type() const -> vr::ConstraintType override;
    [[nodiscard]] auto hasCustomError() const -> bool override;
    [[nodiscard]] auto customError() const -> text::String override;
    [[nodiscard]] auto isNegated() const -> bool override;
    [[nodiscard]] auto hasLocation() const noexcept -> bool override;
    [[nodiscard]] auto location() const noexcept -> const Location & override;
    void setLocation(const Location &newLocation) noexcept override;

    // the internal interface.
    /// Validate a value using a context.
    virtual void validate(const ValidationContext &context) const;
    /// Set the name of this constraint.
    /// @param name The new name.
    void setName(text::String name);
    /// Set a custom error message for this constraint.
    /// @param errorMessage The new error message.
    void setErrorMessage(text::String errorMessage);
    /// Set this constraint as negated.
    /// @param isNegated Whether to negate the constraint.
    void setNegated(bool isNegated);
    /// Test if this constraint came from a template
    [[nodiscard]] auto isFromTemplate() const -> bool;
    /// Set this constraint came from a template.
    /// @param isFromTemplate Whether this constraint came from a template.
    void setFromTemplate(bool isFromTemplate);

private:
    /// Validate the value target for this context.
    void validateValue(const ValidationContext &context) const;
    /// Validate the name target for this context.
    void validateName(const ValidationContext &context) const;

protected:
    /// Validate an integer value.
    /// @param context The validation context to use.
    /// @param value The integer value to validate.
    virtual void validateInteger(const ValidationContext &context, Integer value) const;
    /// Validate a boolean value.
    /// @param context The validation context to use.
    /// @param value The boolean value to validate.
    virtual void validateBoolean(const ValidationContext &context, bool value) const;
    /// Validate a float value.
    /// @param context The validation context to use.
    /// @param value The float value to validate.
    virtual void validateFloat(const ValidationContext &context, Float value) const;
    /// Validate a text value.
    /// @param context The validation context to use.
    /// @param value The text value to validate.
    virtual void validateText(const ValidationContext &context, const text::String &value) const;
    /// Validate a date value.
    /// @param context The validation context to use.
    /// @param value The date value to validate.
    virtual void validateDate(const ValidationContext &context, const time::Date &value) const;
    /// Validate a time value.
    /// @param context The validation context to use.
    /// @param value The time value to validate.
    virtual void validateTime(const ValidationContext &context, const time::Time &value) const;
    /// Validate a date-time value.
    /// @param context The validation context to use.
    /// @param value The date-time value to validate.
    virtual void validateDateTime(const ValidationContext &context, const time::DateTime &value) const;
    /// Validate a bytes value.
    /// @param context The validation context to use.
    /// @param value The bytes value to validate.
    virtual void validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const;
    /// Validate a time delta value.
    /// @param context The validation context to use.
    /// @param value The time delta value to validate.
    virtual void validateTimeDelta(const ValidationContext &context, const time::CalendarDelta &value) const;
    /// Validate a regular expression value.
    /// @param context The validation context to use.
    /// @param value The regular expression value to validate.
    virtual void validateRegEx(const ValidationContext &context, const re::RegExPtr &value) const;
    /// Validate a list of values.
    /// @param context The validation context to use.
    virtual void validateValueList(const ValidationContext &context) const;
    /// Validate a list of sections.
    /// @param context The validation context to use.
    virtual void validateSectionList(const ValidationContext &context) const;
    /// Validate an intermediate section.
    /// @param context The validation context to use.
    virtual void validateIntermediateSection(const ValidationContext &context) const;
    /// Validate a section with names.
    /// @param context The validation context to use.
    virtual void validateSectionWithNames(const ValidationContext &context) const;
    /// Validate a section with texts.
    /// @param context The validation context to use.
    virtual void validateSectionWithTexts(const ValidationContext &context) const;

public: // testing
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const Constraint &constraint) -> InternalViewPtr;
    friend auto internalView(const ConstraintPtr &constraintPtr) -> InternalViewPtr;
    /// Create a diagnostic view of this constraint's internal state.
    [[nodiscard]] virtual auto internalView() const -> InternalViewPtr;
#endif

private:
    text::String _name;
    Location _location;
    const vr::ConstraintType _type;
    text::String _errorMessage;
    bool _isNegated{false};
    bool _isFromTemplate{false};
};

}
