# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import io
import multiprocessing
import queue
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.create_demo import CMAKE_HEADER, DemoPath, CreateDemoPlanner, CreateDemoRunner, lower_underscore
from lib.error import UtilityError
from run import REGISTERED_UTILITIES


def run_create_demo_process(project_dir: str, demo_path: str, result_queue: multiprocessing.Queue) -> None:
    """Run create-demo in a child process and report the result."""
    try:
        output = io.StringIO()
        CreateDemoRunner(
            Path(project_dir),
            DemoPath.parse(demo_path),
            assume_yes=True,
            input_stream=io.StringIO(),
            output_stream=output,
        ).run()
    except BaseException as error:
        result_queue.put(("error", demo_path, f"{type(error).__name__}: {error}"))
    else:
        result_queue.put(("ok", demo_path, output.getvalue()))


class TtyStringIO(io.StringIO):
    """A StringIO object that behaves like an interactive terminal."""

    def isatty(self) -> bool:
        """Pretend this stream is interactive."""
        return True


class CreateDemoTest(unittest.TestCase):
    """Tests for the create-demo utility."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_dir = Path(self.temp_dir.name)
        (self.project_dir / "demos").mkdir()
        (self.project_dir / "demos" / "CMakeLists.txt").write_text(
            f"{CMAKE_HEADER}\nadd_subdirectory(stream)\nadd_subdirectory(text)\n",
            encoding="utf-8",
        )

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_file(self, relative_path: str, text: str) -> None:
        """Write a file below the temporary project."""
        path = self.project_dir / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def read_file(self, relative_path: str) -> str:
        """Read a file below the temporary project."""
        return (self.project_dir / relative_path).read_text(encoding="utf-8")

    def run_create_demo(self, demo_path: str, *, assume_yes: bool = True, input_text: str = "") -> io.StringIO:
        """Run the create-demo runner and return its output."""
        output = io.StringIO()
        CreateDemoRunner(
            self.project_dir,
            DemoPath.parse(demo_path),
            assume_yes=assume_yes,
            input_stream=TtyStringIO(input_text),
            output_stream=output,
        ).run()
        return output

    def create_existing_schema_demo(self) -> None:
        """Create a deliberately unsorted existing schema-style demo."""
        self.write_file(
            "demos/text/CMakeLists.txt",
            f"{CMAKE_HEADER}\nadd_subdirectory(StringView)\n",
        )
        self.write_file(
            "demos/text/StringView/CMakeLists.txt",
            f"{CMAKE_HEADER}\n"
            "erbsland_core_add_demo(string_view)\n"
            "target_sources(string_view PRIVATE\n"
            "        Zebra.cpp\n"
            "        Alpha.cpp\n"
            "        main.cpp\n"
            "        StringViewDemos.hpp\n"
            ")\n",
        )
        self.write_file(
            "demos/text/StringView/StringViewDemos.hpp",
            "// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev\n"
            "// SPDX-License-Identifier: Apache-2.0\n"
            "#pragma once\n"
            "\n"
            "#include <DemoCommon.hpp>\n"
            "\n"
            "void zebra();\n"
            "void alpha();\n",
        )
        self.write_file(
            "demos/text/StringView/main.cpp",
            "// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev\n"
            "// SPDX-License-Identifier: Apache-2.0\n"
            "\n"
            '#include "StringViewDemos.hpp"\n'
            "\n"
            "#include <DemoCommon.hpp>\n"
            "\n"
            "auto main(const int argc, char *argv[]) -> int {\n"
            "    auto app = DemoApplication{argc, argv};\n"
            '    app.registerDemo("Zebra"_el, zebra);\n'
            '    app.registerDemo("Alpha"_el, alpha);\n'
            "    return app.run();\n"
            "}\n",
        )

    def test_utility_is_registered(self) -> None:
        self.assertIn("create_demo", REGISTERED_UTILITIES)

    def test_validates_demo_path(self) -> None:
        DemoPath.parse("text/StringView/BasicUsage")
        invalid_paths = (
            "text/StringView",
            "/text/StringView/BasicUsage",
            "text/stringView/BasicUsage",
            "text/StringView/basicUsage",
            "Text/StringView/BasicUsage",
            "text/../StringView/BasicUsage",
        )
        for invalid_path in invalid_paths:
            with self.subTest(invalid_path=invalid_path):
                with self.assertRaises(UtilityError):
                    DemoPath.parse(invalid_path)

    def test_converts_demo_name_to_target_name(self) -> None:
        self.assertEqual("string_view", lower_underscore("StringView"))
        self.assertEqual("u8_string_view", lower_underscore("U8StringView"))

    def test_creates_new_domain_and_demo_with_warning_and_documentation_block(self) -> None:
        output = self.run_create_demo("alpha/NewDemo/BasicUsage")

        self.assertIn("Library domain does not exist: src/erbsland/alpha/", output.getvalue())
        self.assertIn(":source: alpha/NewDemo/BasicUsage.cpp", output.getvalue())
        self.assertIn(":exec: alpha/new_demo --demo BasicUsage", output.getvalue())
        self.assertEqual(
            f"{CMAKE_HEADER}\n" "add_subdirectory(alpha)\n" "add_subdirectory(stream)\n" "add_subdirectory(text)\n",
            self.read_file("demos/CMakeLists.txt"),
        )
        self.assertEqual(f"{CMAKE_HEADER}\nadd_subdirectory(NewDemo)\n", self.read_file("demos/alpha/CMakeLists.txt"))
        self.assertIn("void basicUsage();\n", self.read_file("demos/alpha/NewDemo/NewDemoDemos.hpp"))
        self.assertIn(
            'app.registerDemo("BasicUsage"_el, basicUsage);\n', self.read_file("demos/alpha/NewDemo/main.cpp")
        )
        self.assertIn(
            "void basicUsage() {\n    // FIXME! Implement this demo.\n}\n",
            self.read_file("demos/alpha/NewDemo/BasicUsage.cpp"),
        )

    def test_creates_new_demo_in_existing_domain_and_warns_for_missing_header(self) -> None:
        self.write_file("demos/text/CMakeLists.txt", f"{CMAKE_HEADER}\nadd_subdirectory(StringView)\n")
        (self.project_dir / "src" / "erbsland" / "text").mkdir(parents=True)

        output = self.run_create_demo("text/NewDemo/BasicUsage")

        self.assertIn("No matching library header found below src/erbsland/text/: NewDemo.hpp", output.getvalue())
        self.assertEqual(
            f"{CMAKE_HEADER}\nadd_subdirectory(NewDemo)\nadd_subdirectory(StringView)\n",
            self.read_file("demos/text/CMakeLists.txt"),
        )

    def test_adds_demo_part_to_existing_demo_and_sorts_managed_lists(self) -> None:
        self.create_existing_schema_demo()
        self.write_file("src/erbsland/text/StringView.hpp", "#pragma once\n")

        self.run_create_demo("text/StringView/Mango")

        self.assertEqual(
            f"{CMAKE_HEADER}\n"
            "erbsland_core_add_demo(string_view)\n"
            "target_sources(string_view PRIVATE\n"
            "        Alpha.cpp\n"
            "        main.cpp\n"
            "        Mango.cpp\n"
            "        StringViewDemos.hpp\n"
            "        Zebra.cpp\n"
            ")\n",
            self.read_file("demos/text/StringView/CMakeLists.txt"),
        )
        self.assertIn(
            "void alpha();\n" "void mango();\n" "void zebra();\n",
            self.read_file("demos/text/StringView/StringViewDemos.hpp"),
        )
        self.assertIn(
            '    app.registerDemo("Alpha"_el, alpha);\n'
            '    app.registerDemo("Mango"_el, mango);\n'
            '    app.registerDemo("Zebra"_el, zebra);\n',
            self.read_file("demos/text/StringView/main.cpp"),
        )

    def test_duplicate_demo_part_fails_without_changes(self) -> None:
        self.create_existing_schema_demo()
        self.write_file("demos/text/StringView/Mango.cpp", "already here\n")

        with self.assertRaises(UtilityError):
            CreateDemoPlanner(self.project_dir, DemoPath.parse("text/StringView/Mango")).create_plan()

        self.assertEqual("already here\n", self.read_file("demos/text/StringView/Mango.cpp"))

    def test_parallel_processes_can_extend_same_demo(self) -> None:
        self.create_existing_schema_demo()
        self.write_file("src/erbsland/text/StringView.hpp", "#pragma once\n")
        demo_parts = ("Banana", "Mango", "Apricot", "Papaya", "Lychee", "Orange")

        context = multiprocessing.get_context("spawn")
        result_queue = context.Queue()
        processes = [
            context.Process(
                target=run_create_demo_process,
                args=(str(self.project_dir), f"text/StringView/{demo_part}", result_queue),
            )
            for demo_part in demo_parts
        ]
        for process in processes:
            process.start()
        try:
            results = [result_queue.get(timeout=20) for _process in processes]
        except queue.Empty:
            self.fail("Timed out waiting for parallel create-demo processes.")
        finally:
            for process in processes:
                process.join(timeout=10)
                if process.is_alive():
                    process.terminate()
                    process.join(timeout=10)

        self.assertEqual([], [result for result in results if result[0] != "ok"])
        self.assertEqual([], [process.exitcode for process in processes if process.exitcode != 0])

        cmake_text = self.read_file("demos/text/StringView/CMakeLists.txt")
        header_text = self.read_file("demos/text/StringView/StringViewDemos.hpp")
        main_text = self.read_file("demos/text/StringView/main.cpp")
        for demo_part in demo_parts:
            function_name = demo_part[:1].lower() + demo_part[1:]
            self.assertTrue((self.project_dir / "demos" / "text" / "StringView" / f"{demo_part}.cpp").is_file())
            self.assertIn(f"        {demo_part}.cpp\n", cmake_text)
            self.assertIn(f"void {function_name}();\n", header_text)
            self.assertIn(f'    app.registerDemo("{demo_part}"_el, {function_name});\n', main_text)

    def test_abort_confirmation_writes_no_files(self) -> None:
        result = CreateDemoRunner(
            self.project_dir,
            DemoPath.parse("text/NewDemo/BasicUsage"),
            assume_yes=False,
            input_stream=TtyStringIO("n\n"),
            output_stream=io.StringIO(),
        ).run()

        self.assertFalse(result)
        self.assertFalse((self.project_dir / "demos" / "text" / "NewDemo" / "BasicUsage.cpp").exists())

    def test_non_interactive_confirmation_fails_clearly(self) -> None:
        with self.assertRaises(UtilityError):
            CreateDemoRunner(
                self.project_dir,
                DemoPath.parse("text/NewDemo/BasicUsage"),
                assume_yes=False,
                input_stream=io.StringIO("y\n"),
                output_stream=io.StringIO(),
            ).run()


if __name__ == "__main__":
    unittest.main()
