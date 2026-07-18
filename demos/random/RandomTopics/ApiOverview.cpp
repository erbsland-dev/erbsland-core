// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// The random module offers generators with different intent.
///
/// Use the shared application generator for ordinary non-security choices,
/// switch to `SecureRandom` for values that protect access or identity, and
/// reserve explicitly seeded `FastRandom` instances for reproducible tests and
/// simulations.
auto buildMapRows(el::Random &random) -> el::StringList {
    const auto terrain = el::StringList{"les"_el, "skala"_el, "voda"_el, "louka"_el};
    auto rows = el::StringList{};

    for (auto y = 0; y < 3; ++y) {
        auto row = el::StringEditor{};
        for (auto x = 0; x < 4; ++x) {
            if (x > 0) {
                row.append(" "_el);
            }
            row.append(random.selectElement(terrain));
        }
        rows.append(el::String{row});
    }
    return rows;
}

void apiOverview() {
    // Use application randomness for ordinary non-security choices.
    auto &random = el::application().random();
    const auto mapRows = buildMapRows(random);
    el::io::printLine("Application random map:"_el);
    el::io::printLine(mapRows.join("\n"_el));

    // The same shared generator can be used throughout the application.
    const auto windSpeed = random.getDouble(0.0, 20.0);
    const auto yesNo = el::BooleanFormat::yesNo();
    el::io::printLine("Shared wind sample in range: "_el, yesNo, windSpeed >= 0.0 && windSpeed <= 20.0);

    // Secure randomness is used when a generated value protects something.
    const auto tokenAlphabet = el::CharSet::fromPattern("A-Za-z0-9"_el);
    const auto token = el::application().secureRandom().buildString(el::CpLength{16U}, tokenAlphabet);
    el::io::printLine("Secure token length: "_el, token.characterLength().toSizeT());

    // WARNING: Use explicit seeds only when reproducibility is the purpose.
    // Never use seeded fast randomness for secrets or security decisions.
    auto firstReplay = el::FastRandom{20260607U};
    auto secondReplay = el::FastRandom{20260607U};
    const auto sameReplaySequence = firstReplay.getUInt32(1U, 100U) == secondReplay.getUInt32(1U, 100U);
    el::io::printLine("Seeded replay check: "_el, yesNo, sameReplaySequence);
}

}
