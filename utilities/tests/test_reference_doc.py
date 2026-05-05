# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import io
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from dev.reference_doc import ApiEntryKind, HeaderScanner, ReferenceDocConfig, ReferenceDocGenerator, ReferenceGroup
from lib.error import UtilityError


class ReferenceDocTest(unittest.TestCase):
    """Tests for the reference documentation utility."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory(dir="/private/tmp")
        self.project_dir = Path(self.temp_dir.name)
        self.source_dir = self.project_dir / "src" / "erbsland"
        self.reference_dir = self.project_dir / "doc" / "reference"
        self.source_dir.mkdir(parents=True)
        self.reference_dir.mkdir(parents=True)
        self.config = ReferenceDocConfig(
            project_dir=self.project_dir,
            source_dir=self.source_dir,
            reference_dir=self.reference_dir,
            excluded_directory_names=frozenset({"impl"}),
            excluded_header_names=frozenset({"all.hpp", "fwd.hpp"}),
            manual_header_paths=frozenset({Path("core/Definitions.hpp")}),
            exclude_underscore_headers=True,
        )
        self.generator = ReferenceDocGenerator(self.config)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def write_header(self, relative_path: str, text: str) -> Path:
        """Write a temporary source header."""
        path = self.source_dir / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def write_reference(self, relative_path: str, text: str) -> Path:
        """Write a temporary reference page."""
        path = self.reference_dir / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def test_path_mapping_and_exclusions(self) -> None:
        header_path = self.source_dir / "math" / "SaturatingInteger.hpp"
        page_path = self.config.reference_page_path(header_path)

        self.assertEqual(self.reference_dir / "math" / "saturating_integer.rst", page_path)
        self.assertEqual(
            self.reference_dir / "unit" / "index_api.rst",
            self.config.reference_page_path(self.source_dir / "unit" / "Index.hpp"),
        )
        self.assertTrue(self.config.should_skip_header(self.source_dir / "math" / "all.hpp"))
        self.assertTrue(self.config.should_skip_header(self.source_dir / "math" / "fwd.hpp"))
        self.assertTrue(self.config.should_skip_header(self.source_dir / "math" / "Name_fwd.hpp"))
        self.assertTrue(self.config.should_skip_header(self.source_dir / "math" / "impl" / "Name.hpp"))
        self.assertTrue(self.config.should_skip_header(self.source_dir / "core" / "Definitions.hpp"))
        self.assertFalse(self.config.should_skip_header(header_path))

    def test_extracts_documented_namespace_scope_declarations(self) -> None:
        path = self.write_header(
            "math/Example.hpp",
            """#pragma once

namespace erbsland::math {

/// A documented class.
template <typename T,
    typename U = T>
    requires true
class Example {
public:
    using Nested = int;
};

/// A documented enum.
enum class Mode { A, B };

using Ignored = int;

/// A documented alias.
using ExampleAlias = Example<int>;

}
""",
        )

        entries = HeaderScanner(self.config).scan(path).entries

        self.assertEqual(
            [
                (ApiEntryKind.CLASS, "erbsland::math::Example"),
                (ApiEntryKind.ENUM_CLASS, "erbsland::math::Mode"),
                (ApiEntryKind.TYPEDEF, "erbsland::math::ExampleAlias"),
            ],
            [(entry.kind, entry.full_name) for entry in entries],
        )

    def test_extracts_documented_namespace_scope_functions(self) -> None:
        path = self.write_header(
            "math/Functions.hpp",
            """#pragma once

namespace erbsland::math {

/// A documented function.
auto add(int left, int right) -> int;

class Undocumented {};

}
""",
        )
        header = HeaderScanner(self.config).scan(path)

        self.assertEqual(
            [(ApiEntryKind.FUNCTION, "erbsland::math::add(int left, int right) -> int")],
            [(entry.kind, entry.full_name) for entry in header.entries],
        )
        self.assertEqual(
            ".. doxygenfunction:: erbsland::math::add(int left, int right) -> int\n",
            self.generator.interface_for_header(header),
        )

    def test_extracts_documented_functions_with_attributes_and_split_return(self) -> None:
        path = self.write_header(
            "err/ThrowHelper.hpp",
            """#pragma once

namespace erbsland::err {

/// Throw an error.
[[noreturn]]
void throwError();

/// Create a value.
[[nodiscard]] constexpr auto
makeValue(int value) noexcept -> int {
    return value;
}

/// Add values.
[[nodiscard]] constexpr auto addValues(
    int first, int second) noexcept -> int {
    return first + second;
}

/// Create a wide value.
[[nodiscard]] constexpr auto wideValue(int value) noexcept
    -> int {
    return value;
}

}
""",
        )

        entries = HeaderScanner(self.config).scan(path).entries

        self.assertEqual(
            [
                (ApiEntryKind.FUNCTION, "erbsland::err::throwError()"),
                (ApiEntryKind.FUNCTION, "erbsland::err::makeValue(int value) noexcept -> int"),
                (ApiEntryKind.FUNCTION, "erbsland::err::addValues(int first, int second) noexcept -> int"),
                (ApiEntryKind.FUNCTION, "erbsland::err::wideValue(int value) noexcept -> int"),
            ],
            [(entry.kind, entry.full_name) for entry in entries],
        )

    def test_uses_doxygenfile_fallback_for_headers_without_documented_entries(self) -> None:
        path = self.write_header(
            "math/Functions.hpp",
            """#pragma once

namespace erbsland::math {

class Undocumented {};

}
""",
        )
        header = HeaderScanner(self.config).scan(path)

        self.assertEqual((), header.entries)
        self.assertEqual(".. doxygenfile:: erbsland/math/Functions.hpp\n", self.generator.interface_for_header(header))

    def test_replace_interface_preserves_text_before_section(self) -> None:
        path = self.write_reference(
            "math/example.rst",
            """Title
=====

Keep this text.

Interface
=========

.. old:: entry
""",
        )

        updated = self.generator.replace_interface_body(path, path.read_text(encoding="utf-8"), ".. new:: entry\n")

        self.assertEqual(
            """Title
=====

Keep this text.

Interface
=========

.. new:: entry
""",
            updated,
        )

    def test_orphan_page_gets_warning_interface(self) -> None:
        path = self.write_reference(
            "math/old.rst",
            """Old
===

Interface
=========

.. old:: entry
""",
        )

        self.generator.update_orphan_pages([])

        self.assertIn("No matching public API header was found", path.read_text(encoding="utf-8"))

    def test_manual_reference_page_is_not_marked_as_orphan(self) -> None:
        path = self.write_reference(
            "core/definitions.rst",
            """Definitions
===========

Interface
=========

.. doxygenfile:: erbsland/core/Definitions.hpp
""",
        )

        self.generator.update_orphan_pages([])

        self.assertNotIn("No matching public API header was found", path.read_text(encoding="utf-8"))

    def test_toctree_entries_are_sorted(self) -> None:
        path = self.write_reference(
            "math/index.rst",
            """Math
****

.. toctree::
    :maxdepth: 3

    zebra
    alpha
""",
        )

        updated = self.generator.replace_toctree_entries(path, path.read_text(encoding="utf-8"), ["zebra", "alpha"])

        self.assertEqual(
            """Math
****

.. toctree::
    :maxdepth: 3

    zebra
    alpha
""".replace("    zebra\n    alpha", "    alpha\n    zebra"),
            updated,
        )

    def test_indexes_include_existing_reference_pages(self) -> None:
        self.write_reference(
            "err/error.rst",
            """Error
=====

Interface
=========

.. warning::

    Orphan page.
""",
        )

        entries = self.generator.index_entries([])

        self.assertIn("error", entries[self.reference_dir / "err"])
        self.assertIn("err/index", entries[self.reference_dir])

    def test_malformed_reference_page_fails(self) -> None:
        path = self.write_reference(
            "math/broken.rst",
            """Broken
======

No interface here.
""",
        )

        with self.assertRaises(UtilityError):
            self.generator.replace_interface_body(path, path.read_text(encoding="utf-8"), ".. entry:: x\n")

    def test_grouped_headers_share_reference_page(self) -> None:
        self.write_header(
            "text/StringList.hpp",
            """#pragma once

namespace erbsland::text {

/// A documented alias.
using StringList = int;

}
""",
        )
        self.write_header(
            "text/StringMap.hpp",
            """#pragma once

namespace erbsland::text {

/// A documented alias.
using StringMap = int;

}
""",
        )
        group = ReferenceGroup(
            page_path=self.reference_dir / "text" / "string_collections.rst",
            relative_page_path=Path("text/string_collections.rst"),
            title="String Collections",
            header_paths=(Path("text/StringList.hpp"), Path("text/StringMap.hpp")),
        )
        config = ReferenceDocConfig(
            project_dir=self.project_dir,
            source_dir=self.source_dir,
            reference_dir=self.reference_dir,
            excluded_directory_names=frozenset({"impl"}),
            excluded_header_names=frozenset({"all.hpp", "fwd.hpp"}),
            manual_header_paths=frozenset(),
            exclude_underscore_headers=True,
            reference_groups=(group,),
        )
        generator = ReferenceDocGenerator(config)

        headers = generator.collect_headers()
        generator.update_reference_pages(headers)

        page = self.reference_dir / "text" / "string_collections.rst"
        text = page.read_text(encoding="utf-8")
        self.assertIn("String Collections", text)
        self.assertIn(".. doxygentypedef:: erbsland::text::StringList", text)
        self.assertIn(".. doxygentypedef:: erbsland::text::StringMap", text)
        self.assertEqual({page}, {header.page_path for header in headers})

    def test_grouped_page_is_included_in_index_entries(self) -> None:
        self.write_header(
            "text/StringList.hpp",
            """#pragma once

namespace erbsland::text {

/// A documented alias.
using StringList = int;

}
""",
        )
        group = ReferenceGroup(
            page_path=self.reference_dir / "text" / "string_collections.rst",
            relative_page_path=Path("text/string_collections.rst"),
            title="String Collections",
            header_paths=(Path("text/StringList.hpp"),),
        )
        config = ReferenceDocConfig(
            project_dir=self.project_dir,
            source_dir=self.source_dir,
            reference_dir=self.reference_dir,
            excluded_directory_names=frozenset(),
            excluded_header_names=frozenset(),
            manual_header_paths=frozenset(),
            exclude_underscore_headers=True,
            reference_groups=(group,),
        )
        generator = ReferenceDocGenerator(config)

        entries = generator.index_entries(generator.collect_headers())

        self.assertIn("string_collections", entries[self.reference_dir / "text"])
        self.assertIn("text/index", entries[self.reference_dir])

    def test_uncategorized_headers_are_warned_but_still_use_legacy_pages(self) -> None:
        self.write_header(
            "math/Example.hpp",
            """#pragma once

namespace erbsland::math {

/// A documented alias.
using Example = int;

}
""",
        )

        headers = self.generator.collect_headers()
        stderr = io.StringIO()
        with redirect_stderr(stderr):
            self.generator.warn_uncategorized_headers(headers)

        self.assertIn("not assigned to reference groups", stderr.getvalue())
        self.assertEqual(self.reference_dir / "math" / "example.rst", headers[0].page_path)

    def test_duplicate_group_headers_fail_when_reading_config(self) -> None:
        self.write_header(
            "text/StringList.hpp",
            """#pragma once

namespace erbsland::text {

/// A documented alias.
using StringList = int;

}
""",
        )
        config_path = self.project_dir / "reference_doc.elcl"
        config_path.write_text(
            """[Main]
Source Directory: "src/erbsland"
Reference Directory: "doc/reference"
Exclude Underscore Headers: Yes

*[Reference Groups]
Page: "text/one.rst"
Headers: "text/StringList.hpp"

*[Reference Groups]
Page: "text/two.rst"
Headers: "text/StringList.hpp"
""",
            encoding="utf-8",
        )

        with self.assertRaises(UtilityError):
            ReferenceDocConfig.read(self.project_dir, config_path)


if __name__ == "__main__":
    unittest.main()
