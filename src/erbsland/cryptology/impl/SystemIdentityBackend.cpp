#include "SystemIdentityBackend.hpp"

#include "../../core/Definitions.hpp"

#if defined(ERBSLAND_OS_MACOS)
#include "MacosSystemIdentityBackend.hpp"
#elif defined(ERBSLAND_OS_WINDOWS)
#include "WindowsSystemIdentityBackend.hpp"
#else
#include "UnsupportedSystemIdentityBackend.hpp"
#endif

namespace erbsland::cryptology::impl {

auto SystemIdentityBackend::create(const bool allowInteraction) -> SystemIdentityBackendPtr {
#if defined(ERBSLAND_OS_MACOS)
    return std::make_shared<MacosSystemIdentityBackend>(allowInteraction);
#elif defined(ERBSLAND_OS_WINDOWS)
    return std::make_shared<WindowsSystemIdentityBackend>(allowInteraction);
#else
    static_cast<void>(allowInteraction);
    return std::make_shared<UnsupportedSystemIdentityBackend>();
#endif
}

}
