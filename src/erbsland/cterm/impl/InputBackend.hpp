// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Backend.hpp"
#include "../Input.hpp"

namespace erbsland::cterm::impl {

/// The input implementation, using the platform backend.
class InputBackend : public Input {
public:
    // defaults
    InputBackend() = default;
    ~InputBackend() override = default;

public:
    [[nodiscard]] auto mode() const noexcept -> Mode override;
    void setMode(Mode mode) override;
    [[nodiscard]] auto readKeyImpl(std::chrono::milliseconds timeout) const -> Key override;
    [[nodiscard]] auto waitForKeyImpl() const -> Key override;
    [[nodiscard]] auto readLine() -> text::String override;
    void purgePendingInput() noexcept override;

public:
    /// Change the current backend.
    void setBackend(BackendPtr backend) noexcept;

private:
    BackendPtr _backend;
};

}
