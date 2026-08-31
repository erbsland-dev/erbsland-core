# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.demo_doc import DemoDocApp, DemoDocError, DemoDocSynchronizer, DemoExecutor
from lib.error import UtilityError


class DemoDocTest(unittest.TestCase):
    """Tests for demo documentation synchronization."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_dir = Path(self.temp_dir.name)
        self.demo_source_dir = self.project_dir / "demos" / "text"
        self.demo_source_dir.mkdir(parents=True)
        self.source_path = self.demo_source_dir / "Sample.cpp"
        self.source_path.write_text(
            "#include <DemoCommon.hpp>\n"
            "\n"
            "namespace demo {\n"
            "\n"
            "void helper();\n"
            "\n"
            "/// Demo documentation starts here.\n"
            "void sampleDemo() {\n"
            "    helper();\n"
            "}\n"
            "\n"
            "}\n"
            "\n"
            "auto main() -> int {\n"
            "    return demo::sampleDemoMain();\n"
            "}\n"
            "\n",
            encoding="utf-8",
        )
        self.fixture_path = self.demo_source_dir / "settings.elcl"
        self.fixture_path.write_text("Value: original\n", encoding="utf-8")
        self.demo_output_dir = self.project_dir / "cmake-build-debug" / "demo-apps"
        self.demo_output_dir.mkdir(parents=True)
        self.executable_path = self.demo_output_dir / "demoapp"
        self.executable_path.write_text("#!/bin/sh\nprintf 'demo output\\n'\n", encoding="utf-8")
        self.executable_path.chmod(0o755)
        self.echo_args_path = self.demo_output_dir / "echoargs"
        self.echo_args_path.write_text("#!/bin/sh\nprintf 'args: %s\\n' \"$*\"\n", encoding="utf-8")
        self.echo_args_path.chmod(0o755)
        self.failing_path = self.demo_output_dir / "failapp"
        self.failing_path.write_text("#!/bin/sh\nprintf 'expected failure\\n'\nexit 1\n", encoding="utf-8")
        self.failing_path.chmod(0o755)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def synchronizer(self, *, force: bool = False) -> DemoDocSynchronizer:
        """Create a synchronizer for the temp project."""
        return DemoDocSynchronizer(self.project_dir, force=force)

    def current_hash(self) -> str:
        """Get the current test source hash."""
        return self.synchronizer().source_hash("text/Sample.cpp")

    def test_zero_blocks_are_unchanged(self) -> None:
        text = "Plain document.\n"

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual(text, updated_text)
        self.assertEqual((), issues)

    def test_sync_generates_source_and_output_for_missing_hash(self) -> None:
        text = (
            "Before.\n"
            "\n"
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "    :exec: demoapp --demo Sample\n"
            "\n"
            "    stale\n"
            "\n"
            ".. erbsland-demo-end::\n"
            "\n"
            "After.\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual((), issues)
        self.assertIn(f"    :source-sha256: {self.current_hash()}", updated_text)
        self.assertIn("    /// Demo documentation starts here.\n", updated_text)
        self.assertNotIn("#include <DemoCommon.hpp>", updated_text)
        self.assertNotIn("namespace demo", updated_text)
        self.assertNotIn("    }\n\n    }\n", updated_text)
        self.assertNotIn("sampleDemoMain", updated_text)
        self.assertIn(".. erbsland-ansi::\n", updated_text)
        self.assertIn("    demo output\n", updated_text)
        self.assertNotIn(".. rubric::", updated_text)

    def test_multiple_execs_generate_command_rubrics(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "    :exec: echoargs --demo First\n"
            "    :exec-2: echoargs --demo Second\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual((), issues)
        self.assertIn("    :exec: echoargs --demo First\n", updated_text)
        self.assertIn("    :exec-2: echoargs --demo Second\n", updated_text)
        self.assertIn(".. rubric:: ``$ echoargs --demo First``\n", updated_text)
        self.assertIn(".. rubric:: ``$ echoargs --demo Second``\n", updated_text)
        self.assertEqual(2, updated_text.count(".. erbsland-ansi::"))
        self.assertIn("    args: --demo First\n", updated_text)
        self.assertIn("    args: --demo Second\n", updated_text)

    def test_single_exec_with_show_cmd_line_generates_rubric(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "    :exec: echoargs --demo Single\n"
            "    :show-cmd-line:\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual((), issues)
        self.assertIn("    :show-cmd-line:\n", updated_text)
        self.assertIn(".. rubric:: ``$ echoargs --demo Single``\n", updated_text)
        self.assertEqual(1, updated_text.count(".. erbsland-ansi::"))

    def test_expected_nonzero_exit_code_is_captured(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "    :exec: failapp\n"
            "    :exec-exit-code: 1\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual((), issues)
        self.assertIn("    :exec-exit-code: 1\n", updated_text)
        self.assertIn("    expected failure\n", updated_text)
        self.assertNotIn(".. note::", updated_text)

    def test_unexpected_exit_code_reports_issue(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "    :exec: failapp\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual(1, len(issues))
        self.assertIn("Demo command failed with exit code 1", issues[0])
        self.assertIn(".. note::\n", updated_text)

    def test_unexpected_success_for_expected_nonzero_exit_reports_issue(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "    :exec: demoapp\n"
            "    :exec-exit-code: 1\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual(1, len(issues))
        self.assertIn("Demo command exited with 0, expected 1", issues[0])
        self.assertIn(".. note::\n", updated_text)

    def test_matching_hash_leaves_block_unchanged_without_force(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            f"    :source-sha256: {self.current_hash()}\n"
            "\n"
            "    intentionally stale body\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual(text, updated_text)
        self.assertEqual((), issues)

    def test_force_regenerates_matching_hash(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            f"    :source-sha256: {self.current_hash()}\n"
            "\n"
            "    intentionally stale body\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer(force=True).process_text(self.project_dir / "doc.rst", text)

        self.assertEqual((), issues)
        self.assertIn(".. code-block:: cpp\n", updated_text)
        self.assertNotIn("intentionally stale body", updated_text)

    def test_function_blocks_select_and_dedent_named_functions(self) -> None:
        function_source_path = self.demo_source_dir / "Functions.cpp"
        function_source_path.write_text(
            "namespace demo {\n"
            "\n"
            "    void first() {\n"
            "        runFirst();\n"
            "    }\n"
            "\n"
            "    auto second() -> void {\n"
            "        if (isEnabled()) {\n"
            "            runSecond();\n"
            "        }\n"
            "    }\n"
            "\n"
            "}\n",
            encoding="utf-8",
        )
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Functions.cpp\n"
            "    :function-blocks: second first\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual((), issues)
        self.assertIn("    :function-blocks: second first\n", updated_text)
        self.assertIn("    :function-blocks-sha256: ", updated_text)
        self.assertIn("    auto second() -> void {\n", updated_text)
        self.assertIn("        runSecond();\n", updated_text)
        self.assertIn("    void first() {\n", updated_text)
        self.assertNotIn("        auto second", updated_text)
        self.assertLess(updated_text.index("auto second"), updated_text.index("void first"))

        changed_text = updated_text.replace(":function-blocks: second first", ":function-blocks: first")
        regenerated_text, regenerated_issues = self.synchronizer().process_text(
            self.project_dir / "doc.rst", changed_text
        )

        self.assertEqual((), regenerated_issues)
        self.assertIn("    :function-blocks: first\n", regenerated_text)
        self.assertNotIn("auto second", regenerated_text)

    def test_unknown_function_block_reports_issue(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "    :function-blocks: missing\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual(1, len(issues))
        self.assertIn("Function block not found: missing.", issues[0])
        self.assertIn(".. note::\n", updated_text)

    def test_multiple_blocks_are_processed(self) -> None:
        text = (
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "\n"
            ".. erbsland-demo-end::\n"
            "\n"
            ".. erbsland-demo::\n"
            "    :source: text/Sample.cpp\n"
            "\n"
            ".. erbsland-demo-end::\n"
        )

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual((), issues)
        self.assertEqual(2, updated_text.count(".. code-block:: cpp"))

    def test_missing_documentation_comment_writes_note_and_issue(self) -> None:
        no_comment_path = self.demo_source_dir / "NoComment.cpp"
        no_comment_path.write_text("void noComment() {}\n", encoding="utf-8")
        text = ".. erbsland-demo::\n" "    :source: text/NoComment.cpp\n" "\n" ".. erbsland-demo-end::\n"

        updated_text, issues = self.synchronizer().process_text(self.project_dir / "doc.rst", text)

        self.assertEqual(1, len(issues))
        self.assertIn("has no /// documentation comment", issues[0])
        self.assertIn(".. note::\n", updated_text)

    def test_malformed_blocks_fail_clearly(self) -> None:
        with self.assertRaises(UtilityError):
            self.synchronizer().process_text(self.project_dir / "doc.rst", ".. erbsland-demo::\n")

        with self.assertRaises(UtilityError):
            self.synchronizer().process_text(self.project_dir / "doc.rst", ".. erbsland-demo-end::\n")

    def test_exec_validation_accepts_strict_demo_command(self) -> None:
        executor = DemoExecutor(self.project_dir)

        self.assertEqual(["demoapp", "--demo", "Sample"], executor.validate_command("demoapp --demo Sample"))

    def test_exec_validation_accepts_domain_specific_demo_command(self) -> None:
        executor = DemoExecutor(self.project_dir)

        self.assertEqual(["text/demoapp", "--demo", "Sample"], executor.validate_command("text/demoapp --demo Sample"))

    def test_exec_validation_accepts_demo_fixture_and_dotted_branch(self) -> None:
        executor = DemoExecutor(self.project_dir)

        self.assertEqual(
            ["demoapp", "demos/text/settings.elcl", "--branch", "application.logging"],
            executor.validate_command("demoapp demos/text/settings.elcl --branch application.logging"),
        )

    def test_executable_path_resolves_domain_specific_demo(self) -> None:
        domain_executable_path = self.demo_output_dir / "text" / "demoapp"
        domain_executable_path.parent.mkdir()
        domain_executable_path.write_text("#!/bin/sh\nprintf 'demo output\\n'\n", encoding="utf-8")
        domain_executable_path.chmod(0o755)

        self.assertEqual(domain_executable_path, DemoExecutor(self.project_dir).executable_path("text/demoapp"))

    def test_refresh_mode_forces_recursive_synchronization(self) -> None:
        app = DemoDocApp()

        app.parse_command_line(["refresh", "doc/topics"])

        self.assertEqual("sync", app.mode)
        self.assertTrue(app.force)
        self.assertTrue(app.recursive)

    def test_exec_validation_accepts_placeholders(self) -> None:
        executor = DemoExecutor(self.project_dir)

        self.assertEqual(
            ["demoapp", "{file:text}", "{file:empty}", "{file:none}", "{directory:cache}"],
            executor.validate_command("demoapp {file:text} {file:empty} {file:none} {directory:cache}"),
        )

    def test_exec_validation_rejects_suspicious_commands(self) -> None:
        executor = DemoExecutor(self.project_dir)

        for command in (
            "../demoapp",
            "text/../demoapp",
            "/text/demoapp",
            "demo.app",
            "demoapp --demo Sample.cpp",
            "demoapp demos/text/missing.elcl",
            "demoapp demos/../settings.elcl",
            "demoapp 'bad value'",
            "demoapp --file={file:text}",
            "demoapp {file:binary}",
            "demoapp {directory:../cache}",
        ):
            with self.subTest(command=command):
                with self.assertRaises(DemoDocError):
                    executor.validate_command(command)

    def test_placeholder_expansion_creates_safe_paths(self) -> None:
        executor = DemoExecutor(self.project_dir)

        with tempfile.TemporaryDirectory(dir="/private/tmp") as temporary_dir:
            expanded = executor.expand_placeholders(
                ["{file:text}", "{file:empty}", "{file:none}", "{directory:cache}", "{directory:cache}"],
                Path(temporary_dir),
            )

            text_path = Path(expanded[0])
            empty_path = Path(expanded[1])
            none_path = Path(expanded[2])
            first_directory_path = Path(expanded[3])
            second_directory_path = Path(expanded[4])
            self.assertTrue(text_path.is_file())
            self.assertEqual(5, len(text_path.read_text(encoding="utf-8").splitlines()))
            self.assertTrue(empty_path.is_file())
            self.assertEqual(0, empty_path.stat().st_size)
            self.assertFalse(none_path.exists())
            self.assertTrue(first_directory_path.is_dir())
            self.assertEqual(first_directory_path, second_directory_path)

    def test_multiple_file_placeholders_create_distinct_paths(self) -> None:
        executor = DemoExecutor(self.project_dir)

        with tempfile.TemporaryDirectory(dir="/private/tmp") as temporary_dir:
            expanded = executor.expand_placeholders(["{file:text}", "{file:text}"], Path(temporary_dir))

            self.assertNotEqual(expanded[0], expanded[1])
            self.assertTrue(Path(expanded[0]).is_file())
            self.assertTrue(Path(expanded[1]).is_file())

    def test_fixture_arguments_are_copied_read_only(self) -> None:
        executor = DemoExecutor(self.project_dir)

        with tempfile.TemporaryDirectory(dir="/private/tmp") as temporary_dir:
            expanded = executor.copy_fixture_arguments(["demos/text/settings.elcl", "Sample"], Path(temporary_dir))

            copied_path = Path(expanded[0])
            self.assertNotEqual(self.fixture_path, copied_path)
            self.assertEqual("Value: original\n", copied_path.read_text(encoding="utf-8"))
            self.assertEqual(0o400, copied_path.stat().st_mode & 0o777)
            self.assertEqual("Sample", expanded[1])

    def test_run_expands_placeholders_for_demo_process(self) -> None:
        placeholder_executable = self.demo_output_dir / "placeholder"
        placeholder_executable.write_text(
            "#!/bin/sh\n"
            'if [ -f "$1" ] && [ -d "$2" ] && [ "$2" = "$3" ] && [ ! -e "$4" ]; then\n'
            "    printf 'placeholder ok\\n'\n"
            "else\n"
            "    printf 'placeholder failed\\n'\n"
            "    exit 7\n"
            "fi\n",
            encoding="utf-8",
        )
        placeholder_executable.chmod(0o755)
        executor = DemoExecutor(self.project_dir)

        output = executor.run("placeholder {file:text} {directory:cache} {directory:cache} {file:none}")

        self.assertEqual("placeholder ok\n", output)

    @unittest.skipUnless(os.name == "posix", "Pseudo-terminal capture requires POSIX.")
    def test_run_captures_ansi_emitted_only_for_an_interactive_terminal(self) -> None:
        terminal_executable = self.demo_output_dir / "terminal-output"
        terminal_executable.write_text(
            "#!/bin/sh\n"
            "if [ -t 1 ]; then\n"
            "    printf '\\033[32mterminal output\\033[0m\\n'\n"
            "    printf 'terminal size: '; stty size <&1\n"
            "else\n"
            "    printf 'plain output\\n'\n"
            "fi\n",
            encoding="utf-8",
        )
        terminal_executable.chmod(0o755)

        output = DemoExecutor(self.project_dir).run("terminal-output")

        self.assertIn("\x1b[32mterminal output", output)
        self.assertIn("terminal size: 40 90\n", output)

    def test_executable_path_rejects_missing_non_executable_and_symlink(self) -> None:
        executor = DemoExecutor(self.project_dir)
        non_executable = self.demo_output_dir / "plain"
        non_executable.write_text("#!/bin/sh\n", encoding="utf-8")
        symlink = self.demo_output_dir / "linked"
        os.symlink(self.executable_path, symlink)

        with self.assertRaises(DemoDocError):
            executor.executable_path("missing")
        with self.assertRaises(DemoDocError):
            executor.executable_path("plain")
        with self.assertRaises(DemoDocError):
            executor.executable_path("linked")


if __name__ == "__main__":
    unittest.main()
