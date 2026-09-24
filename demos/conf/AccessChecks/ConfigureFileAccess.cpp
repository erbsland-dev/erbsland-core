// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/FileAccessCheck.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/path/all.hpp>

namespace demo {

/// Add an application rule to the configurable file access policy.
///
/// `FileAccessCheck` provides the filesystem boundary, size limit, and optional suffix check. A derived check can call
/// the base implementation first and then enforce a rule that is specific to the application. The configured shared
/// instance is installed with `Parser::setAccessCheck()` before parsing starts.
class StationFileAccessCheck final : public el::conf::FileAccessCheck {
public: // implement `AccessCheck`
    auto check(const el::conf::AccessSources &sources) -> el::conf::AccessCheckResult override {
        const auto result = FileAccessCheck::check(sources);
        if (sources.parent != nullptr && el::Path{sources.source->path()}.name().startsWith("draft-"_el)) {
            throw el::conf::ConfError{
                el::conf::ConfErrorCategory::Access,
                "Draft station data cannot be included in an operational configuration."_el,
                el::Path{sources.source->path()}};
        }
        return result;
    }
};

/// Configure and install a file access check before parsing a document.
void configureFileAccess() {
    auto temporaryOptions = el::PathTempDirectoryOptions{};
    temporaryOptions.setPrefix("stacja-badawcza-"_el).setRandomLength(el::CpLength{8U});
    const auto temporary =
        el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
    const auto root = temporary->path();
    (root / "station.elcl"_el)
        .content()
        .writeTextOrThrow(
            "[station]\n"
            "name: \"Stacja Polarna Aurora\"\n"_el);
    const auto mainPath = root / "main.elcl"_el;
    mainPath.content().writeTextOrThrow("@include: \"station.elcl\"\n"_el);

    // Keep the safe directory and size defaults, and require the usual ELCL suffix as well.
    const auto accessCheck = std::make_shared<StationFileAccessCheck>();
    accessCheck->enable(el::conf::FileAccessCheck::RequireSuffix);

    auto parser = el::conf::Parser{};
    parser.setAccessCheck(accessCheck);
    const auto document = parser.parseFileOrThrow(mainPath);
    el::io::printLine("Station: "_el, document->getTextOrThrow("station.name"_el));
}

}
