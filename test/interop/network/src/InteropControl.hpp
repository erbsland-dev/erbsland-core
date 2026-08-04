// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <optional>
#include <string>

class InteropControl final {
public:
    struct Response final {
        std::uint64_t id{};
        std::string status;
        std::optional<std::uint64_t> scenarioId;
        std::optional<std::uint64_t> port;
        std::optional<std::uint64_t> bytesReceived;
        std::string errorCode;
        std::string errorMessage;
    };

public:
    InteropControl();
    ~InteropControl();
    InteropControl(const InteropControl &) = delete;
    InteropControl(InteropControl &&) = delete;
    auto operator=(const InteropControl &) -> InteropControl & = delete;
    auto operator=(InteropControl &&) -> InteropControl & = delete;

public:
    [[nodiscard]] auto port() const noexcept -> std::uint16_t;
    void acceptCounterpart(const std::string &expectedToken);
    [[nodiscard]] auto request(const std::string &message, std::uint64_t expectedId) -> Response;

private:
    using NativeSocket = std::uintptr_t;
    static constexpr auto cInvalidSocket = static_cast<NativeSocket>(-1);
    static constexpr auto cMaximumLineLength = std::size_t{64U * 1024U};

private:
    static void closeSocket(NativeSocket socket) noexcept;
    static void setTimeout(NativeSocket socket);
    static void sendAll(NativeSocket socket, const std::string &text);
    [[nodiscard]] static auto receiveLine(NativeSocket socket) -> std::string;
    [[nodiscard]] static auto jsonString(const std::string &text, const std::string &key) -> std::optional<std::string>;
    [[nodiscard]] static auto jsonUnsigned(const std::string &text, const std::string &key)
        -> std::optional<std::uint64_t>;
    [[nodiscard]] static auto decodeResponse(const std::string &text) -> Response;
    [[nodiscard]] static auto nativeError() -> int;

private:
    NativeSocket _listener{cInvalidSocket};
    NativeSocket _connection{cInvalidSocket};
    std::uint16_t _port{};
#if defined(_WIN32)
    bool _winsockInitialized{};
#endif
};
