***************************
How to Write API Guidelines
***************************

Why API Guidelines
==================

API guidelines enforce a consistent vocabulary across the large API surface of this library.
All guidelines are based on the core principles described in :doc:`api/common`.
Each guideline describes the API of a specific domain in its most compact and condensed form.

When the API is extended, these guidelines provide an immediate overview of the domain and help prevent the API from
diverging.
For example, they avoid situations where one class uses ``erase`` and another uses ``remove`` for the same operation.

Common Principles
=================

* Guidelines do not use prose to describe a domain. Instead, they rely on compact pattern blocks.
* Short technical explanations are only allowed when patterns alone are insufficient.
  Such explanations belong in subsections of the initial **Core Semantics** section.
* Type lists and API patterns are always placed inside ``.. code-block:: text`` blocks.

Main Page Structure
===================

We use ``❮placeholder❯`` here and throughout our guidelines to avoid confusion with template parameters such as ``<T>``.
The following structure also uses ``❮...❯`` for notes and explanations.
This notation is only used in this template and must not appear in an actual guideline page.
The page title uses matching ``*`` adornments above and below it.

.. code-block:: rst

    ******************************
    ❮Domain❯ Domain API Guidelines
    ******************************

    These guidelines extend the Common API Guidelines for public APIs in the ``❮...❯`` namespace.
    If you introduce new API, update this page to provide a good reference for future extensions.

    Core Semantics
    ==============

    ❮... core semantics ...❯

    Primary Types
    =============

    .. code-block:: text

        ExampleA  // represents ❮...❯
        ExampleB  // holds ❮...❯
        ExampleC  // provides ❮...❯

    Secondary Types
    ===============

    .. code-block:: text

        ExampleD  // represents ❮...❯
        ExampleE  // holds ❮...❯
        ExampleF  // provides ❮...❯

        // group name
        ExampleG  // ...

    ❮...❯ Patterns
    ==============

    .. code-block:: text

        o.foo(V)  // does/tests/makes/...
        o.getExample() -> T  // gets/...
        T::bar(U, V)  // builds/creates/converts/...

    ❮... additional pattern blocks if necessary ...❯
    ❮... end of document ...❯

* **Core Semantics:** This *optional* initial section defines the fundamental principles of the domain in an extremely
  compact and technical form.
  It has at most 60 content lines and may contain at most four subsections using the ``-`` adornment.
  Less is better. Useful subsections include:

  - **Vocabulary:** A compact definition list.
    A vocabulary block only makes sense when terms are ambiguous and can have multiple meanings within the domain.
    This is not a thesaurus, index, or encyclopedia. Well-known and unambiguous terms do not belong in such a list.

  - **Special Naming Rules:** Short prose or illustrative text blocks.
    A good example from the text domain is explaining the relationship between ``StringEditor``, ``U(8/16/32)StringEditor``,
    ``String``, ``StringEditor``, and their ``U(8/16/32)`` width-specific forms.
    A simple type list would not adequately explain the API surface and could lead to confusion.
    By explaining how these names relate, the pattern sections can focus on ``StringEditor`` and ``String``.

  - **Special Placeholders:** Used when the placeholders defined in :doc:`api/common` are not sufficient and introducing additional placeholders improves the readability of the following patterns.

  - **Important Behavior:** Used when behavior cannot be fully derived from naming and common expectations.
    For example, whether a begin-end range includes or excludes the last element, if that behavior differs from what developers would typically expect.

* **Type Sections:** One or more sections list the types in the domain.
  The first section is always named **Primary Types**.
  If there are exactly two type sections, the second is named **Secondary Types**.
  With three or more type sections, every additional section uses a descriptive title ending in **Types**.

  **Primary Types** contains only the types that define the domain and its main user-facing abstractions.
  A type is primary when removing it from the overview would hide a fundamental capability or value model of the domain.
  Options, settings, result records, status values, callbacks, editors, implementation interfaces, and other types that
  merely configure or support a primary type are not primary types.

  The section consists of a single ``.. code-block:: text`` without surrounding prose.
  Each line contains one or more comma-separated type names, followed by whitespace, ``//``, and a short description.
  Lines must not exceed 120 characters, and descriptions do not end with a period.

  .. code-block:: text

      ❮Type❯  // ❮description❯

  Closely related types may be grouped on a single line:

  .. code-block:: text

      ❮Type1❯, ❮Type2❯, ❮Type3❯  // ❮description❯

  No more than five types should be grouped together.

  **Secondary Types** contains the remaining types of the domain.

  The section consists of a single ``.. code-block:: text`` without surrounding prose.
  For larger domains, multiple sections may be used instead of a single **Secondary Types** section.
  In that case, replace the word *Secondary* with a more meaningful group name.

* **Pattern Definitions:** This *optional* section defines one- or two-character uppercase shortcuts used in pattern
  blocks.
  It follows all type sections and consists of one ``.. code-block:: text`` without surrounding prose.

  .. code-block:: text

      V, Vp = Value/Value❮Kind❯  // value or kind-specific value

* **Pattern Blocks:** The remainder of the document consists of zero or more pattern blocks.

  Each pattern block describes the API patterns used within the domain.
  Pattern blocks illustrate naming and behavior conventions rather than providing an exhaustive API reference.

  Each section consists of a single ``.. code-block:: text`` without surrounding prose:

  .. code-block:: text

      o.remove(range) -> T&  // remove a character-based range in-place
      o.append(character/text[, count]) -> T&  // append code point(s) or text
      o.allOf/anyOf/noneOf(function) -> bool  // predicate tests over all elements
      o.write(character/text)  // write text without adding a line ending
      T::zero() -> T  // the zero value
      T::fromInteger(v, format) -> T  // create from an integer

  Patterns are usually built around an object instance (``o.call()``) or a static type (``T::call()``), where
  ``o`` and ``T`` always represent the object and type being described.

  The arrow ``->`` indicates a return value.

  Each pattern fits on a line of at most 120 characters and ends with whitespace, ``//``, and a concise description
  without a final period.
  The description should answer the question:

  "If I add a method that matches this pattern, what behavior will users expect from it?"

  Variants can be compacted using ``/``, ``[Optional]``, and placeholders to keep pattern blocks concise and readable.

  Prefer concept-oriented blocks shared by several types, such as **Read Patterns**, **Write Patterns**, or
  **Value Object Patterns**. Do not repeat functionally equivalent getter, setter, construction, conversion, or
  lifecycle patterns for every participating type. One representative pattern is enough to establish the convention.

  Each pattern is a constructor on ``T``, a static method on ``T``, an object method on ``o``, or a free function.
  A single optional return type follows ``->`` with surrounding spaces.

  .. code-block:: text

      T(...)  // construct the section type
      T::create(...) -> T  // create the section type
      o.value() -> V  // access a value
      createValue(...) -> V  // create a value

Structural Validation
=====================

Run ``.venv/bin/python3 utilities/run.py api_guidelines`` to validate all domain-specific pages.
The same check runs as part of ``pre_commit``.
Each domain-specific page is limited to 500 lines so it remains an at-a-glance overview rather than an API reference.

Anti-Patterns
=============

* **Prose:** API guidelines are not intended for extensive explanations.
  Whenever possible, express concepts through type lists and pattern blocks.

* **Exhaustive Lists:** The goal is not to document every API member.
  Capture the patterns that define the domain.
  Minor exceptions and isolated cases can be omitted.

* **Non-semantic Core Content:** Do not place type inventories, member patterns, naming examples, or API summaries in
  **Core Semantics**. Keep them in type and pattern sections. The only exception is a compact **Special Naming Rules**
  subsection when the relationship between public names cannot be expressed clearly by the later patterns.

* **Instructions:** Avoid sentences such as "Use ...", "Don't ...", or "Avoid ...".

  These guidelines work through examples and patterns rather than direct instructions.
  A developer should be able to skim the document, identify the relevant pattern, and naturally arrive at the expected naming and behavior.

Related Guidelines
==================

* Read :doc:`rst_style` to learn how to format documentation pages correctly, choose appropriate heading levels, and create links to code symbols.
* Read :doc:`writing_style` for the tone, vocabulary, and writing style used throughout the documentation.
