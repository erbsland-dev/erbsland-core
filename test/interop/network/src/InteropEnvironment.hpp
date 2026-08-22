// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InteropControl.hpp"

#include <erbsland/system/Subprocess.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

class InteropEnvironment final {
public:
    class Scenario final {
    public:
        Scenario(InteropEnvironment &environment, std::uint64_t id, std::uint16_t port) noexcept;
        ~Scenario();
        Scenario(Scenario &&other) noexcept;
        auto operator=(Scenario &&other) noexcept -> Scenario &;
        Scenario(const Scenario &) = delete;
        auto operator=(const Scenario &) -> Scenario & = delete;

    public:
        [[nodiscard]] auto port() const noexcept -> std::uint16_t;
        [[nodiscard]] auto finish() -> std::uint64_t;
        void cancel() noexcept;

    private:
        InteropEnvironment *_environment{};
        std::uint64_t _id{};
        std::uint16_t _port{};
    };

public:
    explicit InteropEnvironment(const std::filesystem::path &testExecutable);
    ~InteropEnvironment();
    InteropEnvironment(const InteropEnvironment &) = delete;
    InteropEnvironment(InteropEnvironment &&) = delete;
    auto operator=(const InteropEnvironment &) -> InteropEnvironment & = delete;
    auto operator=(InteropEnvironment &&) -> InteropEnvironment & = delete;

public:
    static void install(InteropEnvironment &environment) noexcept;
    [[nodiscard]] static auto instance() -> InteropEnvironment &;
    [[nodiscard]] auto startScenario(
        const std::string &scenario, const std::string &cipher, std::uint64_t payloadLength) -> Scenario;
    [[nodiscard]] auto startClientScenario(
        std::uint16_t port,
        const std::string &scenario,
        const std::string &cipher,
        std::uint64_t payloadLength,
        const std::string &serverName = "localhost") -> Scenario;
    /// Compare bounded HTTP wire syntax with the independent Rust httparse implementation.
    [[nodiscard]] auto compareHttp(const std::string &kind, const std::string &wireHex) -> InteropControl::Response;

private:
    [[nodiscard]] auto nextId() noexcept -> std::uint64_t;
    [[nodiscard]] auto finishScenario(std::uint64_t scenarioId) -> std::uint64_t;
    void cancelScenario(std::uint64_t scenarioId) noexcept;
    [[nodiscard]] static auto jsonEscape(const std::string &value) -> std::string;
    [[nodiscard]] static auto token() -> std::string;

private:
    inline static InteropEnvironment *_instance{};
    InteropControl _control;
    std::optional<erbsland::system::Subprocess> _counterpart;
    std::uint64_t _nextId{1U};
};
