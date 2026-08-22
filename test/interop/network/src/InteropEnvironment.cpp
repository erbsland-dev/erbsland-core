// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "InteropEnvironment.hpp"

#include <erbsland/path/Path.hpp>
#include <erbsland/system/SubprocessOptions.hpp>
#include <erbsland/system/SubprocessOutputMode.hpp>
#include <erbsland/text/StringConverter.hpp>

#include <chrono>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>

InteropEnvironment::Scenario::Scenario(
    InteropEnvironment &environment, const std::uint64_t id, const std::uint16_t port) noexcept :
    _environment{&environment}, _id{id}, _port{port} {
}

InteropEnvironment::Scenario::~Scenario() {
    cancel();
}

InteropEnvironment::Scenario::Scenario(Scenario &&other) noexcept :
    _environment{std::exchange(other._environment, nullptr)},
    _id{std::exchange(other._id, 0U)},
    _port{std::exchange(other._port, 0U)} {
}

auto InteropEnvironment::Scenario::operator=(Scenario &&other) noexcept -> Scenario & {
    if (this != &other) {
        cancel();
        _environment = std::exchange(other._environment, nullptr);
        _id = std::exchange(other._id, 0U);
        _port = std::exchange(other._port, 0U);
    }
    return *this;
}

auto InteropEnvironment::Scenario::port() const noexcept -> std::uint16_t {
    return _port;
}

auto InteropEnvironment::Scenario::finish() -> std::uint64_t {
    if (_environment == nullptr) {
        throw std::runtime_error{"interop scenario is no longer active"};
    }
    const auto bytes = _environment->finishScenario(_id);
    _environment = nullptr;
    return bytes;
}

void InteropEnvironment::Scenario::cancel() noexcept {
    if (_environment != nullptr) {
        _environment->cancelScenario(_id);
        _environment = nullptr;
    }
}

InteropEnvironment::InteropEnvironment(const std::filesystem::path &testExecutable) {
    const auto executableDirectory = std::filesystem::absolute(testExecutable).parent_path();
#if defined(_WIN32)
    const auto counterpartPath = executableDirectory / "erbsland-core-network-interop-counterpart.exe";
#else
    const auto counterpartPath = executableDirectory / "erbsland-core-network-interop-counterpart";
#endif
    const auto dataDirectory = executableDirectory / "data" / "network" / "tls-interop";
    const auto authenticationToken = token();
    const auto toString = [](const std::string &value) -> erbsland::text::String {
        return erbsland::text::StringConverter{value}.toString();
    };
    auto options = erbsland::system::SubprocessOptions{};
    options.setInheritStandardInput(false)
        .setStandardOutputMode(erbsland::system::SubprocessOutputMode::Discard)
        .setStandardErrorMode(erbsland::system::SubprocessOutputMode::Capture);
    _counterpart.emplace(
        erbsland::system::Subprocess::start(
            erbsland::path::Path{counterpartPath},
            erbsland::text::StringList{
                toString("--control-port"),
                toString(std::to_string(_control.port())),
                toString("--token"),
                toString(authenticationToken),
                toString("--certificate"),
                toString((dataDirectory / "server.pem").string()),
                toString("--private-key"),
                toString((dataDirectory / "server-key.pem").string()),
                toString("--trust-certificate"),
                toString((dataDirectory / "ca.pem").string()),
                toString("--trust-certificate"),
                toString((dataDirectory / "server-ecdsa.pem").string()),
                toString("--trust-certificate"),
                toString((dataDirectory / "server-ed25519.pem").string()),
            },
            options));
    try {
        _control.acceptCounterpart(authenticationToken);
    } catch (...) {
        const auto errorOutput = erbsland::text::StringConverter{_counterpart->standardError()}.toStdString();
        if (!errorOutput.empty()) {
            throw std::runtime_error{"failed to start interop counterpart: " + errorOutput};
        }
        throw;
    }
}

InteropEnvironment::~InteropEnvironment() {
    if (_counterpart.has_value()) {
        try {
            const auto id = nextId();
            static_cast<void>(
                _control.request("{\"protocol\":1,\"id\":" + std::to_string(id) + ",\"command\":\"shutdown\"}", id));
            static_cast<void>(_counterpart->wait(erbsland::time::TimeDelta::seconds(2)));
        } catch (...) {
            // Subprocess ownership guarantees termination and reaping after a failed shutdown.
        }
    }
    if (_instance == this) {
        _instance = nullptr;
    }
}

void InteropEnvironment::install(InteropEnvironment &environment) noexcept {
    _instance = &environment;
}

auto InteropEnvironment::instance() -> InteropEnvironment & {
    if (_instance == nullptr) {
        throw std::runtime_error{"the interop environment is not installed"};
    }
    return *_instance;
}

auto InteropEnvironment::startScenario(
    const std::string &scenario, const std::string &cipher, const std::uint64_t payloadLength) -> Scenario {
    const auto id = nextId();
    const auto message = "{\"protocol\":1,\"id\":" + std::to_string(id) + ",\"command\":\"start\",\"scenario\":\"" +
        jsonEscape(scenario) + "\",\"cipher\":\"" + jsonEscape(cipher) +
        "\",\"payload_length\":" + std::to_string(payloadLength) + "}";
    const auto response = _control.request(message, id);
    if (!response.scenarioId.has_value() || !response.port.has_value() || *response.scenarioId != id ||
        *response.port == 0U || *response.port > 65535U) {
        throw std::runtime_error{"invalid start response from interop counterpart"};
    }
    return Scenario{*this, id, static_cast<std::uint16_t>(*response.port)};
}

auto InteropEnvironment::startClientScenario(
    const std::uint16_t port,
    const std::string &scenario,
    const std::string &cipher,
    const std::uint64_t payloadLength,
    const std::string &serverName) -> Scenario {
    const auto id = nextId();
    const auto message = "{\"protocol\":1,\"id\":" + std::to_string(id) + ",\"command\":\"connect\",\"scenario\":\"" +
        jsonEscape(scenario) + "\",\"cipher\":\"" + jsonEscape(cipher) +
        "\",\"payload_length\":" + std::to_string(payloadLength) + ",\"port\":" + std::to_string(port) +
        ",\"server_name\":\"" + jsonEscape(serverName) + "\"}";
    const auto response = _control.request(message, id);
    if (!response.scenarioId.has_value() || *response.scenarioId != id) {
        throw std::runtime_error{"invalid client-scenario response from interop counterpart"};
    }
    return Scenario{*this, id, port};
}

auto InteropEnvironment::compareHttp(const std::string &kind, const std::string &wireHex) -> InteropControl::Response {
    const auto id = nextId();
    return _control.request(
        "{\"protocol\":1,\"id\":" + std::to_string(id) + ",\"command\":\"http-" + jsonEscape(kind) +
            "\",\"wire_hex\":\"" + jsonEscape(wireHex) + "\"}",
        id);
}

auto InteropEnvironment::nextId() noexcept -> std::uint64_t {
    return _nextId++;
}

auto InteropEnvironment::finishScenario(const std::uint64_t scenarioId) -> std::uint64_t {
    const auto id = nextId();
    const auto response = _control.request(
        "{\"protocol\":1,\"id\":" + std::to_string(id) +
            ",\"command\":\"finish\",\"scenario_id\":" + std::to_string(scenarioId) + "}",
        id);
    if (!response.bytesReceived.has_value()) {
        throw std::runtime_error{"finish response has no byte count"};
    }
    return *response.bytesReceived;
}

void InteropEnvironment::cancelScenario(const std::uint64_t scenarioId) noexcept {
    try {
        const auto id = nextId();
        static_cast<void>(_control.request(
            "{\"protocol\":1,\"id\":" + std::to_string(id) +
                ",\"command\":\"cancel\",\"scenario_id\":" + std::to_string(scenarioId) + "}",
            id));
    } catch (...) {
        // Scenario cleanup must not hide the original test failure.
    }
}

auto InteropEnvironment::jsonEscape(const std::string &value) -> std::string {
    auto result = std::string{};
    result.reserve(value.size());
    for (const auto character : value) {
        if (character == '"' || character == '\\') {
            result.push_back('\\');
        }
        result.push_back(character);
    }
    return result;
}

auto InteropEnvironment::token() -> std::string {
    const auto seed = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    auto generator = std::mt19937_64{seed ^ std::random_device{}()};
    auto stream = std::ostringstream{};
    stream << std::hex << generator() << generator();
    return stream.str();
}
