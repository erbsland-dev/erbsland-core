// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all_network_source.hpp>
#include <erbsland/all_network_udp.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/ByteIntegerAccess.hpp>
#include <erbsland/cterm/BlockAttributes.hpp>
#include <erbsland/cterm/ColorPart.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/Endianness.hpp>
#include <erbsland/Events.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/IpEndpoint.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkError.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/OverflowError.hpp>
#include <erbsland/pattern/Text.hpp>
#include <erbsland/Port.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/Time.hpp>
#include <erbsland/TimeAmounts.hpp>
#include <erbsland/TimeEpoch.hpp>
#include <erbsland/Timestamp.hpp>
#include <erbsland/UdpDatagram.hpp>
#include <erbsland/UdpSocket.hpp>

#include <cstdint>
#include <utility>

namespace demo {

using namespace el::text::literals;
using namespace el::cterm;

/// A small RFC 868 UDP time service.
/// @notest{Compiled and exercised as part of the Time Server demo.}
class TimeServerApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override { info().setApplicationName("Time Server"_el); }
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->setHelpDescription("Provides the RFC 868 UDP time service."_el);
        options->addOption({"-a"_el, "--address"_el, "address"_el})
            .setType(el::OptionType::Text)
            .setHelpTitle("Local IPv4 or IPv6 address to bind."_el)
            .setDefaultValue("0.0.0.0"_el)
            .setValidateTextValue<el::Host>("The address argument does not specify a valid IP address"_el);
        options->addOption({"-p"_el, "--port"_el, "port"_el})
            .setType(el::OptionType::Text)
            .setHelpTitle("Local UDP port to bind."_el)
            .setDefaultValue("37"_el)
            .setValidateTextValue<el::Port>("The port option does not specify a valid port number"_el);
    }
    void parseCommandLine() override {
        Application::parseCommandLine();
        if (optionValues() == nullptr) {
            return;
        }
        _address = el::IpAddress::fromStringOrThrow(optionValues()->getText("address"_el));
        _port = el::Port::fromStringOrThrow(optionValues()->getText("port"_el));
        events()->invoke([this]() -> void { startServer(); });
    }

private:
    void startServer() {
        terminal()->printLine(fg::BrightWhite, BlockAttributes::Bold, "Time Server"_el);
        terminal()->printLine(fg::Cyan, "Starting "_el, el::IpEndpoint{_address, _port}.toString());
        _socket = events()->get<el::Network>().createUdpSocket();
        _socket->events()
            .onBound([this]() -> void { onBound(); })
            .onDatagram([this](const el::UdpDatagram &datagram) -> void { onDatagram(datagram); })
            .onError([this](const el::NetworkErrorContext &error) -> void { onError(error); });
        _socket->start(_address, _port);
    }

    void onBound() {
        terminal()->printLine(fg::BrightGreen, "Listening on "_el, _socket->localEndpoint().value().toString());
    }

    void onDatagram(const el::UdpDatagram &datagram) {
        uint8_t version = 1;
        if (datagram.data().length() > el::ByteCount::one()) {
            return;
        }
        if (!datagram.data().isEmpty()) {
            version = datagram.data().getIntegerOrThrow<uint8_t>(el::ByteIndex::zero());
            if (version < 1U || version > 2U) {
                return;
            }
        }
        const auto time = el::Timestamp::now();
        const auto ticks = time.toTicks<el::Seconds>(el::TimeEpoch::Rfc868);
        if (!ticks.has_value()) {
            // Stop the server for this unexpected error.
            throw el::ApplicationError{"The current date/time cannot be represented as an RFC 868 timestamp."_el};
        }
        auto response = el::ByteBlockEditor{};
        if (version == 1) {
            try {
                const auto timestamp = ticks.value().toValue().castOrThrow<std::uint32_t>();
                response.appendInteger(timestamp.toRawValue(), el::Endianness::Big);
            } catch (const el::OverflowError &) {
                return; // ignore the request if we cannot represent the timestamp
            }
        } else {
            const auto timestamp = ticks.value().toValue().castOrThrow<std::uint64_t>();
            response.appendInteger(timestamp.toRawValue(), el::Endianness::Big);
        }
        if (!_socket->send(datagram.remoteEndpoint(), response).isAccepted()) {
            return; // Ignore a full queue and skip this request.
        }
        terminal()->printLine(
            el::cterm::fg::BrightGreen,
            "Received request from "_el,
            datagram.remoteEndpoint().toString(),
            ", replied with "_el,
            time.toString());
    }

    [[noreturn]] void onError(const el::NetworkErrorContext &error) { throw el::NetworkError{error}; }

private:
    el::IpAddress _address;
    el::Port _port;
    el::UdpSocketPtr _socket;
};

}
