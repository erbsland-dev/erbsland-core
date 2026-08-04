// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfError.hpp"
#include "Float.hpp"
#include "Integer.hpp"
#include "Location.hpp"
#include "NamePath.hpp"
#include "TestFormat.hpp"
#include "Value_fwd.hpp"
#include "ValueIterator.hpp"
#include "ValueList.hpp"
#include "ValueMatrix.hpp"
#include "ValueType.hpp"

#include "impl/utilities/TypeTraits.hpp"
#include "vr/Rule_fwd.hpp"

#include "../math/SaturatingMath.hpp"
#include "../mem/ByteBlock.hpp"
#include "../re/RegEx_fwd.hpp"
#include "../text/String.hpp"
#include "../text/StringFormat.hpp"
#include "../time/CalendarDelta.hpp"
#include "../time/Date.hpp"
#include "../time/DateTime.hpp"
#include "../time/Time.hpp"
#include "../time/TimeWithZone.hpp"

namespace erbsland::conf {

/// The base class and interface for all values.
/// @tested{ValueAsMethodsTest ValueGetMethodsTest}
class Value : public std::enable_shared_from_this<Value> {
public:
    // defaults
    virtual ~Value() = default;

public: // basic properties.
    /// The name.
    [[nodiscard]] virtual auto name() const noexcept -> Name = 0;
    /// The name path.
    [[nodiscard]] virtual auto namePath() const noexcept -> NamePath = 0;
    /// Test if this value has a parent.
    [[nodiscard]] virtual auto hasParent() const noexcept -> bool = 0;
    /// The parent.
    [[nodiscard]] virtual auto parent() const noexcept -> ValuePtr = 0;
    /// The type of this value.
    [[nodiscard]] virtual auto type() const noexcept -> ValueType = 0;

public: // location
    /// Test if this value has location info.
    [[nodiscard]] virtual auto hasLocation() const noexcept -> bool = 0;
    /// Get the location info for this value.
    [[nodiscard]] virtual auto location() const noexcept -> Location = 0;
    /// Set the location info for this value.
    virtual void setLocation(const Location &newLocation) noexcept = 0;

public: // validation
    /// Test if this value was validated.
    /// @return `true` if this value was validated using validation-rules.
    [[nodiscard]] virtual auto wasValidated() const noexcept -> bool = 0;
    /// The rule that was used to validate this value.
    /// @return The rule or nullptr if this value was not validated.
    [[nodiscard]] virtual auto validationRule() const noexcept -> vr::RulePtr = 0;
    /// Test if this value is a secret.
    /// This is a convenience method, checking the assigned validation rule.
    /// @return `true` if the validation rule marks this value as secret. `false` otherwise or if this
    /// value wasn't validated.
    [[nodiscard]] virtual auto isSecret() const noexcept -> bool;
    /// Test if this value is a default value from a validation-rules document.
    [[nodiscard]] virtual auto isDefaultValue() const noexcept -> bool = 0;

public: // lists
    /// Get the number of children.
    [[nodiscard]] virtual auto size() const noexcept -> std::size_t = 0;
    /// Test if there is a child-value with the given index, name, or name-path.
    /// @param namePath A name-path, name, or index.
    /// @return `true` if there is a value (and the name-path is valid).
    [[nodiscard]] virtual auto hasValue(const NamePathLike &namePath) const noexcept -> bool = 0;
    /// Get the child-value at the specified index, name, or name-path.
    /// If no value is found at the given location, or the name-path contains syntax errors, the method
    /// returns a `nullptr`.
    /// @param namePath A name-path, name, or index.
    /// @return The child value.
    [[nodiscard]] virtual auto value(const NamePathLike &namePath) const noexcept -> ValuePtr = 0;
    /// Get the child-value at the specified index, name, or name-path.
    /// If no value is found at the given location, or the name-path contains syntax errors,
    /// a `ConfError` exception is thrown.
    /// @param namePath A name-path, name, or index.
    /// @return The child value.
    /// @throws ConfError (NotFound, Syntax) if the value does not exist or the name-path contains syntax errors.
    [[nodiscard]] virtual auto valueOrThrow(const NamePathLike &namePath) const -> ValuePtr = 0;
    /// Get an iterator to the first child value.
    /// @return The value iterator.
    [[nodiscard]] virtual auto begin() const noexcept -> ValueIterator = 0;
    /// Get an iterator to the end of the child values.
    /// @return The value iterator.
    [[nodiscard]] virtual auto end() const noexcept -> ValueIterator = 0;

public: // conversions
    /// @name Access as Typed Value
    /// These methods return the contained value if it has the requested type. Otherwise, a default-constructed
    /// value of the corresponding type is returned, or in case of the <code>...OrThrow</code> variants,
    /// a `ConfError` (TypeMismatch) is thrown.
    /// No type conversion or coercion is performed.
    /// For example,
    /// - `asInteger()` returns the stored `Integer` if this value is of type `Integer`, or `Integer{}` otherwise.
    /// - `asText()` returns the stored `text::String` only if this value is of type `Text`, not if it's e.g. an
    /// `Integer`. To obtain a textual representation of any supported type (e.g. `Integer → "42"`), use
    /// `toTextRepresentation()`.
    /// @{

    /// Access as integer.
    /// @return The value of the requested type, or if this value has a different type, a default value.
    [[nodiscard]] virtual auto asInteger() const noexcept -> Integer = 0;
    /// Access as integer.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asIntegerOrThrow() const -> Integer = 0;
    /// Access as boolean.
    /// @return The value of the requested type, or if this value has a different type, a default value.
    [[nodiscard]] virtual auto asBoolean() const noexcept -> bool = 0;
    /// Access as boolean.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asBooleanOrThrow() const -> bool = 0;
    /// Access as a floating-point value.
    /// @return The value of the requested type, or if this value has a different type, a default value.
    [[nodiscard]] virtual auto asFloat() const noexcept -> Float = 0;
    /// Access as a floating-point value.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asFloatOrThrow() const -> Float = 0;
    /// Access as text.
    /// @return The value of the requested type, or if this value has a different type, a default value.
    [[nodiscard]] virtual auto asText() const noexcept -> text::String = 0;
    /// Access as text.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asTextOrThrow() const -> text::String = 0;
    /// Access as a `time::Date` instance.
    /// @return The value of the requested type, or if this value has a different type, a default value.
    [[nodiscard]] virtual auto asDate() const noexcept -> time::Date = 0;
    /// Access as a `time::Date` instance.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asDateOrThrow() const -> time::Date = 0;
    /// Access as a floating `time::Time` instance, removing any configured zone.
    /// @return The value of the requested type, or a default value if this value has a different type.
    [[nodiscard]] virtual auto asTime() const noexcept -> time::Time = 0;
    /// Access as a floating `time::Time` instance, removing any configured zone.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asTimeOrThrow() const -> time::Time = 0;
    /// Access as a `time::TimeWithZone` instance.
    /// @return The value of the requested type, or if this value has a different type, a default value.
    [[nodiscard]] virtual auto asTimeWithZone() const noexcept -> time::TimeWithZone = 0;
    /// Access as a `time::TimeWithZone` instance.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asTimeWithZoneOrThrow() const -> time::TimeWithZone = 0;
    /// Access as a `time::DateTime` instance.
    /// @return The value of the requested type, or if this value has a different type, a default value.
    [[nodiscard]] virtual auto asDateTime() const noexcept -> time::DateTime = 0;
    /// Access as a `time::DateTime` instance.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asDateTimeOrThrow() const -> time::DateTime = 0;
    /// Access as a `mem::ByteBlock` array.
    /// @return The value of the requested type, or if this value has a different type, a default value.
    [[nodiscard]] virtual auto asBytes() const noexcept -> mem::ByteBlock = 0;
    /// Access as a `mem::ByteBlock` array.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asBytesOrThrow() const -> mem::ByteBlock = 0;
    /// Access as a `time::CalendarDelta` instance.
    /// @return The value of the requested type, or if this value has a different type, a default value.
    [[nodiscard]] virtual auto asCalendarDelta() const noexcept -> time::CalendarDelta = 0;
    /// Access as a `time::CalendarDelta` instance.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asCalendarDeltaOrThrow() const -> time::CalendarDelta = 0;
    /// Access as a compiled regular expression.
    /// @return The value of the requested type, or `nullptr` if this value has a different type.
    [[nodiscard]] virtual auto asRegEx() const noexcept -> re::RegExPtr = 0;
    /// Access as a compiled regular expression.
    /// @return The value of the requested type.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asRegExOrThrow() const -> re::RegExPtr = 0;
    /// Access as a value list.
    /// @important This call does *not* convert a single value into a list with one element.
    ///     Use the `getList()` methods if you like to have this behavior.
    /// @return A value list, or an empty list if this is no value list.
    [[nodiscard]] virtual auto asValueList() const noexcept -> ValueList = 0;
    /// Access as a value list.
    /// @important This call does *not* convert a single value into a list with one element.
    ///     Use the `getList()` methods if you like to have this behavior.
    /// @return A value list.
    /// @throws ConfError (TypeMismatch) if the value is of another type.
    [[nodiscard]] virtual auto asValueListOrThrow() const -> ValueList = 0;

    /// This is a convenience method, accessed this value as one of the supported types. It is implemented
    /// calling the various `to<Type>()` methods.
    /// @important
    /// - There are overloads for all integers, converting the signed 64-bit integer into the desired type.
    /// - There are overloads for all float types, converting a `double` into a `float` if necessary.
    /// - If the value exceeds the range of the target type:
    ///   - for `asType()` saturation logic is used - returning the max/min possible value for the chosen type.
    ///   - for `asTypeOrThrow()` a `TypeMismatch` exception is thrown.
    /// - The overload for `ValueList` works exactly like `asValueList()` and therefore *does not* convert
    ///   a single value into a list with one element. Use the `getList()` methods for this behavior.
    /// @tparam T The type to access this value as.
    /// @return The value or a default value.
    template <typename T>
    [[nodiscard]] auto asType() const noexcept -> T;
    /// @overload
    template <typename T>
    [[nodiscard]] auto asTypeOrThrow() const -> T;
    /// @private
    template <typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] auto asType() const noexcept -> T;
    /// @private
    template <typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] auto asTypeOrThrow() const -> T;
    /// @private
    template <typename T>
        requires std::is_floating_point_v<T>
    [[nodiscard]] auto asType() const noexcept -> T;
    /// @private
    template <typename T>
        requires std::is_floating_point_v<T>
    [[nodiscard]] auto asTypeOrThrow() const -> T;
    /// @}

    /// @name Get as Uniform Value Lists and Matrices
    /// - Tries to get this value as uniform lists that consist of values of the same type.
    /// - If this is a single value that matches the type, a list with one element is returned.
    /// @{

    /// @return A list with values of this type, or an empty list on any problem.
    template <typename T>
    [[nodiscard]] auto asList() const noexcept -> std::vector<T>;
    /// @return A list with values of this type.
    /// @throws ConfError In case of any type mismatch or syntax error in the name path.
    template <typename T>
    [[nodiscard]] auto asListOrThrow() const -> std::vector<T>;
    /// @return A matrix with values of this type, or an empty matrix on any problem.
    template <typename T>
    [[nodiscard]] auto asMatrix() const noexcept -> Matrix<T>;
    /// @return A matrix with values of this type.
    /// @throws ConfError In case of any type mismatch or syntax error in the name path.
    template <typename T>
    [[nodiscard]] auto asMatrixOrThrow() const -> Matrix<T>;
    /// @}

    /// Convert this value to a value list.
    /// In contrast with `asValueList`, this method will not only return a value list if this *is* a value list,
    /// but also if this is a scalar value (Text, Integer, Float, Boolean, time::Date, time::Time,
    /// time::TimeWithZone, time::DateTime, mem::ByteBlock, time::CalendarDelta, re::RegExPtr). If this is a scalar
    /// value, a value list with a single element is returned (this element).
    [[nodiscard]] auto toValueList() noexcept -> ValueList;
    /// @overload
    [[nodiscard]] auto toValueList() const noexcept -> ConstValueList;
    /// Convert this value to a value matrix.
    /// This will return a matrix when this is a ValueList, a nested ValueList or a scalar value.
    /// For a nested value list, a matrix with the largest row and column count is returned.
    /// For a regular value list, a matrix with one column and the number of rows is returned.
    /// For a scalar value, a matrix with one row and one column is returned.
    [[nodiscard]] auto toValueMatrix() noexcept -> ValueMatrix;
    /// @overload
    [[nodiscard]] auto toValueMatrix() const noexcept -> ConstValueMatrix;
    /// Convert this value to its text representation.
    /// Converts the types: Text, Integer, Float, Boolean, time::Date, time::Time, time::TimeWithZone,
    /// time::DateTime, mem::ByteBlock, time::CalendarDelta, re::RegExPtr. Sections and lists result in an empty
    /// string.
    /// @return A string with the text or text representation.
    [[nodiscard]] virtual auto toTextRepresentation() const noexcept -> text::String = 0;
    /// Convert this value to its test adapter representation.
    /// This is used by the test adapter to verify the value, as described in the language documentation.
    /// The general format is <code>&lt;Type&gt;(&lt;value&gt;)</code>, where <code>&lt;Type&gt;</code> is one of the
    /// standardized type names and <code>&lt;value&gt;</code> the value representation as specified.
    /// For example, integer value 5 is converted into the text <code>Integer(5)</code>.
    /// No additional info is added to sections.
    /// @param format The format of the output.
    /// @return The value in its test outcome representation.
    [[nodiscard]] auto toTestText(TestFormat format = {}) const noexcept -> text::String;
    /// Convert this value into a visual value tree.
    /// This method is useful for testing to get a visual representation of a parsed document, or
    /// a branch of the document.
    /// @param format The format of the output.
    /// @return A text with a visual tree representation of this value.
    [[nodiscard]] auto toTestValueTree(TestFormat format = {}) const noexcept -> text::String;

public: // Convenience methods.
    /// Test if this container is empty.
    /// @return `true` if the container is empty.
    [[nodiscard]] auto empty() const noexcept -> bool;
    /// Get the first value of a container.
    /// @return The first value or a `nullptr`.
    [[nodiscard]] auto firstValue() const noexcept -> ValuePtr;
    /// Get the last value of a container.
    /// @return The last value or a `nullptr`.
    [[nodiscard]] auto lastValue() const noexcept -> ValuePtr;
    /// @name Get a Value of a Given Type
    /// Tries to get a value at the given name-path with a given type.
    /// - If the name path is not valid,
    /// - or if there is no value at the name-path
    /// - or the value does not have the expected type,
    /// - a default value passed as `defaultValue` parameter is returned.
    /// - ... or, an exception is thrown for the <code>...OrThrow</code> methods.
    /// If types are converted, the same logic as described in `asType()` or `asTypeOrThrow()` applies.
    /// @{

    /// @param namePath The name path, name or index to resolve, relative to this value.
    /// @param defaultValue The default value returned if the value can't be resolved.
    /// @tparam tExpectedType The type to expect, e.g. `text::String`, `Integer`.
    /// @return the requested value or `defaultValue`.
    template <typename tExpectedType>
        requires(
            std::is_same_v<tExpectedType, bool> ||
            (!std::is_integral_v<tExpectedType> && !std::is_floating_point_v<tExpectedType>))
    [[nodiscard]] auto get(
        const NamePathLike &namePath, impl::value_get_default_param_t<tExpectedType> defaultValue = {}) const noexcept
        -> tExpectedType;
    /// @private
    template <typename tExpectedType>
        requires(std::is_integral_v<tExpectedType> && !std::is_same_v<tExpectedType, bool>)
    [[nodiscard]] auto get(const NamePathLike &namePath, tExpectedType defaultValue = {}) const noexcept
        -> tExpectedType;
    /// @private
    template <typename tExpectedType>
        requires(
            std::is_floating_point_v<tExpectedType> && !std::is_integral_v<tExpectedType> &&
            !std::is_same_v<tExpectedType, bool>)
    [[nodiscard]] auto get(const NamePathLike &namePath, tExpectedType defaultValue = {}) const noexcept
        -> tExpectedType;
    /// @param namePath The name path, name or index to resolve, relative to this value.
    /// @throws ConfError If the value does not exist or has the wrong type.
    /// @tparam tExpectedType The type to expect, e.g. `text::String`, `Integer`.
    /// @return The requested value.
    template <typename tExpectedType>
    [[nodiscard]] auto getOrThrow([[maybe_unused]] const NamePathLike &namePath) const -> tExpectedType {
        using namespace text::literals;
        throw err::LogicError("getOrThrow() not implemented for the given type."_el);
    }
    /// @private
    template <typename tExpectedType>
        requires std::is_integral_v<tExpectedType>
    [[nodiscard]] auto getOrThrow(const NamePathLike &namePath) const -> tExpectedType {
        return valueOrThrow(namePath)->template asTypeOrThrow<tExpectedType>();
    }
    /// @private
    template <typename tExpectedType>
        requires std::is_floating_point_v<tExpectedType>
    [[nodiscard]] auto getOrThrow(const NamePathLike &namePath) const -> tExpectedType {
        return static_cast<tExpectedType>(getFloatOrThrow(namePath));
    }
    /// @param namePath The name path, name or index to resolve, relative to this value.
    /// @param defaultValue The default value returned if the value can't be resolved.
    /// @return The value or `defaultValue` if there is no matching value at `namePath`.
    [[nodiscard]] auto getInteger(const NamePathLike &namePath, Integer defaultValue = {}) const noexcept -> Integer;
    /// @param namePath The name path, name or index to resolve, relative to this value.
    /// @return The requested value.
    /// @throws ConfError If the value does not exist or has the wrong type.
    [[nodiscard]] auto getIntegerOrThrow(const NamePathLike &namePath) const -> Integer;
    /// @copydoc getInteger
    [[nodiscard]] auto getBoolean(const NamePathLike &namePath, bool defaultValue = false) const noexcept -> bool;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getBooleanOrThrow(const NamePathLike &namePath) const -> bool;
    /// @copydoc getInteger
    [[nodiscard]] auto getFloat(const NamePathLike &namePath, Float defaultValue = {}) const noexcept -> Float;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getFloatOrThrow(const NamePathLike &namePath) const -> Float;
    /// @copydoc getInteger
    [[nodiscard]] auto getText(const NamePathLike &namePath, text::String defaultValue = {}) const noexcept
        -> text::String;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getTextOrThrow(const NamePathLike &namePath) const -> text::String;
    /// @copydoc getInteger
    [[nodiscard]] auto getDate(const NamePathLike &namePath, const time::Date &defaultValue = {}) const noexcept
        -> time::Date;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getDateOrThrow(const NamePathLike &namePath) const -> time::Date;
    /// @copydoc getInteger
    [[nodiscard]] auto getTime(const NamePathLike &namePath, const time::Time &defaultValue = {}) const noexcept
        -> time::Time;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getTimeOrThrow(const NamePathLike &namePath) const -> time::Time;
    /// @copydoc getInteger
    [[nodiscard]] auto getTimeWithZone(
        const NamePathLike &namePath, const time::TimeWithZone &defaultValue = {}) const noexcept -> time::TimeWithZone;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getTimeWithZoneOrThrow(const NamePathLike &namePath) const -> time::TimeWithZone;
    /// @copydoc getInteger
    [[nodiscard]] auto getDateTime(const NamePathLike &namePath, const time::DateTime &defaultValue = {}) const noexcept
        -> time::DateTime;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getDateTimeOrThrow(const NamePathLike &namePath) const -> time::DateTime;
    /// @copydoc getInteger
    [[nodiscard]] auto getBytes(const NamePathLike &namePath, const mem::ByteBlock &defaultValue = {}) const noexcept
        -> mem::ByteBlock;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getBytesOrThrow(const NamePathLike &namePath) const -> mem::ByteBlock;
    /// @copydoc getInteger
    [[nodiscard]] auto getCalendarDelta(
        const NamePathLike &namePath, const time::CalendarDelta &defaultValue = {}) const noexcept
        -> time::CalendarDelta;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getCalendarDeltaOrThrow(const NamePathLike &namePath) const -> time::CalendarDelta;
    /// @copydoc getInteger
    [[nodiscard]] auto getRegEx(const NamePathLike &namePath, const re::RegExPtr &defaultValue = {}) const noexcept
        -> re::RegExPtr;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getRegExOrThrow(const NamePathLike &namePath) const -> re::RegExPtr;
    /// @param namePath The name path, name, or index to resolve, relative to this value.
    /// @return The value list or an empty list if there is no matching value at `namePath`.
    [[nodiscard]] auto getValueList(const NamePathLike &namePath) const noexcept -> ValueList;
    /// @copydoc getIntegerOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getValueListOrThrow(const NamePathLike &namePath) const -> ValueList;
    /// @}

    /// @name Get Uniform Value Lists or Matrices
    /// - Tries to get uniform lists/matrices that consist of values of the same type.
    /// - If there is a single value at the name path, a list with one element is returned.
    /// @{

    /// @param namePath The name-path, name, or index of the value list.
    /// @return A list with values of this type, or an empty list on any problem.
    template <typename T>
    [[nodiscard]] auto getList(const NamePathLike &namePath) const noexcept -> std::vector<T>;
    /// @param namePath The name-path, name, or index of the value list.
    /// @return A list with values of this type.
    /// @throws ConfError In case of any type mismatch or syntax error in the name path.
    template <typename T>
    [[nodiscard]] auto getListOrThrow(const NamePathLike &namePath) const -> std::vector<T>;
    /// @param namePath The name-path, name, or index of the value list.
    /// @return A matrix with values of this type, or an empty matrix on any problem.
    template <typename T>
    [[nodiscard]] auto getMatrix(const NamePathLike &namePath) const noexcept -> Matrix<T>;
    /// @param namePath The name-path, name, or index of the value list.
    /// @return A matrix with values of this type.
    /// @throws ConfError In case of any type mismatch or syntax error in the name path.
    template <typename T>
    [[nodiscard]] auto getMatrixOrThrow(const NamePathLike &namePath) const -> Matrix<T>;
    /// @}

    /// @name Get a Section-Map or Section-List
    /// Tries to get a section map or section list at the given path. If the path does not exist (or contains
    /// syntax errors), either a nullptr is returned, or an exception is thrown (`...OrThrow()` methods).
    /// @{

    /// @param namePath The name path, name or index to resolve, relative to this value.
    /// @return The section map/list or nullptr.
    [[nodiscard]] auto getSectionWithNames(const NamePathLike &namePath) const noexcept -> ValuePtr;
    /// @param namePath The name path, name or index to resolve, relative to this value.
    /// @return The section map or section list.
    /// @throws ConfError if the name path has syntax errors, or if there is no matching section at the value-path.
    [[nodiscard]] auto getSectionWithNamesOrThrow(const NamePathLike &namePath) const -> ValuePtr;
    /// @copydoc getSectionWithNames(const NamePathLike&) const
    [[nodiscard]] auto getSectionWithTexts(const NamePathLike &namePath) const noexcept -> ValuePtr;
    /// @copydoc getSectionWithNamesOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getSectionWithTextsOrThrow(const NamePathLike &namePath) const -> ValuePtr;
    /// @copydoc getSectionWithNames(const NamePathLike&) const
    [[nodiscard]] auto getSectionList(const NamePathLike &namePath) const noexcept -> ValuePtr;
    /// @copydoc getSectionWithNamesOrThrow(const NamePathLike&) const
    [[nodiscard]] auto getSectionListOrThrow(const NamePathLike &namePath) const -> ValuePtr;
    /// @}

    /// @name Tests for a Value Type
    /// Test if a value is of a certain type.
    /// @{

    /// @return `true` if the value has the tested type.
    [[nodiscard]] auto isInteger() const noexcept -> bool { return type() == ValueType::Integer; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isBoolean() const noexcept -> bool { return type() == ValueType::Boolean; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isFloat() const noexcept -> bool { return type() == ValueType::Float; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isText() const noexcept -> bool { return type() == ValueType::Text; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isDate() const noexcept -> bool { return type() == ValueType::Date; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isTime() const noexcept -> bool { return type() == ValueType::Time; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isDateTime() const noexcept -> bool { return type() == ValueType::DateTime; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isBytes() const noexcept -> bool { return type() == ValueType::Bytes; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isTimeDelta() const noexcept -> bool { return type() == ValueType::TimeDelta; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isRegEx() const noexcept -> bool { return type() == ValueType::RegEx; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isValueList() const noexcept -> bool { return type() == ValueType::ValueList; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isDocument() const noexcept -> bool { return type() == ValueType::Document; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isRoot() const noexcept -> bool { return type() == ValueType::Document; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isSectionWithNames() const noexcept -> bool { return type() == ValueType::SectionWithNames; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isSectionWithTexts() const noexcept -> bool { return type() == ValueType::SectionWithTexts; }
    /// @copydoc isInteger() const
    [[nodiscard]] auto isSectionList() const noexcept -> bool { return type() == ValueType::SectionList; }

    /// Test if this value is a list.
    /// Tests if this value is a list, like a section list or value list with child-elements that can be
    /// iterated in a sequence.
    /// @return `true` if this value is a list.
    [[nodiscard]] auto isList() const noexcept -> bool { return type().isList(); }

    /// Test if this value is a name-value map.
    /// Tests if this value is a name-value map, like a section with names, section with texts,
    /// intermediate section, or a document.
    [[nodiscard]] auto isMap() const noexcept -> bool { return type().isMap(); }
    /// @}

private:
    /// Convert a scalar or list value to its uniform matrix representation.
    template <typename tValueMatrix, typename tValue>
    [[nodiscard]] static auto toValueMatrixImpl(tValue &value) noexcept -> tValueMatrix;
};

}

#include "Value_asType.tpp"
#include "Value_get.tpp"
#include "Value_list.tpp"
#include "Value_matrix.tpp"
