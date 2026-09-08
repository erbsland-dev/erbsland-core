// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../err/LogicError.hpp"
#include "../../../text/Literals.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace erbsland::core::impl {

/// Thread-safe lazy storage for one application-data component.
/// The factory is serialized with access to the stored pointer and must not recursively access this accessor.
/// A failed factory invocation leaves the accessor empty so a later call can retry creation.
/// @tested{ApplicationDataAccessorTest}
template <typename T>
class ApplicationDataAccessor {
public:
    /// Factory for the lazily created component.
    using Factory = std::function<std::shared_ptr<T>()>;

public:
    /// Create an accessor that default-constructs its component.
    ApplicationDataAccessor() : ApplicationDataAccessor{[]() -> std::shared_ptr<T> { return std::make_shared<T>(); }} {}
    /// Create an accessor using a custom component factory.
    /// @param factory Factory invoked at most once successfully and serialized with all accessor operations.
    explicit ApplicationDataAccessor(Factory factory) : _factory{std::move(factory)} {}

    // defaults/deletions
    ~ApplicationDataAccessor() = default;
    ApplicationDataAccessor(const ApplicationDataAccessor &) = delete;
    ApplicationDataAccessor(ApplicationDataAccessor &&) = delete;
    auto operator=(const ApplicationDataAccessor &) -> ApplicationDataAccessor & = delete;
    auto operator=(ApplicationDataAccessor &&) -> ApplicationDataAccessor & = delete;

public:
    /// Access the component, creating it on first use.
    /// @return Shared ownership of the component.
    /// @throws err::LogicError If the configured factory is empty or returns a null pointer.
    [[nodiscard]] auto get() const -> std::shared_ptr<T> {
        using namespace text::literals;

        const auto lock = std::scoped_lock{_mutex};
        if (_data == nullptr) {
            if (!_factory) {
                throw err::LogicError{"The application-data component factory is empty."_el};
            }
            auto data = _factory();
            if (data == nullptr) {
                throw err::LogicError{"The application-data component factory returned a null pointer."_el};
            }
            _data = std::move(data);
        }
        return _data;
    }
    /// Access the component without creating it.
    /// @return Shared ownership of the component, or a null pointer if it was not created.
    [[nodiscard]] auto getIfExists() const -> std::shared_ptr<T> {
        const auto lock = std::scoped_lock{_mutex};
        return _data;
    }

private:
    mutable std::mutex _mutex;        ///< Serializes component creation and pointer access.
    Factory _factory;                 ///< Factory for the component.
    mutable std::shared_ptr<T> _data; ///< The component after successful creation.
};

}
