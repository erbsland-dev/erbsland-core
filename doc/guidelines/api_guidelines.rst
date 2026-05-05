***************************
How to Write API Guidelines
***************************

Why API Guidelines
==================

API guidelines enforce a consistent vocabulary across the large API surface of this library.
All guidelines are based on the core principles described in :doc:`api/common`.
Each guideline describes the API of a specific domain in its most compact and condensed form.

When the API is extended, these guidelines provide an immediate overview of the domain and help prevent the API from diverging.
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
The following structure also uses ``❮...❯`` for notes and explanations. This notation is only used in this template and must not appear in an actual guideline page.

.. code-block:: rst

    ******************************
    ❮Domain❯ Domain API Guidelines
    ******************************

    These guidelines extend the Common API Guidelines for ❮...❯.

    The purpose of this document is to define a base naming vocabulary for ❮domain❯ APIs.
    It is intentionally plain, technical, and list-based to provide a quick overview of method names and their usage patterns.
    If new vocabulary is introduced, update this page to provide a reference for future extensions.

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

* **Core Semantics:** This *optional* initial section defines the fundamental principles of the domain in an extremely compact and technical form.
  These principles are organized into subsections with descriptive names that support quick skimming.
  Less is better. Useful subsections include:

  - **Vocabulary:** A compact definition list.
    A vocabulary block only makes sense when terms are ambiguous and can have multiple meanings within the domain.
    This is not a thesaurus, index, or encyclopedia. Well-known and unambiguous terms do not belong in such a list.

  - **Special Naming Rules:** Short prose or illustrative text blocks.
    A good example from the text domain is explaining the relationship between ``String``, ``U(8/16/32)String``,
    ``StringView``, ``U(8/16/32)StringView``, and ``U(8/16)StringCharView``.
    A simple type list would not adequately explain the API surface and could lead to confusion.
    By explaining how these names relate, the pattern sections can focus on ``String`` and ``StringView``.

  - **Special Placeholders:** Used when the placeholders defined in :doc:`api/common` are not sufficient and introducing additional placeholders improves the readability of the following patterns.

  - **Important Behavior:** Used when behavior cannot be fully derived from naming and common expectations.
    For example, whether a begin-end range includes or excludes the last element, if that behavior differs from what developers would typically expect.

* **Primary Types:** This mandatory section contains the primary types of the domain.
  These are the types users should know first and that best represent the domain.

  The section consists of a single ``.. code-block:: text`` without surrounding prose.
  Each line contains one or more type names, followed by two spaces and ``//``, and then a short description.
  Descriptions should remain concise and generally stay below 120 columns.

  .. code-block:: text

      ❮Type❯  // ❮description❯

  Closely related types may be grouped on a single line:

  .. code-block:: text

      ❮Type1❯, ❮Type2❯, ❮Type3❯  // ❮description❯

  No more than five types should be grouped together.

  Groups may be introduced by an empty line followed by a group title:

  .. code-block:: text

      // ❮group title❯

* **Secondary Types:** This *optional* section contains the remaining types of the domain.

  The section consists of a single ``.. code-block:: text`` without surrounding prose.
  For larger domains, multiple sections may be used instead of a single **Secondary Types** section.
  In that case, replace the word *Secondary* with a more meaningful group name.

* **Pattern Blocks:** The remainder of the document consists of one or more pattern blocks.

  Each pattern block describes the API patterns used within the domain.
  Pattern blocks illustrate naming and behavior conventions rather than providing an exhaustive API reference.

  Each section consists of a single ``.. code-block:: text`` without surrounding prose:

  .. code-block:: text

      o.remove(range) -> T&  // remove a character-based range in-place.
      o.append(character/text[, count]) -> T&  // append code point(s) or text.
      o.allOf/anyOf/noneOf(function) -> bool  // predicate tests over all elements.
      o.write(character/text)  // write text without adding a line ending.
      T::zero() -> T  // the zero value.
      T::fromInteger(v, format) -> T  // create from an integer.

  Patterns are usually built around an object instance (``o.call()``) or a static type (``T::call()``), where
  ``o`` and ``T`` always represent the object and type being described.

  The arrow ``->`` indicates a return value.

  Each pattern should fit on a single line and end with two spaces followed by ``//`` and a concise description.
  The description should answer the question:

  "If I add a method that matches this pattern, what behavior will users expect from it?"

  Variants can be compacted using ``/``, ``[Optional]``, and placeholders to keep pattern blocks concise and readable.

Anti-Patterns
=============

* **Prose:** API guidelines are not intended for extensive explanations.
  Whenever possible, express concepts through type lists and pattern blocks.

* **Exhaustive Lists:** The goal is not to document every API member.
  Capture the patterns that define the domain.
  Minor exceptions and isolated cases can be omitted.

* **Instructions:** Avoid sentences such as "Use ...", "Don't ...", or "Avoid ...".

  These guidelines work through examples and patterns rather than direct instructions.
  A developer should be able to skim the document, identify the relevant pattern, and naturally arrive at the expected naming and behavior.

Related Guidelines
==================

* Read :doc:`rst_style` to learn how to format documentation pages correctly, choose appropriate heading levels, and create links to code symbols.
* Read :doc:`writing_style` for the tone, vocabulary, and writing style used throughout the documentation.