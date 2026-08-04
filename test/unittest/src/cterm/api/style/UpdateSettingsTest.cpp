// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(UpdateSettings)
class UpdateSettingsTest final : public el::UnitTest {
public:
    void testDefaultSettingsExposeTheExpectedDefaults() {
        const auto &settings = UpdateSettings::defaultSettings();
        const auto expectedMinimumSize = bgeo::BlockSize{0, 0};

        REQUIRE_EQUAL(settings.minimumSize(), expectedMinimumSize);
        REQUIRE_EQUAL(settings.minimumSizeBackground(), Block::space());
        REQUIRE(settings.minimumSizeMessage().isEmpty());
        REQUIRE_FALSE(settings.showCropMarks());
        REQUIRE_EQUAL(settings.cropMarkRight(), U'▶');
        REQUIRE_EQUAL(settings.cropMarkBottomRight(), U'◢');
        REQUIRE_EQUAL(settings.cropMarkBottom(), U'▼');
        REQUIRE(settings.switchToAlternateBuffer());
        REQUIRE_EQUAL(&UpdateSettings::defaultSettings(), &settings);
    }

    void testSettersUpdateAllStoredValues() {
        auto settings = UpdateSettings{};
        const auto expectedMinimumSize = bgeo::BlockSize{80, 25};

        settings.setMinimumSize(expectedMinimumSize);
        settings.setMinimumSizeBackground(Block{U'.'});
        settings.setMinimumSizeMessage(BlockStringEditor{"Terminal too small"_el});
        settings.setShowCropMarks(true);
        settings.setCropMarkRight(Block{U'>'});
        settings.setCropMarkBottomRight(Block{U'+'});
        settings.setCropMarkBottom(Block{U'v'});
        settings.setSwitchToAlternateBuffer(false);

        REQUIRE_EQUAL(settings.minimumSize(), expectedMinimumSize);
        REQUIRE_EQUAL(settings.minimumSizeBackground(), U'.');
        const auto minimumSizeMessageLength = settings.minimumSizeMessage().length();
        REQUIRE_EQUAL(minimumSizeMessageLength, BlockCount{18U});
        REQUIRE(settings.showCropMarks());
        REQUIRE_EQUAL(settings.cropMarkRight(), U'>');
        REQUIRE_EQUAL(settings.cropMarkBottomRight(), U'+');
        REQUIRE_EQUAL(settings.cropMarkBottom(), U'v');
        REQUIRE_FALSE(settings.switchToAlternateBuffer());
    }

    void testApplyToTransfersCropMarkConfigurationToAView() {
        auto settings = UpdateSettings{};
        settings.setShowCropMarks(true);
        settings.setCropMarkRight(Block{U'>'});
        settings.setCropMarkBottomRight(Block{U'+'});
        settings.setCropMarkBottom(Block{U'v'});

        auto view = BufferView{};
        settings.applyTo(view);

        REQUIRE(view.showCropCharacters());
        REQUIRE_EQUAL(view.cropCharacter(bgeo::BlockDirection::East), U'>');
        REQUIRE_EQUAL(view.cropCharacter(bgeo::BlockDirection::SouthEast), U'+');
        REQUIRE_EQUAL(view.cropCharacter(bgeo::BlockDirection::South), U'v');
    }
};
