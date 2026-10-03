// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <tool/doc/impl/DemoCapture.hpp>

TESTED_TARGETS(DemoCapture)
class DocumentationCaptureTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testRejectsMissingExecutableLight() {
        REQUIRE_THROWS(tool::doc::impl::DemoCapture::create()->run(_root / "missing"_el, {}, _root, 0));
    }
};
