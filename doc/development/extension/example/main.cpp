// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <tool/cleanup/CodeCleanupTool.hpp>
#include <tool/doc/BuildDocTool.hpp>
#include <tool/doc/PreprocessSourcesTool.hpp>
#include <tool/frame/ToolApplication.hpp>
#include <tool/headers/PublicHeadersTool.hpp>
#include <tool/pre_commit/PreCommitTool.hpp>

/// Run the maintenance commands for the sample extension.
auto main(const int argc, char *argv[]) -> int {
    using namespace el::text::literals;
    auto app = tool::frame::ToolApplication{
        argc,
        argv,
        {"Erbsland Sample"_el,
            "erbsland-core-sample"_el,
            "erbsland-sample-tool"_el,
            "tool/conf/erbsland-sample-tool.elcl"_el}};
    app.registerPreCommitTask<tool::headers::PublicHeadersTool>();
    app.registerPreCommitTask<tool::cleanup::CodeCleanupTool>();
    app.registerTool<tool::pre_commit::PreCommitTool>();
    app.registerTool<tool::doc::PreprocessSourcesTool>();
    app.registerTool<tool::doc::BuildDocTool>();
    return app.run();
}
