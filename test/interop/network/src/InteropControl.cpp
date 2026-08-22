// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "InteropControl.hpp"

#include <array>
#include <cctype>
#include <chrono>
#include <stdexcept>
#include <string_view>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#endif

InteropControl::InteropControl() {
#if defined(_WIN32)
    auto data = WSADATA{};
    if (::WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        throw std::runtime_error{"failed to initialize Winsock for interop control"};
    }
    _winsockInitialized = true;
#endif
    _listener = static_cast<NativeSocket>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (_listener == cInvalidSocket) {
        throw std::runtime_error{"failed to create interop control listener: " + std::to_string(nativeError())};
    }
    auto address = sockaddr_in{};
    address.sin_family = AF_INET;
    address.sin_port = 0;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(
            static_cast<decltype(::socket(0, 0, 0))>(_listener),
            reinterpret_cast<const sockaddr *>(&address),
            static_cast<int>(sizeof(address))) != 0) {
        throw std::runtime_error{"failed to bind interop control listener: " + std::to_string(nativeError())};
    }
    auto boundAddress = sockaddr_in{};
#if defined(_WIN32)
    auto boundLength = static_cast<int>(sizeof(boundAddress));
#else
    auto boundLength = static_cast<socklen_t>(sizeof(boundAddress));
#endif
    if (::getsockname(
            static_cast<decltype(::socket(0, 0, 0))>(_listener),
            reinterpret_cast<sockaddr *>(&boundAddress),
            &boundLength) != 0) {
        throw std::runtime_error{"failed to query interop control listener: " + std::to_string(nativeError())};
    }
    _port = ntohs(boundAddress.sin_port);
    if (::listen(static_cast<decltype(::socket(0, 0, 0))>(_listener), 1) != 0) {
        throw std::runtime_error{"failed to listen for interop control: " + std::to_string(nativeError())};
    }
    setTimeout(_listener);
}

InteropControl::~InteropControl() {
    closeSocket(_connection);
    closeSocket(_listener);
#if defined(_WIN32)
    if (_winsockInitialized) {
        ::WSACleanup();
    }
#endif
}

auto InteropControl::port() const noexcept -> std::uint16_t {
    return _port;
}

void InteropControl::acceptCounterpart(const std::string &expectedToken) {
    _connection =
        static_cast<NativeSocket>(::accept(static_cast<decltype(::socket(0, 0, 0))>(_listener), nullptr, nullptr));
    if (_connection == cInvalidSocket) {
        throw std::runtime_error{"timed out accepting interop counterpart: " + std::to_string(nativeError())};
    }
    setTimeout(_connection);
    const auto hello = receiveLine(_connection);
    if (jsonUnsigned(hello, "protocol") != std::optional<std::uint64_t>{1U} ||
        jsonString(hello, "type") != std::optional<std::string>{"hello"} ||
        jsonString(hello, "token") != std::optional<std::string>{expectedToken} ||
        jsonString(hello, "implementation") != std::optional<std::string>{"rustls"}) {
        throw std::runtime_error{"invalid hello message from interop counterpart"};
    }
}

auto InteropControl::request(const std::string &message, const std::uint64_t expectedId) -> Response {
    sendAll(_connection, message + "\n");
    auto response = decodeResponse(receiveLine(_connection));
    if (response.id != expectedId) {
        throw std::runtime_error{"interop response id does not match request"};
    }
    if (response.status != "ok") {
        throw std::runtime_error{
            "interop counterpart rejected request [" + response.errorCode + "]: " + response.errorMessage};
    }
    return response;
}

void InteropControl::closeSocket(const NativeSocket socket) noexcept {
    if (socket == cInvalidSocket) {
        return;
    }
#if defined(_WIN32)
    ::closesocket(static_cast<SOCKET>(socket));
#else
    ::close(static_cast<int>(socket));
#endif
}

void InteropControl::setTimeout(const NativeSocket socket) {
#if defined(_WIN32)
    const auto timeout = DWORD{10'000U};
    if (::setsockopt(
            static_cast<SOCKET>(socket),
            SOL_SOCKET,
            SO_RCVTIMEO,
            reinterpret_cast<const char *>(&timeout),
            static_cast<int>(sizeof(timeout))) != 0) {
#else
    const auto timeout = timeval{.tv_sec = 10, .tv_usec = 0};
    if (::setsockopt(static_cast<int>(socket), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) {
#endif
        throw std::runtime_error{"failed to configure interop control timeout: " + std::to_string(nativeError())};
    }
}

void InteropControl::sendAll(const NativeSocket socket, const std::string &text) {
    auto offset = std::size_t{};
    while (offset < text.size()) {
#if defined(_WIN32)
        const auto count =
            ::send(static_cast<SOCKET>(socket), text.data() + offset, static_cast<int>(text.size() - offset), 0);
#else
        const auto count = ::send(static_cast<int>(socket), text.data() + offset, text.size() - offset, 0);
#endif
        if (count <= 0) {
            throw std::runtime_error{"failed to write interop control channel: " + std::to_string(nativeError())};
        }
        offset += static_cast<std::size_t>(count);
    }
}

auto InteropControl::receiveLine(const NativeSocket socket) -> std::string {
    auto result = std::string{};
    auto buffer = std::array<char, 1024>{};
    while (true) {
#if defined(_WIN32)
        const auto count = ::recv(static_cast<SOCKET>(socket), buffer.data(), static_cast<int>(buffer.size()), 0);
#else
        const auto count = ::recv(static_cast<int>(socket), buffer.data(), buffer.size(), 0);
#endif
        if (count <= 0) {
            throw std::runtime_error{"failed to read interop control channel: " + std::to_string(nativeError())};
        }
        const auto view = std::string_view{buffer.data(), static_cast<std::size_t>(count)};
        const auto newline = view.find('\n');
        result.append(view.substr(0, newline));
        if (result.size() > cMaximumLineLength) {
            throw std::runtime_error{"interop control line exceeds 64 KiB"};
        }
        if (newline != std::string_view::npos) {
            break;
        }
    }
    if (!result.empty() && result.back() == '\r') {
        result.pop_back();
    }
    return result;
}

auto InteropControl::jsonString(const std::string &text, const std::string &key) -> std::optional<std::string> {
    const auto marker = std::string{"\""} + key + "\":";
    auto position = text.find(marker);
    if (position == std::string::npos) {
        return std::nullopt;
    }
    position += marker.size();
    while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) {
        ++position;
    }
    if (position >= text.size() || text[position] != '"') {
        return std::nullopt;
    }
    auto result = std::string{};
    for (++position; position < text.size(); ++position) {
        const auto character = text[position];
        if (character == '"') {
            return result;
        }
        if (character == '\\') {
            if (++position >= text.size()) {
                return std::nullopt;
            }
            const auto escaped = text[position];
            if (escaped == '"' || escaped == '\\' || escaped == '/') {
                result.push_back(escaped);
            } else if (escaped == 'n') {
                result.push_back('\n');
            } else if (escaped == 'r') {
                result.push_back('\r');
            } else if (escaped == 't') {
                result.push_back('\t');
            } else {
                return std::nullopt;
            }
        } else {
            result.push_back(character);
        }
    }
    return std::nullopt;
}

auto InteropControl::jsonUnsigned(const std::string &text, const std::string &key) -> std::optional<std::uint64_t> {
    const auto marker = std::string{"\""} + key + "\":";
    auto position = text.find(marker);
    if (position == std::string::npos) {
        return std::nullopt;
    }
    position += marker.size();
    while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position]))) {
        ++position;
    }
    if (position >= text.size() || !std::isdigit(static_cast<unsigned char>(text[position]))) {
        return std::nullopt;
    }
    auto value = std::uint64_t{};
    while (position < text.size() && std::isdigit(static_cast<unsigned char>(text[position]))) {
        value = value * 10U + static_cast<std::uint64_t>(text[position] - '0');
        ++position;
    }
    return value;
}

auto InteropControl::decodeResponse(const std::string &text) -> Response {
    if (jsonUnsigned(text, "protocol") != std::optional<std::uint64_t>{1U}) {
        throw std::runtime_error{"invalid interop response protocol"};
    }
    const auto id = jsonUnsigned(text, "id");
    const auto status = jsonString(text, "status");
    if (!id.has_value() || !status.has_value()) {
        throw std::runtime_error{"malformed interop response"};
    }
    return Response{
        .id = *id,
        .status = *status,
        .scenarioId = jsonUnsigned(text, "scenario_id"),
        .port = jsonUnsigned(text, "port"),
        .bytesReceived = jsonUnsigned(text, "bytes_received"),
        .accepted = jsonUnsigned(text, "accepted"),
        .parsed = jsonString(text, "parsed").value_or(std::string{}),
        .errorCode = jsonString(text, "error_code").value_or(std::string{}),
        .errorMessage = jsonString(text, "error_message").value_or(std::string{}),
    };
}

auto InteropControl::nativeError() -> int {
#if defined(_WIN32)
    return ::WSAGetLastError();
#else
    return errno;
#endif
}
