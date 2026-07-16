#  Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
#  SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import importlib
import sys
from dataclasses import dataclass
from shutil import get_terminal_size
from textwrap import TextWrapper


@dataclass(frozen=True)
class RegisteredUtility:
    display_name: str
    module_path: str
    class_name: str
    help: str | None = None


REGISTERED_UTILITIES = {
    "cleanup": RegisteredUtility(
        "Clean Up",
        "dev.cleanup",
        "CleanupApp",
        help="Automatically clean up development files: sort and validate includes.",
    ),
    "cleanup_rst": RegisteredUtility(
        "Clean Up reStructuredText",
        "dev.cleanup_rst",
        "CleanupRstApp",
        help="Normalize reStructuredText documentation: links, paragraphs and title adornments.",
    ),
    "create_demo": RegisteredUtility(
        "Create Demo",
        "dev.create_demo",
        "CreateDemoApp",
        help="Create or extend an empty documentation demo template.",
    ),
    "demo_doc": RegisteredUtility(
        "Demo Documentation",
        "dev.demo_doc",
        "DemoDocApp",
        help="Validate or synchronize erbsland-demo documentation blocks.",
    ),
    "build_performance": RegisteredUtility(
        "Build Performance",
        "dev.build_performance",
        "BuildPerformanceApp",
        help="Audit source-level build dependencies and collect compiler performance measurements.",
    ),
    "dev_setup": RegisteredUtility(
        "Development Setup",
        "dev.dev_setup",
        "DevSetupApp",
        help="Set up a fresh development worktree: .venv, submodules and the debug CMake build tree.",
    ),
    "update_includes": RegisteredUtility(
        "Update Includes",
        "dev.update_includes",
        "UpdateIncludesApp",
        help="Regenerate public include wrappers and all.hpp files.",
    ),
    "fix_include_paths": RegisteredUtility(
        "Fix Include Paths",
        "dev.fix_include_paths",
        "FixIncludePathsApp",
        help="Fix wrong include paths in C/C++ source files.",
    ),
    "pre_commit": RegisteredUtility(
        "Pre-Commit",
        "dev.pre_commit",
        "PreCommitApp",
        help="Run pre-commit normalization and checks on the codebase.",
    ),
    "reference_doc": RegisteredUtility(
        "Reference Documentation",
        "dev.reference_doc",
        "ReferenceDocApp",
        help="Update reference documentation pages from public API headers.",
    ),
    "test_status": RegisteredUtility(
        "Test Status",
        "dev.test_status",
        "TestStatusApp",
        help="Validate or normalize API test-status documentation markers.",
    ),
    "rebuild_doc": RegisteredUtility(
        "Rebuild Documentation",
        "dev.rebuild_doc",
        "RebuildDocApp",
        help="Safely remove generated documentation output and rebuild the Sphinx documentation.",
    ),
    "security_hashes": RegisteredUtility(
        "Security Hashes",
        "dev.security_hashes",
        "SecurityHashesApp",
        help="Generate or verify security hashes for infrastructure files.",
    ),
    "generate_unicode_light_data": RegisteredUtility(
        "Generate Unicode Light Data",
        "dev.generate_unicode_light_data",
        "GenerateUnicodeLightDataApp",
        help="Generate the compact Unicode Light data tables from local UCD files.",
    ),
    "generate_time_zone_data": RegisteredUtility(
        "Generate Time Zone Data",
        "dev.generate_time_zone_data",
        "GenerateTimeZoneDataApp",
        help="Generate compact time-zone metadata from the vendored IANA tz database.",
    ),
    "generate_common_box_frame_style": RegisteredUtility(
        "Generate Common Box Frame Style",
        "dev.generate_common_box_frame_style",
        "GenerateCommonBoxFrameStyleApp",
        help="Generate cterm common box-frame character combination data.",
    ),
    "generate_frame_border_joint_chars": RegisteredUtility(
        "Generate Frame Border Joint Characters",
        "dev.generate_frame_border_joint_chars",
        "GenerateFrameBorderJointCharsApp",
        help="Generate cterm frame-border joint character lookup data.",
    ),
    "github_workflows": RegisteredUtility(
        "GitHub Workflows",
        "dev.github_workflows",
        "GitHubWorkflowsApp",
        help="Audit GitHub workflow action pins and version annotations.",
    ),
    "generate_saturating_math_data": RegisteredUtility(
        "Generate Saturating Math Data",
        "test.generate_saturating_math_data",
        "GenerateSaturatingMathDataApp",
        help="Regenerate saturating math unit-test data files.",
    ),
    "generate_time_tests": RegisteredUtility(
        "Generate Time Tests",
        "test.generate_time_tests",
        "GenerateTimeTestsApp",
        help="Regenerate generated date/time unit-test source files.",
    ),
}


def show_help() -> None:
    print("Usage: python3 utilities/run.py <utility_name> [<utility_arguments>]")
    print("Available utilities:")
    help_indent = 8
    term_width = get_terminal_size(fallback=(80, 24)).columns
    wrapper = TextWrapper(
        width=term_width - help_indent,
        initial_indent=" " * help_indent,
        subsequent_indent=" " * help_indent,
    )
    for identifier, utility in REGISTERED_UTILITIES.items():
        print(f"- {identifier} - {utility.display_name}")
        if utility.help:
            print(wrapper.fill(utility.help))


def main(argv: list[str] | None = None) -> int:
    if argv is None:
        argv = sys.argv[1:]
    if not argv:
        show_help()
        return 1
    if argv[0] == "-h" or argv[0] == "--help":
        show_help()
        return 0
    if argv[0] not in REGISTERED_UTILITIES:
        print(f"Unknown utility: {argv[0]}", file=sys.stderr)
        show_help()
        return 1
    utility = REGISTERED_UTILITIES[argv[0]]
    module = importlib.import_module(utility.module_path)
    app_class = getattr(module, utility.class_name)
    app = app_class()
    app.program_name = f"utilities/run.py {argv[0]}"
    return app.main(argv[1:])


if __name__ == "__main__":
    raise SystemExit(main())
