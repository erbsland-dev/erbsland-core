// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SyslogLogWriterOptions.hpp"

#include "../err/ParameterError.hpp"
#include "../network/IpAddress.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::log {

using namespace text::literals;

SyslogLogWriterOptions::SyslogLogWriterOptions() :
    _endpoint{network::Host{network::IpAddress::loopbackV4()}, network::Port{514U}} {
}

auto SyslogLogWriterOptions::setTransport(const SyslogTransport value) noexcept -> SyslogLogWriterOptions & {
    _transport = value;
    return *this;
}

auto SyslogLogWriterOptions::setEndpoint(network::HostEndpoint value) noexcept -> SyslogLogWriterOptions & {
    _endpoint = std::move(value);
    return *this;
}

auto SyslogLogWriterOptions::setFacility(const uint8_t value) -> SyslogLogWriterOptions & {
    if (value > 23U) {
        throw err::ParameterError{"A syslog facility must be in the range zero through 23."_el, "value"_el};
    }
    _facility = value;
    return *this;
}

auto SyslogLogWriterOptions::setHostName(text::String value) -> SyslogLogWriterOptions & {
    _hostName = std::move(value);
    return *this;
}

auto SyslogLogWriterOptions::setApplicationName(text::String value) -> SyslogLogWriterOptions & {
    _applicationName = std::move(value);
    return *this;
}

auto SyslogLogWriterOptions::setProcessId(text::String value) -> SyslogLogWriterOptions & {
    _processId = std::move(value);
    return *this;
}

auto SyslogLogWriterOptions::setMessageId(text::String value) -> SyslogLogWriterOptions & {
    _messageId = std::move(value);
    return *this;
}

auto SyslogLogWriterOptions::setTlsConfigurationLabel(text::String value) -> SyslogLogWriterOptions & {
    if (value.isEmpty()) {
        throw err::ParameterError{"A TLS configuration label must not be empty."_el, "value"_el};
    }
    _tlsLabel = std::move(value);
    return *this;
}

auto SyslogLogWriterOptions::setMaximumPendingBytes(const unit::ByteLength value) -> SyslogLogWriterOptions & {
    if (value.isZero()) {
        throw err::ParameterError{"The syslog pending-data limit must be positive."_el, "value"_el};
    }
    _maximumPendingBytes = value;
    return *this;
}

}
