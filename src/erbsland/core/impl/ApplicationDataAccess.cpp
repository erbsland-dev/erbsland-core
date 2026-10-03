#include "ApplicationDataAccess.hpp"

#include "ApplicationInstanceManager.hpp"

namespace erbsland::core::impl {

auto applicationData() -> ApplicationDataPtr {
    return ApplicationInstanceManager::instance()->retainData();
}

}
