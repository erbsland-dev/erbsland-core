// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Result.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

namespace erbsland::util {

/// A result that transports additional typed data.
/// The result derives from its status type and keeps its success/failure semantics.
/// @tparam tData The transported data type.
/// @tparam tStatus The result status type.
/// @tested{ResultWithDataTest}
template <typename tData, typename tStatus = Result>
    requires std::derived_from<tStatus, Result>
class ResultWithData : public tStatus {
public:
    /// The transported data type.
    using Data = tData;
    /// The specialized result status type.
    using Status = tStatus;

public:
    /// Create a result with copied data.
    /// @param result The result status.
    /// @param data The data to transport.
    ResultWithData(const Status result, const Data &data) : Status{result}, _data{data} {}
    /// Create a result with moved data.
    /// @param result The result status.
    /// @param data The data to transport.
    ResultWithData(const Status result, Data &&data) noexcept(std::is_nothrow_move_constructible_v<Data>) :
        Status{result}, _data{std::move(data)} {}

    // defaults
    ~ResultWithData() = default;
    ResultWithData(const ResultWithData &) = default;
    ResultWithData(ResultWithData &&) noexcept(std::is_nothrow_move_constructible_v<Data>) = default;
    auto operator=(const ResultWithData &) -> ResultWithData & = default;
    auto operator=(ResultWithData &&) noexcept(std::is_nothrow_move_assignable_v<Data>) -> ResultWithData & = default;

public: // accessors
    /// Get the specialized result status.
    [[nodiscard]] auto status() const noexcept -> Status { return static_cast<const Status &>(*this); }
    /// Access the transported data.
    [[nodiscard]] auto data() const noexcept -> const Data & { return _data; }
    /// Access the transported data.
    [[nodiscard]] auto data() noexcept -> Data & { return _data; }
    /// Take the transported data from this result.
    [[nodiscard]] auto takeData() noexcept(std::is_nothrow_move_constructible_v<Data>) -> Data {
        return std::move(_data);
    }

private:
    Data _data;
};

}
