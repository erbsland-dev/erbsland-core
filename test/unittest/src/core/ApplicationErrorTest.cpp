// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/core/ApplicationError.hpp>
#include <erbsland/i18n/DisplayTextMap.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Application ApplicationError ApplicationErrorContext)
class ApplicationErrorTest final : public el::UnitTest {
public:
    void testReasonConstructorPreservesDiagnosticTitle() {
        const auto error = el::core::ApplicationError{"Startup failed"_el};
        REQUIRE_EQUAL(error.title(), "Startup failed"_el);
        const auto text = error.diagnostic()->toTextDocument().toString();
        REQUIRE(text.contains("Startup failed"_el));
    }

    void testContextRendersAllDetails() {
        auto context = el::core::ApplicationErrorContext{"Invalid configuration"_el, "Correct the marked value."_el};
        context.setSourceName("application settings"_el)
            .setSourcePath("/tmp/app.conf"_el)
            .setCodeLocation({el::unit::LineIndex{4U}, el::unit::ColumnIndex{6U}});
        const auto error = el::core::ApplicationError{context, {}};
        const auto text = error.diagnostic()->toTextDocument().toString();

        REQUIRE(text.contains("Invalid configuration"_el));
        REQUIRE(text.contains("Correct the marked value."_el));
        REQUIRE(text.contains("Source:"_el));
        REQUIRE(text.contains("application settings"_el));
        REQUIRE(text.contains("Path:"_el));
        REQUIRE(text.contains("/tmp/app.conf"_el));
        REQUIRE(text.contains("Line:"_el));
        REQUIRE(text.contains("Column:"_el));
    }

    void testCustomAndResetApplicationDisplayText() {
        auto scope = ApplicationTestScope<>{};
        REQUIRE(scope.app().displayText());
        auto map = scope.app().displayText()->clone();
        map->set("UnknownError"_el, "Unbekannter Fehler"_el);
        scope.app().setDisplayTextMap(map);

        const auto localizedText = scope.app().displayText()->text("UnknownError"_el);
        REQUIRE_EQUAL(localizedText, "Unbekannter Fehler"_el);
        scope.app().setDisplayTextMap(nullptr);
        REQUIRE(scope.app().displayText());
        const auto defaultText = scope.app().displayText()->text("UnknownError"_el);
        REQUIRE_EQUAL(defaultText, "Unknown Error"_el);
    }

    void testEmptyTitleUsesProvidedDisplayText() {
        auto map = el::i18n::DisplayTextMap::defaultMap()->clone();
        map->set("UnknownError"_el, "Unspecified"_el);
        const auto error = el::core::ApplicationError{el::core::ApplicationErrorContext{}, {}};
        const auto text = error.diagnostic()->toTextDocument(map).toString();
        REQUIRE(text.contains("Unspecified"_el));
    }
};
