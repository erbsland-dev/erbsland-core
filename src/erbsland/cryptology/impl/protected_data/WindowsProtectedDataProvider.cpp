// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsProtectedDataProvider.hpp"

#include "WindowsLocalBuffer.hpp"

#include "../SecureEraseGuard.hpp"

#include "../../../core/Application.hpp"
#include "../../../mem/ByteBuffer.hpp"
#include "../../../mem/Endianness.hpp"
#include "../../../random/Random.hpp"
#include "../../../text/Literals.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../CryptologyError.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

WindowsProtectedDataProvider::WindowsProtectedDataProvider() :
    _api{}, _contextId{core::application().secureRandom().buildByteBlock(unit::ByteLength{cContextSize})} {
    // Microsoft DPAPI-NG descriptor contract: LOCAL=logon binds OS protection to the current Windows logon.
    // The random context adds the narrower application-service lifetime checked after authenticated decryption.
    if (_api.createProtectionDescriptorFn()(L"LOCAL=logon", 0U, &_descriptor) != ERROR_SUCCESS) {
        throw CryptologyError{"DPAPI-NG protected-data initialization failed."_el};
    }
}

WindowsProtectedDataProvider::~WindowsProtectedDataProvider() {
    // Release the native descriptor, then erase the application-lifetime context while its storage is still valid.
    if (_descriptor != nullptr) {
        _api.closeProtectionDescriptorFn()(_descriptor);
    }
    _contextId.secureErase();
}

auto WindowsProtectedDataProvider::protect(const mem::ConstByteSpan plaintext, const unit::ByteLength plaintextLength)
    -> mem::ByteBlock {
    // Provider framing: length || random application context || plaintext. DPAPI-NG authenticates the whole input.
    // The guard erases this application-owned plaintext allocation on success and every exceptional exit.
    auto input = mem::ByteBuffer{unit::ByteLength{8U + cContextSize + plaintext.size()}};
    input.setSensitive(true);
    const auto eraseGuard = SecureEraseGuard{input};
    input.setIntegerOrThrow<uint64_t>(
        unit::ByteIndex::zero(), static_cast<uint64_t>(plaintextLength.toSizeT()), mem::Endianness::Big);
    input.overwrite(unit::ByteIndex{8U}, _contextId.span());
    input.overwrite(unit::ByteIndex{8U + cContextSize}, plaintext);

    // Microsoft NCryptProtectSecret contract: use the descriptor silently and retain only its opaque output blob.
    auto output = WindowsLocalBuffer{false};
    const auto inputBytes = reinterpret_cast<const BYTE *>(input.span().data());
    const auto status = _api.protectSecretFn()(
        _descriptor,
        NCRYPT_SILENT_FLAG,
        const_cast<PBYTE>(inputBytes),
        static_cast<ULONG>(input.span().size()),
        nullptr,
        nullptr,
        output.dataAddress(),
        output.sizeAddress());
    if (status != ERROR_SUCCESS) {
        throw CryptologyError{"DPAPI-NG protected-data encryption failed."_el};
    }
    auto result = mem::ByteBlock::fromSpan(output.span());
    result.markAsSensitive();
    return result;
}

auto WindowsProtectedDataProvider::unprotect(const mem::ConstByteSpan envelope, const unit::ByteLength plaintextLength)
    -> mem::ByteBlock {
    // Microsoft NCryptUnprotectSecret authenticates the opaque blob and allocates plaintext with LocalAlloc. Mark the
    // native guard sensitive before the operation so partial-success and exceptional paths wipe any returned bytes.
    auto output = WindowsLocalBuffer{true};
    const auto envelopeBytes = reinterpret_cast<const BYTE *>(envelope.data());
    const auto status = _api.unprotectSecretFn()(
        nullptr,
        NCRYPT_SILENT_FLAG,
        const_cast<PBYTE>(envelopeBytes),
        static_cast<ULONG>(envelope.size()),
        nullptr,
        nullptr,
        output.dataAddress(),
        output.sizeAddress());
    if (status != ERROR_SUCCESS) {
        throw CryptologyError{"DPAPI-NG protected-data decryption failed."_el};
    }

    // Copy into library-owned sensitive storage; both this copy and the DPAPI-NG LocalAlloc buffer are independently
    // erased when their guards leave scope.
    auto temporary = mem::ByteBuffer::fromSpan(output.span());
    temporary.setSensitive(true);
    const auto eraseGuard = SecureEraseGuard{temporary};

    // Verify the authenticated length and application context before exposing any plaintext payload.
    if (temporary.length().toSizeT() != 8U + cContextSize + plaintextLength.toSizeT() ||
        temporary.getIntegerOrThrow<uint64_t>(unit::ByteIndex::zero(), mem::Endianness::Big) !=
            plaintextLength.toSizeT() ||
        !_contextId.isEqualConstTime(temporary.span().subspan(8U, cContextSize))) {
        throw CryptologyError{"Protected-data length authentication failed."_el};
    }

    // Transfer only a sensitive copy of the payload; temporary retains and erases the framed plaintext allocation.
    auto result = mem::ByteBlock::fromSpan(temporary.span().subspan(8U + cContextSize));
    result.markAsSensitive();
    return result;
}

}
