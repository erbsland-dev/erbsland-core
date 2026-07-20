// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>

using namespace el::conf;

// Testing all requirements from chapter "Validation Rules"->"Reserved Names".
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrReservedNamesTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testUnknownReservedNameRejected() {
        // All names with the "vr_" prefix are reserved and must be known.
        WITH_CONTEXT(requireRulesFailLines({
            "[settings.vr_headset]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requireError("Unknown reserved name"));
    }

    void testEscapedReservedNameIsRegular() {
        // Escaped reserved names define rules for regular identifiers.
        WITH_CONTEXT(requireRulesPassLines({
            "[settings.vr_vr_headset]",
            "type: \"text\"",
        }));
        WITH_CONTEXT(requirePassLines({
            "[settings]",
            "vr_headset: \"device\"",
        }));
    }
};
