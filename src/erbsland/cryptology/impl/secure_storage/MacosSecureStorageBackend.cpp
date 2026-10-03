#include "MacosSecureStorageBackend.hpp"

#include "MacosPasswordBuffer.hpp"

#include "../MacosKeychainInteraction.hpp"

#include "../../../text/impl/PlatformU8StringAccess.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../SecureStorage.hpp"
#include "../../SecureStorageError.hpp"

#include <Security/cssmapple.h>

#include <cerrno>
// The standard user Keychain supports unsigned command-line clients and erasable native password buffers.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace erbsland::cryptology::impl {

using namespace text;
using text::impl::PlatformU8StringAccess;
using namespace text::literals;

auto MacosSecureStorageBackend::serviceName(const String &applicationNamespace) -> String {
    return StringFormat{"erbsland.secure-storage:{}"_el}.build(applicationNamespace);
}

auto MacosSecureStorageBackend::defaultKeychain() -> MacosOwnedReference<SecKeychainRef> {
    auto keychain = MacosOwnedReference<SecKeychainRef>{};
    const auto status = SecKeychainCopyDefault(keychain.put());
    if (status != errSecSuccess) {
        fail(status);
    }
    return keychain;
}

auto MacosSecureStorageBackend::read(
    const String &applicationNamespace, const String &item, const SecureStorageInteraction interaction)
    -> std::optional<ProtectedByteBlock> {
    const auto service = PlatformU8StringAccess{serviceName(applicationNamespace)};
    const auto account = PlatformU8StringAccess{item};
    auto data = static_cast<void *>(nullptr);
    auto length = UInt32{};
    auto status = OSStatus{};
    {
        const auto interactionGuard = MacosKeychainInteraction{interaction == SecureStorageInteraction::Allow};
        if (interactionGuard.status() != errSecSuccess) {
            fail(interactionGuard.status());
        }
        const auto keychain = defaultKeychain();
        status = SecKeychainFindGenericPassword(
            keychain.get(),
            static_cast<UInt32>(service.sizeWithoutNull()),
            service.nullTerminatedCharPtr(),
            static_cast<UInt32>(account.sizeWithoutNull()),
            account.nullTerminatedCharPtr(),
            &length,
            &data,
            nullptr);
    }
    const auto owner = MacosPasswordBuffer{data, length};
    if (status == errSecItemNotFound) {
        return {};
    }
    if (status != errSecSuccess) {
        fail(status);
    }
    if (length > SecureStorage::maximumPayloadLength().toSizeT()) {
        throw SecureStorageError{
            SecureStorageErrorCategory::SizeLimit, "Stored secret exceeds the 2,560-byte limit."_el};
    }
    // Transfer plaintext after releasing the native interaction lock. The owner erases the native buffer on success
    // and on every exceptional exit, before SecKeychainItemFreeContent releases it.
    return ProtectedByteBlock{mem::ConstByteSpan{static_cast<const mem::Byte *>(data), length}};
}

void MacosSecureStorageBackend::write(
    const String &applicationNamespace,
    const String &item,
    const mem::ConstByteSpan data,
    const SecureStorageInteraction interaction) {
    const auto service = PlatformU8StringAccess{serviceName(applicationNamespace)};
    const auto account = PlatformU8StringAccess{item};
    auto status = OSStatus{};
    {
        const auto interactionGuard = MacosKeychainInteraction{interaction == SecureStorageInteraction::Allow};
        if (interactionGuard.status() != errSecSuccess) {
            fail(interactionGuard.status());
        }
        const auto keychain = defaultKeychain();
        auto existing = MacosOwnedReference<SecKeychainItemRef>{};
        status = SecKeychainFindGenericPassword(
            keychain.get(),
            static_cast<UInt32>(service.sizeWithoutNull()),
            service.nullTerminatedCharPtr(),
            static_cast<UInt32>(account.sizeWithoutNull()),
            account.nullTerminatedCharPtr(),
            nullptr,
            nullptr,
            existing.put());
        if (status == errSecSuccess) {
            // Replace in place, preserving the old secret if update fails. Never delete before writing.
            status = SecKeychainItemModifyAttributesAndData(
                existing.get(), nullptr, static_cast<UInt32>(data.size()), data.data());
        } else if (status == errSecItemNotFound) {
            status = SecKeychainAddGenericPassword(
                keychain.get(),
                static_cast<UInt32>(service.sizeWithoutNull()),
                service.nullTerminatedCharPtr(),
                static_cast<UInt32>(account.sizeWithoutNull()),
                account.nullTerminatedCharPtr(),
                static_cast<UInt32>(data.size()),
                data.data(),
                nullptr);
            if (status == errSecDuplicateItem) {
                status = SecKeychainFindGenericPassword(
                    keychain.get(),
                    static_cast<UInt32>(service.sizeWithoutNull()),
                    service.nullTerminatedCharPtr(),
                    static_cast<UInt32>(account.sizeWithoutNull()),
                    account.nullTerminatedCharPtr(),
                    nullptr,
                    nullptr,
                    existing.put());
                if (status == errSecSuccess) {
                    status = SecKeychainItemModifyAttributesAndData(
                        existing.get(), nullptr, static_cast<UInt32>(data.size()), data.data());
                }
            }
        }
    }
    if (status != errSecSuccess) {
        fail(status);
    }
}

auto MacosSecureStorageBackend::remove(
    const String &applicationNamespace, const String &item, const SecureStorageInteraction interaction) -> bool {
    const auto service = PlatformU8StringAccess{serviceName(applicationNamespace)};
    const auto account = PlatformU8StringAccess{item};
    auto status = OSStatus{};
    {
        const auto interactionGuard = MacosKeychainInteraction{interaction == SecureStorageInteraction::Allow};
        if (interactionGuard.status() != errSecSuccess) {
            fail(interactionGuard.status());
        }
        const auto keychain = defaultKeychain();
        auto existing = MacosOwnedReference<SecKeychainItemRef>{};
        status = SecKeychainFindGenericPassword(
            keychain.get(),
            static_cast<UInt32>(service.sizeWithoutNull()),
            service.nullTerminatedCharPtr(),
            static_cast<UInt32>(account.sizeWithoutNull()),
            account.nullTerminatedCharPtr(),
            nullptr,
            nullptr,
            existing.put());
        if (status == errSecSuccess) {
            status = SecKeychainItemDelete(existing.get());
        }
    }
    if (status == errSecItemNotFound) {
        return false;
    }
    if (status != errSecSuccess) {
        fail(status);
    }
    return true;
}

void MacosSecureStorageBackend::fail(const OSStatus status) {
    auto category = SecureStorageErrorCategory::NativeFailure;
    if (status == errSecInteractionNotAllowed || status == errSecInteractionRequired) {
        category = SecureStorageErrorCategory::InteractionRequired;
    } else if (status == errSecUserCanceled) {
        category = SecureStorageErrorCategory::Cancelled;
    } else if (
        status == errSecAuthFailed || status == errSecNoAccessForItem || status == errSecErrnoBase + EPERM ||
        status == errSecErrnoBase + EACCES) {
        category = SecureStorageErrorCategory::AccessDenied;
    }
    throw SecureStorageError{category, StringFormat{"Keychain secret operation failed ({})."_el}.build(status)};
}

}

#pragma clang diagnostic pop
