# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
"""Process-level checks for startup and project file confinement."""

import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[2]
CURRENT_COPYRIGHT = (
    (PROJECT / "src/erbsland/text/punycode/impl/IdnaData.cpp").read_text().splitlines()[0].removeprefix("// ")
)
UNICODE_CONFIG = (PROJECT / "tool/conf/source/unicode.elcl").read_text()
BINARY = PROJECT / "cmake-build-debug/tool" / ("erbsland-core-tool.exe" if os.name == "nt" else "erbsland-core-tool")


@unittest.skipUnless(BINARY.is_file(), "Build the Debug erbsland-core-tool target first")
class CoreToolTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.base = Path(self.temporary.name).resolve()
        self.root = self.base / "project"
        self.root.mkdir()
        (self.root / "CMakeLists.txt").write_text("project(erbsland-core LANGUAGES CXX)\n")
        self.config = self.root / "tool/conf/erbsland-core-tool.elcl"
        self.config.parent.mkdir(parents=True)
        self.write_config("[rendering]\n[generate_idna_data]\n")
        (self.root / "tool/data/ucd").mkdir(parents=True)
        (self.root / "src/erbsland/text/punycode/impl").mkdir(parents=True)

    def write_config(self, text, *, unicode_directory=None):
        source = UNICODE_CONFIG
        if unicode_directory is not None:
            source = source.replace('"tool/data/ucd"', f'"{Path(unicode_directory).as_posix()}"')
        # Hold rendering fixed while comparing exact payloads with checked-in output.
        text = text.replace("[rendering]\n", f'[rendering]\ncopyright_line: "{CURRENT_COPYRIGHT}"\n', 1)
        self.config.write_text(text + "\n" + source)

    def invoke(self, *arguments, cwd=None):
        return subprocess.run(
            [str(BINARY), *map(str, arguments)], cwd=cwd or self.root, capture_output=True, text=True, timeout=60
        )

    def test_core_displays_work_without_a_project_or_configuration(self):
        self.write_config("intentionally invalid configuration")
        for arguments in [
            (),
            ("--help",),
            ("-h",),
            ("--version",),
            ("generate_idna_data", "--help"),
            ("generate_idna_data", "--help=--check"),
            ("generate_idna_data", "--version"),
            ("generate_idna_data", "--project", self.base, "--config", self.base, "--help"),
        ]:
            with self.subTest(arguments=arguments):
                result = self.invoke(*arguments, cwd=self.base)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertTrue(result.stdout.strip())
                self.assertNotIn("Could not find a valid Erbsland Core project directory", result.stderr)

    def test_execution_requires_a_root_and_module_first_options(self):
        result = self.invoke("generate_idna_data", cwd=self.base)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Could not find a valid Erbsland Core project directory", result.stderr)
        self.assertNotEqual(self.invoke("--project", self.root, "generate_idna_data").returncode, 0)
        self.assertNotEqual(self.invoke("generate_idna_data", "--project", self.base).returncode, 0)
        result = self.invoke("generate_idna_data", "--project", self.root, cwd=self.base)
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("Could not find a valid Erbsland Core project directory", result.stderr)

    @unittest.skipIf(os.name == "nt", "POSIX permission and symbolic-link probes")
    def test_access_denied_stops_discovery(self):
        denied = self.root / "denied"
        nested = denied / "nested"
        nested.mkdir(parents=True)
        denied.chmod(0o111)
        try:
            if os.access(denied, os.R_OK | os.X_OK):
                self.skipTest("Process can bypass directory permissions")
            result = self.invoke("generate_idna_data", cwd=nested)
            self.assertNotEqual(result.returncode, 0)
        finally:
            denied.chmod(0o700)

    def test_external_configuration_is_rejected(self):
        external = self.base / "outside.elcl"
        external.write_text("intentionally invalid configuration")
        result = self.invoke("generate_idna_data", "--config", external)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("leaves the validated project root", result.stderr)
        nested = self.root / "nested"
        nested.mkdir()
        result = self.invoke("generate_idna_data", "--config", "../../outside.elcl", cwd=nested)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("leaves the validated project root", result.stderr)

    @unittest.skipIf(os.name == "nt", "POSIX symbolic-link creation")
    def test_links_rejected_for_config_includes_inputs_and_outputs(self):
        external = self.base / "outside.elcl"
        external.write_text("outside sentinel")
        link = self.root / "linked.elcl"
        link.symlink_to(external)
        result = self.invoke("generate_idna_data", "--config", link)
        self.assertIn("Symbolic links", result.stderr)
        self.write_config('@include: "../../linked.elcl"\n')
        result = self.invoke("generate_idna_data")
        self.assertIn("Access denied", result.stderr)
        self.write_config('@include: "../../*.elcl"\n')
        result = self.invoke("generate_idna_data")
        self.assertIn("Access denied", result.stderr)
        self.write_config("[rendering]\n[generate_idna_data]\n")
        directory = self.root / "linked-directory"
        directory.symlink_to(self.base, target_is_directory=True)
        shutil.copytree(PROJECT / "tool/data/ucd", self.root / "tool/data/ucd", dirs_exist_ok=True)
        for option, value in [("--output", directory / "new.cpp"), ("--output", link)]:
            result = self.invoke("generate_idna_data", option, value)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Symbolic links", result.stderr)
        self.write_config("[rendering]\n[generate_idna_data]\n", unicode_directory=directory)
        result = self.invoke("generate_idna_data")
        self.assertIn("Symbolic links", result.stderr)
        self.assertEqual(external.read_text(), "outside sentinel")
        self.assertFalse((self.base / "new.cpp").exists())

    def test_complete_generation_preserves_output_and_timestamp(self):
        shutil.copytree(PROJECT / "tool/data/ucd", self.root / "tool/data/ucd", dirs_exist_ok=True)
        output = self.root / "generated.cpp"
        output.write_text("existing output")
        result = self.invoke("generate_idna_data", "--output", output, "--check")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("differs", result.stdout)
        self.assertEqual(output.read_text(), "existing output")
        self.assertEqual(self.invoke("generate_idna_data", "--output", output).returncode, 0)
        expected = PROJECT / "src/erbsland/text/punycode/impl/IdnaData.cpp"
        self.assertEqual(output.read_bytes(), expected.read_bytes())
        modified = output.stat().st_mtime_ns
        self.assertEqual(self.invoke("generate_idna_data", "--output", output).returncode, 0)
        self.assertEqual(output.stat().st_mtime_ns, modified)
        (self.root / "tool/data/ucd/UnicodeData.txt").write_text("corrupted dataset")
        previous_names = sorted(path.name for path in self.root.iterdir())
        result = self.invoke("generate_idna_data", "--output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("checksum recorded in the manifest", result.stderr)
        self.assertEqual(output.read_bytes(), expected.read_bytes())
        self.assertEqual(sorted(path.name for path in self.root.iterdir()), previous_names)

    def test_external_includes_are_denied_before_parsing(self):
        external = self.base / "outside.elcl"
        external.write_text("intentionally invalid configuration")
        for include in [external.as_posix(), "../../../outside.elcl", "../../../*.elcl"]:
            with self.subTest(include=include):
                self.write_config(f'@include: "{include}"\n')
                result = self.invoke("generate_idna_data")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Access denied", result.stderr)

    @unittest.skipIf(os.name == "nt", "POSIX symbolic-link creation")
    def test_core_resolver_accepts_include_links_with_internal_targets(self):
        target = self.root / "internal.elcl"
        target.write_text("[rendering]\n[another_tool]\nvalue: 1\n[generate_idna_data]\n")
        link = self.root / "linked.elcl"
        link.symlink_to(target)
        self.write_config('@include: "../../linked.elcl"\n')
        result = self.invoke("generate_idna_data")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("manifest", result.stderr)
        self.assertNotIn("Access denied", result.stderr)

    def test_generation_check_and_failure_preserve_output(self):
        self.assertEqual(self.invoke("generate_idna_data", "--check", cwd=PROJECT).returncode, 0)
        output = self.root / "generated.cpp"
        output.write_text("existing output")
        result = self.invoke("generate_idna_data", "--output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(output.read_text(), "existing output")
        shutil.copytree(PROJECT / "tool/data/ucd", self.root / "tool/data/ucd", dirs_exist_ok=True)
        result = self.invoke("generate_idna_data", "--output", self.base / "outside.cpp")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("leaves the validated project root", result.stderr)
        self.assertFalse((self.base / "outside.cpp").exists())

    def test_only_selected_configuration_section_is_validated(self):
        self.write_config("[rendering]\n[another_tool]\noutput_file: 1\n[generate_idna_data]\n")
        result = self.invoke("generate_idna_data")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("manifest", result.stderr)
        self.write_config("[rendering]\n[another_tool]\noutput_file: 1\n[generate_idna_data]\noutput_file: 1\n")
        result = self.invoke("generate_idna_data")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("output_file", result.stderr)
        self.assertNotIn("manifest", result.stderr)

    def test_psl_generation_overrides_check_and_preservation(self):
        shutil.copytree(PROJECT / "tool/data/ucd", self.root / "tool/data/ucd", dirs_exist_ok=True)
        inputs = self.root / "inputs"
        shutil.copytree(PROJECT / "tool/data/public_suffix", inputs)
        source = inputs / "custom.dat"
        (inputs / "public_suffix_list.dat").rename(source)
        manifest = inputs / "manifest.elcl"
        manifest.write_text(manifest.read_text().replace("public_suffix_list.dat", "custom.dat"))
        output = self.root / "generated.cpp"
        self.write_config(
            f'[rendering]\n[generate_public_suffix_data]\noutput_file: "missing"\n[public_suffix]\ninput_file: "{source.as_posix()}"\n'
        )
        args = ("generate_public_suffix_data", "-o", output)
        result = self.invoke(*args, "-c")
        self.assertNotEqual(result.returncode, 0, result.stderr)
        self.assertFalse(output.exists())
        result = self.invoke(*args)
        self.assertEqual(result.returncode, 0, result.stderr)
        expected = (PROJECT / "src/erbsland/network/impl/http/cookie/PublicSuffixData.cpp").read_bytes()
        self.assertEqual(output.read_bytes(), expected)
        # Equivalent regeneration preserves both the old copyright and CRLF bytes.
        first_line, remaining = expected.split(b"\n", 1)
        old_header = re.sub(rb"[0-9]{4}(?:-[0-9]{4})?", b"2001", first_line, count=1)
        preserved = (old_header + b"\n" + remaining).replace(b"\n", b"\r\n").rstrip(b"\r\n")
        output.write_bytes(preserved)
        modified = output.stat().st_mtime_ns
        for check in [(), ("--check",)]:
            result = self.invoke(*args, *check)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(output.read_bytes(), preserved)
            self.assertEqual(output.stat().st_mtime_ns, modified)
        output.write_bytes(preserved + b"\r\n\r\n")
        self.assertNotEqual(self.invoke(*args, "--check").returncode, 0)
        self.assertEqual(self.invoke(*args).returncode, 0)
        self.assertEqual(output.read_bytes(), expected)
        source.write_bytes(source.read_bytes() + b"\n")
        result = self.invoke(*args)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Integrity Check Failed", result.stderr)
        self.assertEqual(output.read_bytes(), expected)
        self.write_config(
            f'[rendering]\n[generate_public_suffix_data]\n[public_suffix]\ninput_file: "{(self.base / "outside.dat").as_posix()}"\n'
        )
        result = self.invoke(*args)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("leaves the validated project root", result.stderr)

    def test_psl_help_and_invalid_configuration(self):
        result = self.invoke("generate_public_suffix_data", "--help", cwd=self.base)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("--input-file", result.stdout)
        self.assertNotIn("--ucd-directory", result.stdout)
        self.assertIn("--output-file", result.stdout)
        self.write_config("[rendering]\n[generate_public_suffix_data]\nunknown: 1\n")
        result = self.invoke("generate_public_suffix_data")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unknown", result.stderr)
        self.write_config(
            '[rendering]\nlicense_line: "${env:ERBSLAND_CORE_CERTAINLY_MISSING,required}"\n[generate_public_suffix_data]\n'
        )
        result = self.invoke("generate_public_suffix_data")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ERBSLAND_CORE_CERTAINLY_MISSING", result.stderr)

    def test_rendering_section_is_required(self):
        self.write_config("[generate_idna_data]\n")
        result = self.invoke("generate_idna_data")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("requires a [rendering] section", result.stderr)

    def test_removed_source_options_and_tool_settings_are_rejected(self):
        for command, options in [
            ("generate_idna_data", ["-u", "--ucd-directory", "--ucd-dir"]),
            ("generate_public_suffix_data", ["-u", "--ucd-directory", "--ucd-dir", "-i", "--input-file", "--input"]),
        ]:
            for option in options:
                with self.subTest(command=command, option=option):
                    result = self.invoke(command, option, "unused")
                    self.assertNotEqual(result.returncode, 0)
            for setting in ["ucd_directory", "unicode_version"]:
                self.write_config(f'[rendering]\n[{command}]\n{setting}: "old"\n')
                result = self.invoke(command)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(setting, result.stderr)
        self.write_config('[rendering]\n[generate_public_suffix_data]\ninput_file: "old"\n')
        self.assertIn("input_file", self.invoke("generate_public_suffix_data").stderr)

    def test_unused_psl_source_is_not_loaded_or_validated(self):
        shutil.copytree(PROJECT / "tool/data/ucd", self.root / "tool/data/ucd", dirs_exist_ok=True)
        self.write_config("[rendering]\n[generate_idna_data]\n[public_suffix]\ninput_file: 1\n")
        result = self.invoke("generate_idna_data", "--output", self.root / "generated.cpp")
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_source_configuration_and_version_failures_preserve_output(self):
        output = self.root / "generated.cpp"
        output.write_text("existing output")
        self.write_config("[rendering]\n[generate_idna_data]\n")
        self.config.write_text(self.config.read_text().replace("Unicode Version", "Unknown Value"))
        result = self.invoke("generate_idna_data", "--output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(output.read_text(), "existing output")
        shutil.copytree(PROJECT / "tool/data/ucd", self.root / "tool/data/ucd", dirs_exist_ok=True)
        self.write_config("[rendering]\n[generate_idna_data]\n")
        # Alter the configured pin without changing the independently pinned manifest.
        lines = self.config.read_text().splitlines()
        lines = ['Unicode Version: "999.0.0"' if line.startswith("Unicode Version") else line for line in lines]
        self.config.write_text("\n".join(lines) + "\n")
        result = self.invoke("generate_idna_data", "--output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unexpected Unicode version", result.stderr)
        self.assertEqual(output.read_text(), "existing output")
