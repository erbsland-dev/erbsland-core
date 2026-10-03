#pragma once

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/impl/MacosKeychainInteraction.hpp>
#include <erbsland/cryptology/impl/MacosOwnedReference.hpp>
#include <erbsland/CryptologyError.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/text/impl/PlatformU8StringAccess.hpp>
#include <Security/Security.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

/// Temporary native identity store that never imports into or selects personal user identities.
/// Its public test-key PKCS#12 containers use platform-compatible legacy wrapping solely for test import.
/// This does not enable legacy TLS algorithms or private-key codecs in the library.
class MacosIdentityTestFixture final {
public:
    /// Import the named public test fixture into a newly created isolated keychain.
    explicit MacosIdentityTestFixture(const char *fixture) {
        using namespace el::text::literals;
        using namespace el::cryptology;
        using namespace el::cryptology::impl;
        _directory = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto path = _directory->path() / "identity-test.keychain"_el;
        const auto pathAccess = el::text::impl::PlatformU8StringAccess{path.toString()};
        const auto data =
            CryptologyTestHelper::bytesFromHex(el::text::String{el::unittest::fh::readDataText(fixture)}.trimmed());
        const auto nativeData = MacosOwnedReference<CFDataRef>{CFDataCreate(
            kCFAllocatorDefault,
            reinterpret_cast<const UInt8 *>(data.span().data()),
            static_cast<CFIndex>(data.length().toSizeT()))};
        const auto options = MacosOwnedReference<CFMutableDictionaryRef>{CFDictionaryCreateMutable(
            kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
        auto originalSearchList = MacosOwnedReference<CFArrayRef>{};
        auto imported = MacosOwnedReference<CFArrayRef>{};
        auto status = OSStatus{};
        {
            const auto interaction = MacosKeychainInteraction{false};
            status = interaction.status();
            if (status == errSecSuccess) {
                status = SecKeychainCopySearchList(originalSearchList.put());
            }
            if (status == errSecSuccess) {
                status = SecKeychainCreate(
                    pathAccess.nullTerminatedCharPtr(), 12, "fixture-only", false, nullptr, _keychain.put());
            }
            if (originalSearchList.get() != nullptr &&
                SecKeychainSetSearchList(originalSearchList.get()) != errSecSuccess) {
                std::terminate();
            }
            if (status == errSecSuccess) {
                CFDictionarySetValue(options.get(), kSecImportExportPassphrase, CFSTR("fixture-only"));
                CFDictionarySetValue(options.get(), kSecImportExportKeychain, _keychain.get());
                status = SecPKCS12Import(nativeData.get(), options.get(), imported.put());
            }
        }
        if (status != errSecSuccess) {
            clear();
            throw CryptologyError{
                el::text::StringFormat{"Isolated Keychain fixture import failed: {}."_el}.build(status)};
        }
    }
    /// Delete the isolated keychain and its fixture keys.
    ~MacosIdentityTestFixture() { clear(); }
    /// Borrow the isolated keychain for a scoped internal backend test.
    [[nodiscard]] auto keychain() const noexcept -> SecKeychainRef { return _keychain.get(); }

private:
    /// Remove only the temporary keychain created by this fixture.
    void clear() noexcept {
        if (_keychain.get() != nullptr) {
            const auto interaction = el::cryptology::impl::MacosKeychainInteraction{false};
            if (interaction.status() != errSecSuccess || SecKeychainDelete(_keychain.get()) != errSecSuccess) {
                std::terminate();
            }
            _keychain.reset();
        }
    }

private:
    el::path::TempDirectoryPtr _directory;
    el::cryptology::impl::MacosOwnedReference<SecKeychainRef> _keychain;
};

#pragma clang diagnostic pop
