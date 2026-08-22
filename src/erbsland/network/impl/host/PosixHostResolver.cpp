// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixHostResolver.hpp"

#include "HostResolverErrorContext.hpp"

#include "../../../system/PlatformError.hpp"
#include "../../../text/impl/PlatformU8StringAccess.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringConverter.hpp"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <memory>
#include <string_view>

namespace erbsland::network::impl {

using namespace text::literals;

auto PosixHostResolver::resolve(const HostName &hostName) -> util::List<IpAddress> {
    auto hints = addrinfo{};
    hints.ai_family = AF_UNSPEC;
    const auto asciiHostName = hostName.toString(HostNameFormat::IdnaAscii);
    const auto hostText = text::impl::PlatformU8StringAccess{asciiHostName};
    auto nativeResult = static_cast<addrinfo *>(nullptr);
    const auto errorCode = ::getaddrinfo(hostText.nullTerminatedCharPtr(), nullptr, &hints, &nativeResult);
    if (errorCode != 0) {
        const auto errorMessage = text::StringConverter{std::string_view{::gai_strerror(errorCode)}}.toString();
        throw system::PlatformError{
            "The platform host resolver failed."_el,
            std::make_shared<HostResolverErrorContext>(
                errorCode, errorMessage, errorCategory(errorCode), isTemporaryError(errorCode))};
    }
    const auto resultOwner = std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)>{nativeResult, &::freeaddrinfo};
    auto result = util::List<IpAddress>{};
    for (auto current = nativeResult; current != nullptr; current = current->ai_next) {
        const auto address = addressFrom(current->ai_addr, current->ai_family);
        if (address.has_value() && !result.contains(*address)) {
            result.append(*address);
        }
    }
    return result;
}

auto PosixHostResolver::addressFrom(const sockaddr *socketAddress, const int family) noexcept
    -> std::optional<IpAddress> {
    auto bytes = IpAddress::Bytes{};
    const auto source = [&]() noexcept -> const std::uint8_t * {
        if (family == AF_INET) {
            return reinterpret_cast<const std::uint8_t *>(
                &reinterpret_cast<const sockaddr_in *>(socketAddress)->sin_addr);
        }
        if (family == AF_INET6) {
            return reinterpret_cast<const std::uint8_t *>(
                &reinterpret_cast<const sockaddr_in6 *>(socketAddress)->sin6_addr);
        }
        return nullptr;
    }();
    if (source == nullptr) {
        return std::nullopt;
    }
    const auto byteCount = family == AF_INET ? std::size_t{4} : std::size_t{16};
    for (auto index = std::size_t{0}; index < byteCount; ++index) {
        bytes.set(unit::ByteIndex::fromSizeT(index), mem::Byte{source[index]});
    }
    return IpAddress::fromBytes(family == AF_INET ? IpVersion::V4 : IpVersion::V6, bytes);
}

auto PosixHostResolver::errorCategory(const int errorCode) noexcept -> system::PlatformErrorCategory {
    if (errorCode == EAI_NONAME) {
        return system::PlatformErrorCategory::NotFound;
    }
#if defined(EAI_NODATA) && EAI_NODATA != EAI_NONAME
    if (errorCode == EAI_NODATA) {
        return system::PlatformErrorCategory::NotFound;
    }
#endif
    return system::PlatformErrorCategory::Unknown;
}

auto PosixHostResolver::isTemporaryError(const int errorCode) noexcept -> bool {
    return errorCode == EAI_AGAIN;
}

auto createHostResolver() -> HostResolverPtr {
    return std::make_shared<PosixHostResolver>();
}

}
