.. index::
    single: Syntax; anchors
    single: Anchors
    single: Word boundary

*******
Anchors
*******

Anchors match *positions* in the input rather than characters. They are **zero-width assertions**:
they check where something is allowed to match, but they never consume input themselves.

Anchors are essential when you want to control *where* a pattern may appear, for example at the
start of the input, at line boundaries, or between word and non-word characters.

:expression:`\\A`
=================

Matches only at the **start of the input**.

This anchor is independent of multiline mode and always refers to the absolute beginning of the
input. It is equivalent to :expression:`^` when multiline mode is disabled, but remains unaffected
when multiline mode is enabled.

Use this anchor when you need a strict guarantee that matching begins at the very start.

:expression:`\\b`
=================

Matches a **word boundary**, that is, the position between a word character and a non-word character.

What counts as a word character depends on the active ASCII or Unicode mode (see :esc_code:`w`).
The start and end of the input are treated as non-word positions, which means :expression:`\\b`
can also match at the very beginning or end of the input if the adjacent character is a word
character.

This anchor is commonly used to ensure that identifiers, keywords, or tokens are matched as whole
words rather than as substrings.

:expression:`\\B`
=================

Matches a **non-word boundary**, meaning a position between two word characters or between two
non-word characters.

The same ASCII or Unicode rules apply as for :expression:`\\b`. This anchor is essentially the
logical opposite of :expression:`\\b` and is useful when you explicitly want to avoid word
boundaries.

:expression:`\\Z`
=================

Matches only at the **end of the input**.

This anchor does **not** match before a trailing newline. It always refers to the true end of the
input and is equivalent to :expression:`\\z` and :expression:`$` when multiline mode is disabled.

Use this anchor when matching must stop exactly at the input boundary.

:expression:`^`
===============

Matches the start of the input, or the start of a line when multiline mode is enabled.

You can enable multiline mode either programmatically via
:cpp:any:`Flag::Multiline <erbsland::re::Flag::Multiline>` or inline using :expression:`(?m)`.
In multiline mode, :expression:`^` matches immediately after a line feed (:esc_code:`n`) or at the
start of the input.

In single-line mode, :expression:`^` is equivalent to :expression:`\\A`.

:expression:`$`
===============

Matches the end of the input, or the end of a line when multiline mode is enabled.

Multiline mode can be enabled via
:cpp:any:`Flag::Multiline <erbsland::re::Flag::Multiline>` or inline using :expression:`(?m)`.
In multiline mode, :expression:`$` matches immediately before a line feed (:esc_code:`n`) or at the
end of the input.

In single-line mode, this anchor is equivalent to :expression:`\\Z`.

Legacy and Compatibility Anchors
================================

:expression:`\\z`
-----------------

Matches only at the **end of the input** (same behavior as :expression:`\\Z`).

This anchor exists solely for compatibility with other regular expression engines. It can be
disabled using
:cpp:any:`Feature::AnchorLowercaseZ <erbsland::re::Feature::AnchorLowercaseZ>`.

For new patterns, prefer :expression:`\\Z` for clarity and consistency.

Notes and Compatibility
=======================

* Anchors such as :expression:`\\b` and :expression:`\\B` are **not allowed inside character
  classes**. If you need conditional behavior, use grouping instead, for example
  :expression:`(?:[...]|\\b)`.
* Unlike PCRE, :expression:`\\Z` and :expression:`$` in single-line mode **do not** match before a
  trailing newline. They only match at the true end of the input.
* :expression:`\\b` is always interpreted as a word-boundary anchor. It is **not** an alias for the
  backspace character inside character classes. If you need a literal backspace, use
  :esc_code:`x08`.

