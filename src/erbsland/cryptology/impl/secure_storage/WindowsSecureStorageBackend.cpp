#include "WindowsSecureStorageBackend.hpp"

#include "WindowsCredentialBuffer.hpp"

#include "../../../text/impl/PlatformU16StringAccess.hpp"
#include "../../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringConverter.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../SecureStorage.hpp"
#include "../../SecureStorageError.hpp"

namespace erbsland::cryptology::impl {

using namespace mem;
using namespace text;
using text::impl::PlatformU16StringAccess;
using text::impl::UnsafeU8StringAccess;
using namespace text::literals;

auto WindowsSecureStorageBackend::targetName(const String &applicationNamespace, const String &item) -> U16String {
    // Credential Manager compares target names case-insensitively. Encode exact UTF-8 bytes so distinct spelling
    // remains distinct; the separator cannot occur within either hexadecimal component.
    const auto namespaceBytes = ByteBlock::fromSpan(UnsafeU8StringAccess{applicationNamespace}.dataSpan());
    const auto itemBytes = ByteBlock::fromSpan(UnsafeU8StringAccess{item}.dataSpan());
    return StringConverter{StringFormat{"erbsland.secure-storage:{}:{}"_el}.build(
                               String::fromByteBlock(namespaceBytes), String::fromByteBlock(itemBytes))}
        .toU16String();
}

auto WindowsSecureStorageBackend::read(
    const String &applicationNamespace, const String &item, const SecureStorageInteraction)
    -> std::optional<ProtectedByteBlock> {
    const auto target = PlatformU16StringAccess{targetName(applicationNamespace, item)};
    auto credential = PCREDENTIALW{};
    if (!CredReadW(target.nullTerminatedWideCharPtr(), CRED_TYPE_GENERIC, 0, &credential)) {
        const auto error = GetLastError();
        if (error == ERROR_NOT_FOUND) {
            return {};
        }
        fail(error);
    }
    const auto owner = WindowsCredentialBuffer{credential};
    if (credential->CredentialBlobSize > SecureStorage::maximumPayloadLength().toSizeT()) {
        throw SecureStorageError{
            SecureStorageErrorCategory::SizeLimit, "Stored secret exceeds the 2,560-byte limit."_el};
    }
    return ProtectedByteBlock{toConstByteSpan(
        std::span{credential->CredentialBlob, static_cast<std::size_t>(credential->CredentialBlobSize)})};
}

void WindowsSecureStorageBackend::write(
    const String &applicationNamespace, const String &item, const ConstByteSpan data, const SecureStorageInteraction) {
    const auto target = PlatformU16StringAccess{targetName(applicationNamespace, item)};
    auto credential = CREDENTIALW{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<wchar_t *>(target.nullTerminatedWideCharPtr());
    credential.CredentialBlobSize = static_cast<DWORD>(data.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<Byte *>(data.data()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    // CredWriteW atomically upserts the credential. It borrows the caller's plaintext for this call only.
    if (!CredWriteW(&credential, 0)) {
        fail(GetLastError());
    }
}

auto WindowsSecureStorageBackend::remove(
    const String &applicationNamespace, const String &item, const SecureStorageInteraction) -> bool {
    const auto target = PlatformU16StringAccess{targetName(applicationNamespace, item)};
    if (!CredDeleteW(target.nullTerminatedWideCharPtr(), CRED_TYPE_GENERIC, 0)) {
        const auto error = GetLastError();
        if (error == ERROR_NOT_FOUND) {
            return false;
        }
        fail(error);
    }
    return true;
}

void WindowsSecureStorageBackend::fail(const DWORD error) {
    auto category = SecureStorageErrorCategory::NativeFailure;
    if (error == ERROR_ACCESS_DENIED || error == ERROR_NO_SUCH_LOGON_SESSION) {
        category = SecureStorageErrorCategory::AccessDenied;
    } else if (error == ERROR_CANCELLED) {
        category = SecureStorageErrorCategory::Cancelled;
    } else if (error == ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION) {
        category = SecureStorageErrorCategory::InteractionRequired;
    }
    throw SecureStorageError{
        category, StringFormat{"Credential Manager secret operation failed ({})."_el}.build(error)};
}

}
