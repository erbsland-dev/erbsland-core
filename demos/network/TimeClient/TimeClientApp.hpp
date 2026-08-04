// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/network/host_lookup/all.hpp>
#include <erbsland/network/source/all.hpp>
#include <erbsland/network/udp/all.hpp>

#include <cstdint>
#include <utility>

namespace demo {

using namespace el::text::literals;
using namespace el::cterm;

/// A small RFC 868 UDP time client.
class TimeClientApp final : public el::Application {
private:
    /// The current stage of the one-request exchange.
    enum class State : uint8_t {
        Resolving,          ///< Resolving the server host.
        Binding,            ///< Binding the local UDP socket.
        WaitingForResponse, ///< Waiting for the expected server reply.
        Closing,            ///< Closing the local UDP socket.
    };

public:
    using Application::Application;

protected: // implement Application
    void initialize() override {
        // Set the application name for the command line help.
        info().setApplicationName("Time Client"_el);
    }

    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Queries an RFC 868 UDP time service."_el);
        options->addOption("host"_el)
            .setRequired()
            .setHelpDescription("Server host name or IP address."_el)
            .setValidateTextValue<el::Host>("The host argument does not specify a valid host name or IP address"_el);
        options->addOption({"-p"_el, "--port"_el, "port"_el})
            .setType(el::OptionType::Text)
            .setHelpDescription("Server UDP port."_el)
            .setDefaultValue("37"_el)
            .setValidateTextValue<el::Port>("The port option does not specify a valid port number"_el);
        options->addOption({"--protocol"_el, "protocol"_el})
            .setType(el::OptionType::Integer)
            .setValueName("version"_el)
            .setHelpDescription("Server protocol version. 1 = RFC868, 2 = 64bit."_el)
            .setDefaultValue(1)
            .setValidateFn([](const el::OptionValuePtr &value, const el::OptionValuesPtr &) -> void {
                if (value->getInteger() < 1 || value->getInteger() > 2) {
                    throw el::OptionError("The protocol must be 1 or 2"_el);
                }
            });
    }

    void parseCommandLine() override {
        Application::parseCommandLine();
        if (optionValues() == nullptr) {
            return;
        }

        _host = el::Host::fromStringOrThrow(optionValues()->getText("host"_el));
        _port = el::Port::fromStringOrThrow(optionValues()->getText("port"_el));
        _protocol = el::saturatingCast<uint8_t>(optionValues()->getInteger("protocol"_el));
        events()->invoke([this]() -> void { startClient(); });
    }

private:
    void startClient() {
        _state = State::Resolving;
        terminal()->printLine(fg::BrightWhite, BlockAttributes::Bold, "Time Client"_el);
        terminal()->printLine(fg::Cyan, "Resolving "_el, _host.toString());

        _lookup = events()->get<el::Network>().createHostLookup();
        _lookup->events()
            .onResolved([this](const el::List<el::IpAddress> &addresses) -> void { onResolved(addresses); })
            .onError([this](const el::NetworkErrorContext &error) -> void { onError(error); });
        _lookup->start(_host);
    }

    void onResolved(const el::List<el::IpAddress> &addresses) {
        if (addresses.isEmpty()) {
            throw el::ApplicationError{"The server host did not resolve to an IP address."_el};
        }

        _serverEndpoint = el::IpEndpoint{addresses.first(), _port};
        terminal()->printLine(
            el::cterm::fg::BrightBlack,
            "Creating a "_el,
            (_serverEndpoint.address().isV4() ? "IPv4"_el : "IPv6"_el),
            " socket"_el);

        _socket = events()->get<el::Network>().createUdpSocket();
        _socket->events()
            .onBound([this]() -> void { onBound(); })
            .onDatagram([this](const el::UdpDatagram &datagram) -> void { onDatagram(datagram); })
            .onClosed([this]() -> void { onClosed(); })
            .onError([this](const el::NetworkErrorContext &error) -> void { onError(error); });
        _state = State::Binding;
        _socket->start(_serverEndpoint.address().isV4() ? el::IpAddress::anyV4() : el::IpAddress::anyV6());
    }

    void onBound() {
        terminal()->printLine(fg::BrightGreen, "Bound to "_el, _socket->localEndpoint().value().toString());
        terminal()->printLine(fg::BrightBlack, "Sending time request to "_el, _serverEndpoint.toString());
        _state = State::WaitingForResponse;
        el::ByteBlock data;
        if (_protocol == 2) {
            data = el::ByteBlock{el::Byte{0x02U}};
        }
        if (!_socket->send(_serverEndpoint, data).isAccepted()) {
            throw el::ApplicationError{"The time request could not be queued."_el};
        }
        terminal()->printLine(fg::BrightBlack, "Waiting for response"_el);
        _timeout = events()->createTimer([this]() -> void { onTimeout(); });
        _timeout->startFixedDelay(el::TimeDelta::seconds(10));
    }

    void onDatagram(const el::UdpDatagram &datagram) {
        if (_state != State::WaitingForResponse || datagram.remoteEndpoint() != _serverEndpoint) {
            return; // ignore random packets from unknown sources
        }
        const auto dataSize = datagram.data().length();
        if (dataSize != el::ByteLength{4} && dataSize != el::ByteLength{8}) {
            return; // ignore invalid lengths
        }
        el::Seconds ticks;
        if (dataSize == el::ByteLength{4}) {
            ticks = el::Seconds{
                static_cast<int64_t>(datagram.data().getInteger<std::uint32_t>(el::ByteIndex{}, el::Endianness::Big))};
        } else {
            ticks = el::Seconds{el::saturatingCast<int64_t>(
                datagram.data().getInteger<std::uint64_t>(el::ByteIndex{}, el::Endianness::Big))};
        }
        const auto time = el::DateTime::fromTicks(ticks, el::TimeEpoch::Rfc868).value_or(el::DateTime{});
        terminal()->printLine(fg::BrightGreen, "Received response from "_el, datagram.remoteEndpoint().toString());
        terminal()->printLine(fg::BrightWhite, "Time is "_el, time.toIsoString());
        _state = State::Closing;
        _socket->close();
    }

    void onClosed() {
        terminal()->printLine(fg::BrightBlack, "Closed socket"_el);
        quit();
    }

    void onTimeout() {
        // Throwing from the Application main loop will gracefully terminate the application.
        throw el::ApplicationError{"The time request timed out"_el};
    }

    void onError(const el::NetworkErrorContext &error) {
        // This will terminate the application and report the network error.
        throw el::NetworkError{error};
    }

private:
    State _state{State::Resolving};
    el::Host _host;
    el::Port _port;
    uint8_t _protocol;
    el::IpEndpoint _serverEndpoint;
    el::HostLookupPtr _lookup;
    el::UdpSocketPtr _socket;
    el::EventTimerPtr _timeout;
};

}
