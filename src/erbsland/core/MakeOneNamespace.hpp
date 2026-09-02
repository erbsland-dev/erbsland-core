// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Namespaces.hpp"

#include "../system/MakeOneNamespace.hpp"

namespace erbsland {
// Import all API namespaces into the main one.
using namespace core;
using namespace cryptology;
using namespace debug;
using namespace err;
using namespace event;
using namespace geometry;
using namespace i18n;
using namespace log;
using namespace math;
using namespace mem;
using namespace network;
using namespace options;
using namespace path;
using namespace random;
using namespace resource;
using namespace stream;
namespace io = stream::io;
using namespace text;
namespace html = text::html;
namespace punycode = text::punycode;
using namespace time;
using namespace unit;
using namespace util;
}
