// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ServerPart.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathResolveMode.hpp>
#include <erbsland/PathType.hpp>
#include <erbsland/PathWalkOptions.hpp>
#include <erbsland/PathWalkStatus.hpp>
#include <erbsland/Port.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/SymlinkMode.hpp>

#include <cstddef>

namespace demo {

void ServerPart::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->addOption("root"_el).setRequired().setHelp("Directory to serve on the local machine."_el);
    options->addOption({"--port"_el, "port"_el})
        .setType(el::OptionType::Text)
        .setDefaultValue("17680"_el)
        .setValidateTextValue<el::Port>("Invalid port."_el)
        .setHelp("Local HTTP port; zero selects an available port."_el);
    options->addOption({"--timeout"_el, "timeout"_el})
        .setType(el::OptionType::Integer)
        .setDefaultValue(0)
        .setHelp("Stop after this many seconds; zero runs until interrupted."_el);
}

void ServerPart::parseCommandLine(const el::OptionValuesPtr &values) {
    _root = el::Path::fromNativeOrThrow(values->getText("root"_el)).resolveOrThrow(el::PathResolveMode::Physical);
    if (!_root.info().isDirectory()) {
        throw el::ApplicationError{"The root must name a directory."_el};
    }
    _port = el::Port::fromStringOrThrow(values->getText("port"_el));
    _timeout = el::Seconds{values->getInteger("timeout"_el)};
    if (_timeout.isNegative()) {
        throw el::ApplicationError{"The timeout must be non-negative."_el};
    }
}

auto ServerPart::createIndex() const -> el::String {
    auto html = el::StringEditor{"<!doctype html><html><meta charset=\"utf-8\"><title>Files</title>"_el};
    html.append("<h1>Files</h1><ul>"_el);
    auto options = el::PathWalkOptions{};
    options.setTypes(el::PathType::RegularFile).setSymlinkMode(el::SymlinkMode::Skip);
    auto count = std::size_t{0};
    _root.walker().walkOrThrow(
        [&html, &count, this](const el::Path &path) -> el::PathWalkStatus {
            const auto name = path.toRelativeOrThrow(_root).toPosix();
            // Escape the decoded path for use in a URL, then escape the link for HTML below.
            const auto url = el::StringFormat{"/files/{:text:esc=url_path}"_el}.build(name);
            html.append(
                el::StringFormat{"<li><a href=\"{:text:esc=html}\">{:text:esc=html}</a></li>"_el}.build(url, name));
            return ++count >= 1000U ? el::PathWalkStatus::Stop : el::PathWalkStatus::Continue;
        },
        options);
    html.append("</ul><p>Listing up to 1000 files.</p></html>"_el);
    return html;
}

}
