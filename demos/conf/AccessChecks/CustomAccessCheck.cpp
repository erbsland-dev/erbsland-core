// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/AccessCheck.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/path/all.hpp>

#include <exception>

namespace demo {

/// Restrict parsing to an exact list of approved configuration files.
///
/// A fully custom `AccessCheck` is useful when an application already has a complete source policy. The parser calls
/// `check()` for the root and every included source before opening it. This implementation canonicalizes file paths and
/// grants access only when the requested path is present in its allowlist.
class ApprovedStationSources final : public el::conf::AccessCheck {
public:
    explicit ApprovedStationSources(const el::PathList &approvedPaths) {
        for (const auto &path : approvedPaths) {
            _approvedPaths += path.resolveOrThrow(el::PathResolveMode::Physical);
        }
    }

public: // implement `AccessCheck`
    auto check(const el::conf::AccessSources &sources) -> el::conf::AccessCheckResult override {
        if (sources.source == nullptr || sources.root == nullptr || sources.source->name() != "file"_el) {
            return el::conf::AccessCheckResult::Denied;
        }
        auto requestedPath = el::Path{sources.source->path()};
        try {
            requestedPath = requestedPath.resolveOrThrow(el::PathResolveMode::Physical);
        } catch (const el::PathError &) {
            throw el::conf::ConfError{
                el::conf::ConfErrorCategory::Access,
                "Configuration Source Access Denied"_el,
                "The configuration source path cannot be resolved."_el,
                requestedPath,
                std::current_exception()};
        }
        for (const auto &approvedPath : _approvedPaths) {
            if (requestedPath == approvedPath) {
                return el::conf::AccessCheckResult::Granted;
            }
        }
        throw el::conf::ConfError{
            el::conf::ConfErrorCategory::Access,
            "The configuration source is not in the station allowlist."_el,
            requestedPath};
    }

private:
    el::PathList _approvedPaths;
};

/// Install a custom access check that implements an application-owned trust boundary.
void customAccessCheck() {
    auto temporaryOptions = el::PathTempDirectoryOptions{};
    temporaryOptions.setPrefix("stacja-badawcza-"_el).setRandomLength(el::CpLength{8U});
    const auto temporary =
        el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
    const auto root = temporary->path();
    const auto stationPath = root / "station.elcl"_el;
    stationPath.content().writeTextOrThrow(
        "[station]\n"
        "name: \"Obserwatorium Północne\"\n"_el);
    const auto mainPath = root / "main.elcl"_el;
    mainPath.content().writeTextOrThrow("@include: \"station.elcl\"\n"_el);

    const auto approvedPaths = el::PathList{mainPath, stationPath};

    auto parser = el::conf::Parser{};
    parser.setAccessCheck(std::make_shared<ApprovedStationSources>(approvedPaths));
    const auto document = parser.parseFileOrThrow(mainPath);
    el::io::printLine("Approved station: "_el, document->getTextOrThrow("station.name"_el));
}

}
