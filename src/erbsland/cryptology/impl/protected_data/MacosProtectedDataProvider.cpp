// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MacosProtectedDataProvider.hpp"

#include "MacosDataReference.hpp"

#include "../SecureEraseGuard.hpp"

#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteBuffer.hpp"
#include "../../../mem/Endianness.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../CryptologyError.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

MacosProtectedDataProvider::MacosProtectedDataProvider() {
    // Apple Security keychain contract: assign a random per-application tag so shutdown can delete this exact key.
    const auto uuid = CFUUIDCreate(kCFAllocatorDefault);
    const auto uuidBytes = CFUUIDGetUUIDBytes(uuid);
    _applicationTag = CFDataCreate(
        kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(&uuidBytes), static_cast<CFIndex>(sizeof(uuidBytes)));
    CFRelease(uuid);

    // Apple Secure Enclave guidance: restrict the permanent private key to this device, unlocked use, and private-key
    // operations. The enclave retains the private key material; this provider owns only its SecKey reference.
    auto accessError = CFErrorRef{};
    const auto accessControl = SecAccessControlCreateWithFlags(
        kCFAllocatorDefault,
        kSecAttrAccessibleWhenUnlockedThisDeviceOnly,
        kSecAccessControlPrivateKeyUsage,
        &accessError);
    if (accessControl == nullptr) {
        CFRelease(_applicationTag);
        _applicationTag = nullptr;
        throwError(accessError);
    }

    // Apple Security key-generation contract: request a 256-bit prime-random EC key from the Secure Enclave token and
    // store only the private key persistently under the application tag.
    const void *privateKeys[] = {kSecAttrIsPermanent, kSecAttrApplicationTag, kSecAttrAccessControl};
    const void *privateValues[] = {kCFBooleanTrue, _applicationTag, accessControl};
    const auto privateAttributes = CFDictionaryCreate(
        kCFAllocatorDefault,
        privateKeys,
        privateValues,
        3,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);
    const auto keySize = int32_t{256};
    const auto keySizeValue = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &keySize);
    const void *keys[] = {kSecAttrKeyType, kSecAttrKeySizeInBits, kSecAttrTokenID, kSecPrivateKeyAttrs};
    const void *values[] = {
        kSecAttrKeyTypeECSECPrimeRandom, keySizeValue, kSecAttrTokenIDSecureEnclave, privateAttributes};
    const auto attributes = CFDictionaryCreate(
        kCFAllocatorDefault, keys, values, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    auto error = CFErrorRef{};
    _privateKey = SecKeyCreateRandomKey(attributes, &error);
    CFRelease(attributes);
    CFRelease(keySizeValue);
    CFRelease(privateAttributes);
    CFRelease(accessControl);
    if (_privateKey == nullptr) {
        CFRelease(_applicationTag);
        _applicationTag = nullptr;
        throwError(error);
    }

    // Apple Security encryption contract: encryption uses the ordinary public key; decryption remains enclave-bound.
    // Reject initialization unless both directions support the exact cofactor-X9.63-SHA-256-AES-GCM construction.
    _publicKey = SecKeyCopyPublicKey(_privateKey);
    if (_publicKey == nullptr ||
        !SecKeyIsAlgorithmSupported(
            _publicKey, kSecKeyOperationTypeEncrypt, kSecKeyAlgorithmECIESEncryptionCofactorX963SHA256AESGCM) ||
        !SecKeyIsAlgorithmSupported(
            _privateKey, kSecKeyOperationTypeDecrypt, kSecKeyAlgorithmECIESEncryptionCofactorX963SHA256AESGCM)) {
        if (_publicKey != nullptr) {
            CFRelease(_publicKey);
            _publicKey = nullptr;
        }
        CFRelease(_privateKey);
        _privateKey = nullptr;
        const void *queryKeys[] = {kSecClass, kSecAttrApplicationTag};
        const void *queryValues[] = {kSecClassKey, _applicationTag};
        const auto query = CFDictionaryCreate(
            kCFAllocatorDefault,
            queryKeys,
            queryValues,
            2,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);
        SecItemDelete(query);
        CFRelease(query);
        CFRelease(_applicationTag);
        _applicationTag = nullptr;
        throw CryptologyError{"The Secure Enclave does not support protected-data encryption."_el};
    }
}

MacosProtectedDataProvider::~MacosProtectedDataProvider() {
    // Release public and private handles before deleting the tagged keychain item that owns the enclave key reference.
    if (_publicKey != nullptr) {
        CFRelease(_publicKey);
    }
    if (_privateKey != nullptr) {
        CFRelease(_privateKey);
    }
    if (_applicationTag != nullptr) {
        const void *queryKeys[] = {kSecClass, kSecAttrApplicationTag};
        const void *queryValues[] = {kSecClassKey, _applicationTag};
        const auto query = CFDictionaryCreate(
            kCFAllocatorDefault,
            queryKeys,
            queryValues,
            2,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);
        SecItemDelete(query);
        CFRelease(query);
        CFRelease(_applicationTag);
    }
}

auto MacosProtectedDataProvider::protect(const mem::ConstByteSpan plaintext, const unit::ByteLength plaintextLength)
    -> mem::ByteBlock {
    // Provider framing: prepend the fixed-width length inside the authenticated ECIES plaintext. The sensitive guard
    // erases this sole application-owned plaintext allocation on success and on every exceptional exit.
    auto input = mem::ByteBuffer{unit::ByteLength{8U + plaintext.size()}};
    input.setSensitive(true);
    const auto eraseGuard = SecureEraseGuard{input};
    input.setIntegerOrThrow<uint64_t>(
        unit::ByteIndex::zero(), static_cast<uint64_t>(plaintextLength.toSizeT()), mem::Endianness::Big);
    input.overwrite(unit::ByteIndex{8U}, plaintext);
    // Present the guarded allocation to Security without copying secret bytes into a second Core Foundation buffer.
    // kCFAllocatorNull keeps ownership and erasure responsibility with input for this synchronous operation.
    const auto inputData = MacosDataReference{CFDataCreateWithBytesNoCopy(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8 *>(input.span().data()),
        static_cast<CFIndex>(input.span().size()),
        kCFAllocatorNull)};
    if (inputData.isEmpty()) {
        throw CryptologyError{"Secure Enclave input allocation failed."_el};
    }

    // Apple Security performs cofactor ECDH, X9.63/SHA-256 derivation, and AES-GCM authentication as one operation.
    auto error = CFErrorRef{};
    const auto encrypted = MacosDataReference{SecKeyCreateEncryptedData(
        _publicKey, kSecKeyAlgorithmECIESEncryptionCofactorX963SHA256AESGCM, inputData.get(), &error)};
    if (encrypted.isEmpty()) {
        throwError(error);
    }
    auto result = mem::ByteBlock::fromSpan(
        std::span{
            reinterpret_cast<const std::byte *>(CFDataGetBytePtr(encrypted.get())),
            static_cast<std::size_t>(CFDataGetLength(encrypted.get()))});
    result.markAsSensitive();
    return result;
}

auto MacosProtectedDataProvider::unprotect(const mem::ConstByteSpan envelope, const unit::ByteLength plaintextLength)
    -> mem::ByteBlock {
    // The opaque envelope is copied into non-secret Core Foundation storage for the synchronous Security operation.
    const auto encrypted = MacosDataReference{CFDataCreate(
        kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(envelope.data()), static_cast<CFIndex>(envelope.size()))};
    if (encrypted.isEmpty()) {
        throw CryptologyError{"Secure Enclave envelope allocation failed."_el};
    }

    // Apple Security authenticates and decrypts the complete ECIES envelope before returning a plaintext CFData.
    auto error = CFErrorRef{};
    const auto decrypted = MacosDataReference{SecKeyCreateDecryptedData(
        _privateKey, kSecKeyAlgorithmECIESEncryptionCofactorX963SHA256AESGCM, encrypted.get(), &error)};
    if (decrypted.isEmpty()) {
        throwError(error);
    }

    // Copy the authenticated result immediately into erasable sensitive storage. Security owns the returned immutable
    // CFData allocation; its public API permits prompt release here but does not expose a supported explicit wipe.
    auto temporary = mem::ByteBuffer::fromSpan(
        std::span{
            reinterpret_cast<const std::byte *>(CFDataGetBytePtr(decrypted.get())),
            static_cast<std::size_t>(CFDataGetLength(decrypted.get()))});
    temporary.setSensitive(true);
    const auto eraseGuard = SecureEraseGuard{temporary};

    // The length field is inside the authenticated ECIES plaintext and binds the caller-visible block metadata.
    if (temporary.length().toSizeT() != 8U + plaintextLength.toSizeT() ||
        temporary.getIntegerOrThrow<uint64_t>(unit::ByteIndex::zero(), mem::Endianness::Big) !=
            plaintextLength.toSizeT()) {
        throw CryptologyError{"Protected-data length authentication failed."_el};
    }

    // Transfer only a sensitive copy of the payload; temporary retains and erases the framed plaintext allocation.
    auto result = mem::ByteBlock::fromSpan(temporary.span().subspan(8U));
    result.markAsSensitive();
    return result;
}

void MacosProtectedDataProvider::throwError(const CFErrorRef error) {
    if (error != nullptr) {
        const auto code = CFErrorGetCode(error);
        CFRelease(error);
        throw CryptologyError{text::StringFormat{"Secure Enclave protected-data operation failed ({})."_el}.build(
            static_cast<int64_t>(code))};
    }
    throw CryptologyError{"Secure Enclave protected-data operation failed."_el};
}

}
