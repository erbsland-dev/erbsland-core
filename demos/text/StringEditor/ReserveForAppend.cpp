// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <array>

namespace demo {

/// Reserve string storage once when the final native size is already known.
///
/// Reserving before every append step can repeatedly materialize new storage.
/// The better pattern is to calculate the final native size, reserve once, and
/// then append the fragments. Unreserved growth may otherwise reallocate and
/// copy the existing text several times.
void reserveForAppend() {
    const auto fragments = std::array{
        "Aurora station: céu limpo"_el,
        "Wind tunnel: brise légère"_el,
        "Observatory: 星が明るい"_el,
    };

    auto requiredBytes = std::size_t{0};
    for (const auto &fragment : fragments) {
        requiredBytes += fragment.length().toSizeT() + 1U;
    }

    auto planned = el::StringEditor{};
    planned.reserve(el::ByteLength::fromSizeT(requiredBytes));

    el::io::printLine("Planned reservation"_el);
    el::io::printLine("  required bytes : "_el, requiredBytes);
    el::io::printLine("  length after reserve : "_el, planned.length());
    el::io::printLine("  capacity after reserve : "_el, planned.capacity());

    for (const auto &fragment : fragments) {
        planned.append(fragment).append(U'\n');
    }

    el::io::printLine("  length after append : "_el, planned.length());
    el::io::printLine("  capacity after append : "_el, planned.capacity());

    auto repeated = el::StringEditor{};
    el::io::printLine();
    el::io::printLine("Repeated exact reservations"_el);
    for (const auto &fragment : fragments) {
        const auto nextLength = repeated.length().toSizeT() + fragment.length().toSizeT() + 1U;
        repeated.reserve(el::ByteLength::fromSizeT(nextLength));
        repeated.append(fragment).append(U'\n');
        el::io::printLine("  after step: length "_el, repeated.length(), ", capacity "_el, repeated.capacity());
    }

    el::io::printLine();
    el::io::printLine("Final report:"_el);
    el::io::print(planned);
}

}
