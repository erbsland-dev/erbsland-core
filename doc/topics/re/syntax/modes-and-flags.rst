.. index::
    single: Syntax; flags
    single: Syntax; modes
    single: Flags
    single: Modes
    single: Verbose mode

***************
Modes and Flags
***************

Flags control how the regex engine parses and matches a pattern. You can set flags either
programmatically via :cpp:any:`erbsland::re::Flags` or inline as part of the pattern.

Inline flags can apply in two different scopes:

* **Global flags** apply to the entire pattern and are only accepted at the very start.
* **Local flags** apply only within a specific group and must use group syntax.

This strict separation avoids ambiguous flag lifetime and makes the effective matching
rules explicit at every position in the pattern.

Standard Flags
==============

:expression:`(?i)`
------------------

Enables **case-insensitive** matching.

* Applies to literal characters and character classes.
* Unicode categories such as :expression:`\\p{Uppercase_Letter}` are **not** affected;
  they always keep their literal meaning (see :doc:`character-types`).

This form is accepted **only at the start** of the pattern.

.. code-block:: text
    :caption: Example (global case-insensitive match)

    (?i)error:\s+file\s+not\s+found

Use the grouped form :expression:`(?i:...)` to limit case-insensitive matching to a
specific part of the pattern.

:expression:`(?m)`
------------------

Enables **multi-line** mode.

* With this flag, :expression:`^` and :expression:`$` match the start and end of
  each line instead of the entire input.
* Line boundaries are based on line feeds. If you need Windows-style CRLF handling,
  enable :cpp:any:`Flag::CRLF <erbsland::re::Flag::CRLF>` programmatically.

This form is accepted **only at the start** of the pattern.

.. code-block:: text
    :caption: Example

    (?m)^ERROR:.*$

Use :expression:`(?m:...)` for a local override.

:expression:`(?s)`
------------------

Enables **dot-all** mode.

* :expression:`.` also matches line feeds (:esc_code:`n`).
* The whitespace class :esc_code:`s` also includes line feeds in this mode, keeping
  whitespace handling consistent with :expression:`.`.

This form is accepted **only at the start** of the pattern.

.. code-block:: text
    :caption: Example

    (?s)<tag>.*?</tag>

Use :expression:`(?s:...)` for a local override.

:expression:`(?x)`
------------------

Enables **verbose** mode.

* Whitespace is ignored while parsing the pattern.
* A :expression:`#` character starts a comment that runs until the end of the line.
* Inside character classes, whitespace and :expression:`#` retain their literal meaning.

Verbose mode is especially useful for long or complex patterns.

.. code-block:: text
    :caption: Example

    (?x)
    ^\s*             # optional leading whitespace
    ERROR            # literal keyword
    \s*:\s*
    (?i:file)        # case-insensitive match for "file"
    \s+not\s+found
    $

To match literal whitespace in verbose mode, place it inside a character class
(for example :expression:`[ ]`) or escape it.

Inline Flag Sets
================

:expression:`(?<flags>)`
------------------------

Enables multiple flags at once (**global only**).

* Allowed flags: :expression:`i`, :expression:`m`, :expression:`s`, :expression:`x`,
  :expression:`a`, :expression:`u`.
* This form is accepted **only at the very start** of the pattern.
* A maximum of 10 flag characters is allowed.

.. rubric:: Examples:

* :expression:`(?ims)` enables case-insensitive, multi-line, and dot-all mode.
* :expression:`(?u)` explicitly switches back to Unicode mode.

Local Flag Overrides
====================

:expression:`(?<flags>:...)`
----------------------------

Applies or removes flags **only within the group**.

* Flags not listed are inherited from the surrounding context.
* A single minus sign :expression:`-` switches from enabling to disabling flags.
* Only :expression:`i`, :expression:`m`, :expression:`s`, and :expression:`x` may be disabled.
* ASCII and Unicode mode are switched explicitly using :expression:`a` and :expression:`u`.

.. rubric:: Examples:

.. code-block:: text
    :caption: Case-insensitive match with a strict case-sensitive keyword

      (?i:error:\s+(?-i:FILE)\s+not\s+found)

.. code-block:: text
    :caption: Enable multiline mode only for a specific group

      header:(?m:^.+$)

.. code-block:: text
    :caption: Switch to ASCII semantics locally

      (?a:\w+)

Unsupported Standalone Flag Removal
===================================

Standalone flag removal without a group is **not supported**.

The following form is rejected:

* :expression:`(?-i)`

This is intentional. Without a group boundary, it would be unclear where the disabled
flag should stop applying.

Always use the grouped form instead:

* :expression:`(?-i:...)` disables a flag *only* within that group.

This design ensures that every flag change has a clear and explicit scope.

Legacy and Compatibility Flags
==============================

These flag toggles exist primarily for compatibility with engines that default to ASCII
or allow implicit mode switches. This engine is Unicode-first.

ASCII Mode
----------

:expression:`(?a)` enables ASCII mode and restricts character escapes such as
:esc_code:`d`, :esc_code:`s`, and :esc_code:`w` (and their negations) to ASCII semantics.

See :doc:`character-types` for the exact definitions.

Unicode Mode
------------

:expression:`(?u)` disables ASCII mode and returns to Unicode semantics.

This is the **only** supported way to leave ASCII mode. The parser rejects
:expression:`-a` and :expression:`-u` in flag lists to avoid ambiguous transitions.

Differences from Other Regex Engines
====================================

* Global inline flags are only accepted at the **start** of the pattern.
  Many engines allow them anywhere and apply them from that point onward.
* Flag changes must always be scoped explicitly using groups.
  Standalone removal like :expression:`(?-i)` is not accepted.
* ASCII and Unicode mode are switched explicitly using :expression:`(?a)` and
  :expression:`(?u)`, rather than implicit negation.

These rules favor **explicit scope, readability, and safe refactoring** over
implicit or positional flag behavior.
