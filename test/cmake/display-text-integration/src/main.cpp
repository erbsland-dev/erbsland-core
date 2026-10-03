// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <Empty.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/DisplayText.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionManager.hpp>
#include <Extra.hpp>
#include <Labels.hpp>

using namespace el::text::literals;

auto main() -> int {
    auto app = el::core::Application{};
    auto manager = el::options::OptionManager{};
    const auto registry = app.displayText();
    if (registry->text("options.usage_label"_el) != "Usage"_el) {
        return 1;
    }
    const auto empty = Empty::create();
    if (!empty->domains().empty()) {
        return 4;
    }
    registry->addData(empty);
    const auto words = sample::wording::Labels::create();
    registry->addData(words);
    registry->addData(Extra::create());
    if (el::displayText("application_text"_el) != "catalog ready"_el ||
        el::displayText("extra_text"_el) != "extra"_el ||
        el::displayText("sample.deep.unicode_label"_el) != "Grüße 😀"_el ||
        el::displayText("sample.deep.multiline"_el) != "one\ntwo"_el ||
        el::displayFormat("sample.deep.pattern"_el).build(17) != "Value: 17"_el ||
        !manager.helpDocument({}).toString().contains("Custom Usage"_el)) {
        return 2;
    }
    if (!registry->removeData(words) || el::displayText("options.usage_label"_el) != "Usage"_el) {
        return 3;
    }
    return 0;
}
