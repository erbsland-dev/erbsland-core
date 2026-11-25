// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <tool/anti_patterns/AntiPatternsTool.hpp>
#include <tool/cleanup/CodeCleanupTool.hpp>
#include <tool/cterm/BoxFrameSource.hpp>
#include <tool/cterm/CommonBoxFrameStyleTool.hpp>
#include <tool/cterm/FrameBorderJointCharsTool.hpp>
#include <tool/doc/ApiGuidelinesTool.hpp>
#include <tool/doc/ApiTestStatusTool.hpp>
#include <tool/doc/BuildDocTool.hpp>
#include <tool/doc/CreateDemoTool.hpp>
#include <tool/doc/PreprocessSourcesTool.hpp>
#include <tool/doc/ProcessRstTool.hpp>
#include <tool/doc/UpdateReferenceTool.hpp>
#include <tool/elcl_format/ElclFormatTool.hpp>
#include <tool/frame/ToolApplication.hpp>
#include <tool/github_workflows/GitHubWorkflowsTool.hpp>
#include <tool/headers/PublicHeadersTool.hpp>
#include <tool/i18n/DisplayTextsTool.hpp>
#include <tool/idna/IdnaTool.hpp>
#include <tool/include_dependencies/IncludeDependenciesTool.hpp>
#include <tool/pre_commit/PreCommitTool.hpp>
#include <tool/psl/PublicSuffixSource.hpp>
#include <tool/psl/PublicSuffixTool.hpp>
#include <tool/security_hashes/SecurityHashesTool.hpp>
#include <tool/test_data/CategoryTestDataTool.hpp>
#include <tool/test_data/CryptologyVectorsTool.hpp>
#include <tool/test_data/SaturatingMathTool.hpp>
#include <tool/test_data/TimeTestsTool.hpp>
#include <tool/time_zone/TimeZoneDataTool.hpp>
#include <tool/time_zone/TimeZoneSource.hpp>
#include <tool/unicode/UnicodeDataTool.hpp>
#include <tool/unicode/UnicodeSource.hpp>

/// Main method.
/// Registers built-in commands and runs the Core application lifecycle.
auto main(const int argc, char *argv[]) -> int {
    using namespace el::text::literals;
    auto app = tool::frame::ToolApplication{
        argc,
        argv,
        {"Erbsland Core"_el, "erbsland-core"_el, "erbsland-core-tool"_el, "tool/conf/erbsland-core-tool.elcl"_el}};
    app.registerSource<tool::unicode::UnicodeSource>();
    app.registerSource<tool::cterm::BoxFrameSource>();
    app.registerSource<tool::psl::PublicSuffixSource>();
    app.registerSource<tool::time_zone::TimeZoneSource>();
    app.registerTool<tool::elcl_format::ElclFormatTool>();
    app.registerTool<tool::cterm::CommonBoxFrameStyleTool>();
    app.registerTool<tool::cterm::FrameBorderJointCharsTool>();
    app.registerTool<tool::doc::PreprocessSourcesTool>();
    app.registerTool<tool::doc::ProcessRstTool>();
    app.registerTool<tool::doc::CreateDemoTool>();
    app.registerTool<tool::doc::BuildDocTool>();
    app.registerTool<tool::github_workflows::GitHubWorkflowsTool>();
    app.registerTool<tool::idna::IdnaTool>();
    app.registerTool<tool::include_dependencies::IncludeDependenciesTool>();
    app.registerTool<tool::i18n::DisplayTextsTool>();
    app.registerTool<tool::psl::PublicSuffixTool>();
    app.registerTool<tool::time_zone::TimeZoneDataTool>();
    app.registerTool<tool::unicode::UnicodeDataTool>();
    app.registerTool<tool::test_data::SaturatingMathTool>();
    app.registerTool<tool::test_data::TimeTestsTool>();
    app.registerTool<tool::test_data::CategoryTestDataTool>();
    app.registerTool<tool::test_data::CryptologyVectorsTool>();
    app.registerPreCommitTask<tool::headers::PublicHeadersTool>();
    app.registerPreCommitTask<tool::cleanup::CodeCleanupTool>();
    app.registerPreCommitTask<tool::anti_patterns::AntiPatternsTool>();
    app.registerPreCommitTask<tool::doc::ApiGuidelinesTool>();
    app.registerPreCommitTask<tool::doc::ApiTestStatusTool>();
    app.registerPreCommitTask<tool::doc::UpdateReferenceTool>();
    app.registerPreCommitTask<tool::security_hashes::SecurityHashesTool>();
    app.registerTool<tool::pre_commit::PreCommitTool>();
    return app.run();
}
