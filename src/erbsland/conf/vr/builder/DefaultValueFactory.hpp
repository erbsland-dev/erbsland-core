// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../re/RegEx_fwd.hpp"
#include "../../../text/StringList.hpp"
#include "../../../time/CalendarDelta.hpp"
#include "../../../time/Time.hpp"
#include "../../../time/TimeWithZone.hpp"
#include "../../impl/value/Value.hpp"

#include <vector>

namespace erbsland::conf::vr::builder::detail {

[[nodiscard]] auto createDefaultValue(Integer value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(bool value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(Float value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const text::String &value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const time::Date &value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const time::Time &value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const time::TimeWithZone &value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const time::DateTime &value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const mem::ByteBlock &value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const time::CalendarDelta &value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const re::RegExPtr &value) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const std::vector<Integer> &values) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const std::vector<bool> &values) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const std::vector<Float> &values) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const text::StringList &values) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const std::vector<mem::ByteBlock> &values) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const std::vector<std::vector<Integer>> &values) -> impl::ValuePtr;
[[nodiscard]] auto createDefaultValue(const std::vector<std::vector<Float>> &values) -> impl::ValuePtr;

}
