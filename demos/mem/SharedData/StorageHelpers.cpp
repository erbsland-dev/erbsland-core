// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/mem/CowManualStorage.hpp>
#include <erbsland/mem/CowStorage.hpp>

#include <vector>

namespace demo {

/// Choose automatic or explicitly marked writable copy-on-write access.
///
/// `CowStorage` overloads `data()` for reading and writing. `CowManualStorage`
/// keeps `data()` read-only and names its writable path `detachedData()`, which
/// is useful when a containing class must make every mutation easy to audit.
void storageHelpers() {
    using Concepts = std::vector<el::String>;

    auto automatic = el::mem::CowStorage<Concepts>::from({"form"_el, "rytme"_el});
    auto automaticCopy = automatic;

    // A mutable data() call automatically detaches CowStorage.
    automaticCopy.data().emplace_back("balance"_el);

    auto explicitWrite = el::mem::CowManualStorage<Concepts>::from({"linje"_el, "flade"_el});
    auto explicitCopy = explicitWrite;

    // CowManualStorage gives the writable operation a distinct name.
    explicitCopy.detachedData().emplace_back("rum"_el);

    const auto &automaticRead = automatic;
    const auto &automaticCopyRead = automaticCopy;
    el::io::printLine("Automatic original: "_el, automaticRead.data().size());
    el::io::printLine("Automatic copy    : "_el, automaticCopyRead.data().size());
    el::io::printLine("Explicit original : "_el, explicitWrite.data().size());
    el::io::printLine("Explicit copy     : "_el, explicitCopy.data().size());
}

}
