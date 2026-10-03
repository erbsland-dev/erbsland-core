#include "MacosKeychainInteraction.hpp"

#include <exception>
// Standard user-Keychain interaction policy has no nondeprecated per-operation equivalent.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace erbsland::cryptology::impl {

std::mutex MacosKeychainInteraction::_mutex;

MacosKeychainInteraction::MacosKeychainInteraction(const bool allowInteraction) : _lock{_mutex} {
    // The standard user Keychain does not honor per-query authentication UI flags. Its interaction switch is global.
    _status = SecKeychainGetUserInteractionAllowed(&_previous);
    if (_status == errSecSuccess) {
        _status = SecKeychainSetUserInteractionAllowed(allowInteraction);
        _restore = _status == errSecSuccess;
    }
}

MacosKeychainInteraction::~MacosKeychainInteraction() {
    // Never silently leave a changed authorization policy behind, including during exception unwinding.
    if (_restore && SecKeychainSetUserInteractionAllowed(_previous) != errSecSuccess) {
        std::terminate();
    }
}

}

#pragma clang diagnostic pop
