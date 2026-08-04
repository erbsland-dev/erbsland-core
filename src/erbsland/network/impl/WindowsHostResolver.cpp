// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsHostResolver.hpp"

#include "HostResolverErrorContext.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../system/PlatformError.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"
#include "../../text/StringFormat.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <memory>
#include <string_view>

namespace erbsland::network::impl {

using namespace text::literals;

WindowsHostResolver::WindowsHostResolver() : _runtime{WindowsNetworkRuntime::shared()} {
}

auto WindowsHostResolver::resolve(const HostName &hostName) -> util::List<IpAddress> {
    auto hints = ADDRINFOW{};
    hints.ai_family = AF_UNSPEC;
    const auto hostText = text::StringConverter{hostName.toString(HostNameFormat::IdnaAscii)}.toStdWString();
    auto nativeResult = static_cast<PADDRINFOW>(nullptr);
    const auto errorCode = ::GetAddrInfoW(hostText.c_str(), nullptr, &hints, &nativeResult);
    if (errorCode != 0) {
        throw system::PlatformError{
            "The platform host resolver failed."_el,
            std::make_shared<HostResolverErrorContext>(
                errorCode, errorMessage(errorCode), errorCategory(errorCode), isTemporaryError(errorCode))};
    }
    const auto resultOwner = std::unique_ptr<ADDRINFOW, decltype(&::FreeAddrInfoW)>{nativeResult, &::FreeAddrInfoW};
    auto result = util::List<IpAddress>{};
    for (auto current = nativeResult; current != nullptr; current = current->ai_next) {
        const auto address = addressFrom(current->ai_addr, current->ai_family);
        if (address.has_value() && !result.contains(*address)) {
            result.append(*address);
        }
    }
    return result;
}

auto WindowsHostResolver::addressFrom(const sockaddr *socketAddress, const int family) noexcept
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

auto WindowsHostResolver::errorCategory(const int errorCode) noexcept -> system::PlatformErrorCategory {
    if (errorCode == WSAHOST_NOT_FOUND || errorCode == WSANO_DATA) {
        return system::PlatformErrorCategory::NotFound;
    }
    return system::PlatformErrorCategory::Unknown;
}

auto WindowsHostResolver::isTemporaryError(const int errorCode) noexcept -> bool {
    return errorCode == WSATRY_AGAIN;
}

auto WindowsHostResolver::errorMessage(const int errorCode) -> text::String {
    auto buffer = static_cast<wchar_t *>(nullptr);
    const auto length = ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        static_cast<DWORD>(errorCode),
        0,
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);
    if (length == 0 || buffer == nullptr) {
        return text::StringFormat{"Windows resolver error {}"_el}.build(errorCode);
    }
    const auto owner = std::unique_ptr<wchar_t, decltype(&::LocalFree)>{buffer, &::LocalFree};
    return text::StringConverter{std::wstring_view{buffer, static_cast<std::size_t>(length)}}.toString().trimmed();
}

auto createHostResolver() -> HostResolverPtr {
    return std::make_shared<WindowsHostResolver>();
}

}
