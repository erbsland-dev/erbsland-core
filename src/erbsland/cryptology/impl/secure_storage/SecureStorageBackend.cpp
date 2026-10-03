#include "SecureStorageBackend.hpp"

#include "../../../core/Definitions.hpp"

#if defined(ERBSLAND_OS_MACOS)
#include "MacosSecureStorageBackend.hpp"
#elif defined(ERBSLAND_OS_WINDOWS)
#include "WindowsSecureStorageBackend.hpp"
#else
#include "UnsupportedSecureStorageBackend.hpp"
#endif

namespace erbsland::cryptology::impl {

auto SecureStorageBackend::create() -> std::shared_ptr<SecureStorageBackend> {
#if defined(ERBSLAND_OS_MACOS)
    return std::make_shared<MacosSecureStorageBackend>();
#elif defined(ERBSLAND_OS_WINDOWS)
    return std::make_shared<WindowsSecureStorageBackend>();
#else
    return std::make_shared<UnsupportedSecureStorageBackend>();
#endif
}

}
