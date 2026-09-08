// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CatalogMonitorApplication.hpp"

namespace demo {

using namespace el::text::literals;

/// Tell the service manager about initialization work that may take a while.
///
/// The remaining time must be positive and should be a realistic upper estimate rather than a heartbeat interval.
/// Once `reportStartupPending()` is called, every successful startup path must eventually call
/// `reportStartupComplete()`.
void CatalogMonitorApplication::initialize() {
    info().setApplicationName("Catalog Monitor"_el);
    info().setApplicationVersion(el::Version{1, 0, 0});

    // Publish an initial estimate before loading external configuration.
    reportStartupPending(el::Seconds{30});
    loadConfiguration();

    // Refine the estimate before opening the catalog and starting periodic work.
    reportStartupPending(el::Seconds{10});
    openCatalog();
    _refreshTimer = events()->createTimer([this]() -> void { refreshCatalog(); });
    _refreshTimer->startFixedDelay(el::Seconds{30});

    // Mark the service ready only after it can perform its advertised work.
    reportStartupComplete();
}

void CatalogMonitorApplication::cleanup() noexcept {
    _refreshTimer.reset();
    el::io::printLine("Catalog monitor stopped."_el);
}

void CatalogMonitorApplication::loadConfiguration() {
    el::io::printLine("Configuration loaded."_el);
}

void CatalogMonitorApplication::openCatalog() {
    el::io::printLine("Catalog opened."_el);
}

void CatalogMonitorApplication::refreshCatalog() {
    el::io::printLine("Catalog index refreshed."_el);
}

}
