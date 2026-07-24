# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from lib.api_guidelines import validate_api_guideline_text

TITLE = """**********************
Example API Guidelines
**********************

"""


def messages(text: str) -> list[str]:
    """Get validation messages without line numbers."""
    return [issue.message for issue in validate_api_guideline_text(text)]


class ApiGuidelinesTest(unittest.TestCase):
    """Tests for domain-specific API guideline validation."""

    def test_rejects_page_over_500_lines(self) -> None:
        source = TITLE + "Primary Types\n=============\n\n.. code-block:: text\n\n    Value // value\n"
        source += "\n" * (501 - len(source.splitlines()))

        result = messages(source)

        self.assertIn("page must not exceed 500 lines.", result)

    def test_accepts_complete_page(self) -> None:
        source = TITLE + """Core Semantics
==============

Vocabulary
----------

``value``
    One immutable value.

Primary Types
=============

.. code-block:: text

    Widget, WidgetEditor // immutable value and mutable editor
    Value❮Kind❯ // value selected by kind
    ❮domain❯::❮Type❯Error // domain-specific error pattern

Secondary Types
===============

.. code-block:: text

    WidgetError // widget operation error

Pattern Definitions
===================

.. code-block:: text

    V, Vp = Value/Value❮Kind❯ // value or kind-specific value

Widget Patterns
===============

.. code-block:: text

    T(value) // create a widget
    T::fromValue(value) -> T // create from a value
    o.value() -> V // access the value
    o.get❮Type❯OrThrow() -> V // access a typed value or throw
    o.❮operation❯() // apply a named operation
    o.first()/last() -> V // access a boundary value
    o.enable/disable() // change a state
    makeWidget([value]) -> Widget // create a default or initialized widget
"""

        self.assertEqual([], validate_api_guideline_text(source))

    def test_rejects_wrong_title_and_section_order(self) -> None:
        source = """======================
Example API Guidelines
======================

Example Patterns
================

.. code-block:: text

    o.value() // access the value

Primary Types
=============

.. code-block:: text

    Widget // one widget
"""

        result = messages(source)

        self.assertIn("page title must use matching '*' lines above and below the title.", result)
        self.assertIn("pattern sections must follow all type sections.", result)
        self.assertIn("type sections must precede pattern sections.", result)

    def test_rejects_content_before_first_section(self) -> None:
        source = TITLE + """This introduction is not allowed.

Primary Types
=============

.. code-block:: text

    Widget // one widget
"""

        self.assertIn(
            "content outside Core Semantics and structural sections is not allowed.",
            messages(source),
        )

    def test_requires_primary_and_secondary_names(self) -> None:
        source = TITLE + """Value Types
===========

.. code-block:: text

    Value // one value

Helper Types
============

.. code-block:: text

    Helper // one helper
"""

        result = messages(source)

        self.assertIn("the first type section must be Primary Types.", result)
        self.assertIn("with two type sections, the second must be Secondary Types.", result)

    def test_limits_core_semantics_and_subsections(self) -> None:
        semantics = "\n".join(f"line {index}" for index in range(61))
        subsections = "\n".join(f"\nPart {index}\n------\n" for index in range(5))
        source = TITLE + f"""Core Semantics
==============

{semantics}
{subsections}
Primary Types
=============

.. code-block:: text

    Widget // one widget
"""

        result = messages(source)

        self.assertIn("Core Semantics must not exceed 60 lines.", result)
        self.assertIn("Core Semantics allows at most 4 subsections.", result)

    def test_rejects_prose_and_bad_code_block_spacing(self) -> None:
        source = TITLE + """Primary Types
=============
.. code-block:: text
    Widget // one widget

This prose is not allowed.
"""

        self.assertIn(
            "section must contain only one correctly spaced '.. code-block:: text' block.",
            messages(source),
        )

    def test_validates_type_lines(self) -> None:
        source = TITLE + """Primary Types
=============

.. code-block:: text

    widget // invalid lowercase type.
    List<Value> // use a placeholder-based type pattern
"""

        result = messages(source)

        self.assertIn("type entry must contain comma-separated type names or type patterns.", result)
        self.assertIn("description must not end with a period.", result)

    def test_validates_pattern_definitions(self) -> None:
        source = TITLE + """Primary Types
=============

.. code-block:: text

    Widget // one widget

Pattern Definitions
===================

.. code-block:: text

    value = Widget // invalid shortcut
    V = Widget | Other // invalid type separator
"""

        result = messages(source)

        self.assertIn(
            "pattern shortcuts must be comma-separated one- or two-character names starting uppercase.",
            result,
        )
        self.assertIn("pattern definition value must contain type patterns separated by '/' or ', '.", result)

    def test_validates_patterns_and_return_types(self) -> None:
        source = TITLE + """Primary Types
=============

.. code-block:: text

    Widget // one widget

Widget Patterns
===============

.. code-block:: text

    widget.value() // invalid object spelling
    o.value() -> Widget/Other // multiple return types
    o.value() -> Widget -> Other // two arrows
"""

        result = messages(source)

        self.assertIn(
            "pattern must be a T constructor, T:: static method, o. object method, or free function.",
            result,
        )
        self.assertIn("invalid return type in pattern.", result)
        self.assertIn("pattern allows at most one return type.", result)

    def test_accepts_optional_and_nested_template_return_types(self) -> None:
        source = TITLE + """Primary Types
=============

.. code-block:: text

    Widget // one widget

Widget Patterns
===============

.. code-block:: text

    o.value() -> [Widget] // access an optional value
    o.values() -> Map<Key, List<Value>> // access nested values
"""

        self.assertEqual([], validate_api_guideline_text(source))

    def test_rejects_trailing_call_syntax(self) -> None:
        source = TITLE + """Primary Types
=============

.. code-block:: text

    Widget // one widget

Widget Patterns
===============

.. code-block:: text

    o.value() trailing() // invalid trailing call syntax
"""

        self.assertIn(
            "pattern must be a T constructor, T:: static method, o. object method, or free function.",
            messages(source),
        )

    def test_rejects_long_or_incorrectly_indented_lines(self) -> None:
        long_description = "x" * 110
        source = TITLE + f"""Primary Types
=============

.. code-block:: text

     Widget // wrong indentation
    Other // {long_description}
"""

        result = messages(source)

        self.assertIn("code-block lines must use exactly four spaces of indentation.", result)
        self.assertIn("code-block lines must not exceed 120 characters.", result)


if __name__ == "__main__":
    unittest.main()
