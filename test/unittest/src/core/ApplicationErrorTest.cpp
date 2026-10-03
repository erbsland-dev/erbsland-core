// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include "../i18n/DisplayTextTestLayer.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/DisplayText.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Application ApplicationError ApplicationErrorContext)
class ApplicationErrorTest final : public el::UnitTest {
public:
    void testReasonConstructorPreservesDiagnosticTitle() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto error = el::core::ApplicationError{"Startup failed"_el};
        REQUIRE_EQUAL(error.title(), "Startup failed"_el);
        const auto text = error.diagnostic()->toTextDocument().toString();
        REQUIRE(text.contains("Startup failed"_el));
    }

    void testContextRendersAllDetails() {
        auto applicationScope = ApplicationTestScope<>{};
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
        {
            auto layer = DisplayTextTestLayer{
                {""_el, "unknown_error"_el, "Unbekannter Fehler"_el},
            };
            REQUIRE_EQUAL(el::i18n::displayText("unknown_error"_el), "Unbekannter Fehler"_el);
        }
        REQUIRE_EQUAL(el::i18n::displayText("unknown_error"_el), "Unknown Error"_el);
    }

    void testEmptyTitleUsesProvidedDisplayText() {
        auto applicationScope = ApplicationTestScope<>{};
        auto map = DisplayTextTestLayer{
            {""_el, "unknown_error"_el, "Unspecified"_el},
        };
        const auto error = el::core::ApplicationError{el::core::ApplicationErrorContext{}, {}};
        const auto text = error.diagnostic()->toTextDocument().toString();
        REQUIRE(text.contains("Unspecified"_el));
    }
};
