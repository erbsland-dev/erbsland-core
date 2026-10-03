// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/Date.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/time_zone/TimeZoneModel.hpp>
#include <tool/time_zone/TimeZoneRenderer.hpp>

TESTED_TARGETS(TimeZoneSource TimeZoneModel TimeZoneRenderer TimeZoneDataTool)
class TimeZoneDataToolTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testVerifiedSourceTransitionsAndWindowsMapping() {
        prepareSnapshot();
        for (const auto &path : files().collectFilenamesOnly(el::Path{"tz/tzdb"_el}, ""_el)) {
            write(path.toString(), files().readText(path).replacedAll("\n"_el, "\r\n"_el));
        }
        runWithSources(el::StringList{"time_zone"_el}, [&] {
            const auto model =
                tool::time_zone::TimeZoneModel{tool::frame::app().source<tool::time_zone::TimeZoneSource>()};
            REQUIRE_EQUAL(model.versionYear(), 2026);
            REQUIRE_EQUAL(model.versionRevision(), 2);
            REQUIRE_EQUAL(model.zoneFor("Test/Alias"_el).id, 1U);
            REQUIRE_EQUAL(model.windowsMappings().size(), 1U);
            REQUIRE_EQUAL(model.windowsMappings().front().second, "Test/Alias"_el);
            REQUIRE(model.textId("TDT"_el) != 0U);
            const auto &zone = model.zoneFor("Test/City"_el);
            REQUIRE_EQUAL(zone.entries.size(), 2U);
            const auto [boundary, offset] = model.untilBoundary(zone.entries.front());
            const auto date = el::time::Date{el::time::Year{2024}, el::time::Month{3}, el::time::Day{10}};
            REQUIRE_EQUAL(boundary, static_cast<uint64_t>(date.toDaysSinceEpoch().toRawValue()) * 86400U + 21600U);
            REQUIRE_EQUAL(offset, -14400);
            const auto output = tool::time_zone::TimeZoneRenderer{model};
            REQUIRE_EQUAL(output.outputs().size(), 25U);
            REQUIRE(output.outputs().front().second.contains("Test/City"_el) == false);
            auto zonesSource = el::String{};
            for (const auto &[name, content] : output.outputs()) {
                if (name == "Zones.cpp"_el) {
                    zonesSource = content;
                }
            }
            REQUIRE(zonesSource.contains(
                "#include \"RegionAfrica.hpp\"\n"
                "#include \"RegionAntarctica.hpp\"\n"
                "#include \"RegionAsia.hpp\"\n"
                "#include \"RegionAustralasia.hpp\"\n"
                "#include \"RegionEtcetera.hpp\"\n"
                "#include \"RegionEurope.hpp\"\n"
                "#include \"RegionNorthamerica.hpp\"\n"
                "#include \"RegionSouthamerica.hpp\"\n"_el));
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testHashAndVersionFailures() {
        prepareSnapshot();
        write("tz/tzdb/africa"_el, "tampered\n"_el);
        REQUIRE(runSourceProbe() != 0);
        prepareSnapshot("2027a"_el);
        write("tz/manifest.elcl"_el, "[dataset]\ntzdb_version: \"2026b\"\ncldr_version: \"48\"\n[files]\n"_el);
        REQUIRE(runSourceProbe() != 0);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testChangedAndMalformedRecords() {
        prepareSnapshot(
            "2026b"_el,
            "Rule X 2020 max - Mar Sun>=8 2:00 2:00 D\n"
            "Rule X 2020 max - Nov Sun>=1 2:00 0 S\n"
            "Zone Test/City -5:00 X T%sT 2024 Mar 10 2:00\n"
            "    -4:00 - TST\n"_el);
        runWithSources(el::StringList{"time_zone"_el}, [&] {
            const auto model =
                tool::time_zone::TimeZoneModel{tool::frame::app().source<tool::time_zone::TimeZoneSource>()};
            REQUIRE_EQUAL(model.untilBoundary(model.zoneFor("Test/City"_el).entries.front()).second, -10800);
        });
        prepareSnapshot("2026b"_el, "Zone Test/City -5:00 Missing T%sT\n"_el);
        runWithSources(el::StringList{"time_zone"_el}, [&] {
            REQUIRE_THROWS(
                tool::time_zone::TimeZoneModel{tool::frame::app().source<tool::time_zone::TimeZoneSource>()});
        });
        prepareSnapshot("2026b"_el, "Zone Test/VeryLongCityPart -5:00 - TST\n"_el);
        runWithSources(el::StringList{"time_zone"_el}, [&] {
            REQUIRE_THROWS(
                tool::time_zone::TimeZoneModel{tool::frame::app().source<tool::time_zone::TimeZoneSource>()});
        });
    }

private:
    void prepareSnapshot(
        const el::String &version = "2026b"_el,
        const el::String &africa = "Rule X 2020 max - Mar Sun>=8 2:00 1:00 D\n"
                                   "Rule X 2020 max - Nov Sun>=1 2:00 0 S\n"
                                   "Zone Test/City -5:00 X T%sT 2024 Mar 10 2:00\n"
                                   "    -4:00 - TST\n"_el) {
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[time_zone]\ninput_directory: \"tz\"\n"_el);
        auto files = std::vector<std::pair<el::String, el::String>>{
            {"tzdb/version"_el, el::join(version, "\n"_el)},
            {"tzdb/africa"_el, africa},
            {"tzdb/antarctica"_el, {}},
            {"tzdb/asia"_el, {}},
            {"tzdb/australasia"_el, {}},
            {"tzdb/europe"_el, {}},
            {"tzdb/northamerica"_el, {}},
            {"tzdb/southamerica"_el, {}},
            {"tzdb/etcetera"_el, {}},
            {"tzdb/backward"_el, "Link Test/City Test/Alias\n"_el},
            {"windowsZones.xml"_el,
                "<supplementalData><mapZone other=\"Test Standard Time\" territory=\"001\" "
                "type=\"Test/Alias\"/></supplementalData>"_el}};
        auto manifest = el::StringEditor{
            el::StringFormat{"[dataset]\ntzdb_version: \"{}\"\ncldr_version: \"48\"\n[files]\n"_el}.build(version)};
        for (const auto &[name, body] : files) {
            write(el::join("tz/"_el, name), body);
            auto hash = el::Hasher{el::HashAlgorithm::Sha3_256};
            hash.update(body);
            const auto digest =
                el::text::base_n::BaseNEncoder{hash.finalize(), el::text::base_n::BaseNFormat::base16()}.toString();
            manifest.append(el::StringFormat{"\"{}\": \"{}\"\n"_el}.build(name, digest));
        }
        write("tz/manifest.elcl"_el, manifest);
    }

    [[nodiscard]] auto runSourceProbe() -> int {
        const auto previous = SourceTestTool::identifiers;
        SourceTestTool::identifiers = el::StringList{"time_zone"_el};
        const auto result = runTool<SourceTestTool>();
        SourceTestTool::identifiers = previous;
        return result;
    }
};
