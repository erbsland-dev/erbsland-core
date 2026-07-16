.. index::
    single: Syntax; character types
    single: Character types
    single: Escape sequences; character types
    single: Unicode; categories

***************
Character Types
***************

Character type escapes match common character categories without requiring you to write
explicit character classes. Like character classes, they always match **exactly one character**.

Use character type escapes for readability and intent, especially when working with Unicode
text or when the exact set of characters is secondary to their semantic meaning.

:expression:`.`
===============

Matches any single character.

By default, this escape matches any character **except** the line feed character
(:esc_code:`n`, LF, :unicode:`A`).

When the :cpp:any:`Flag::DotAll <erbsland::re::Flag::DotAll>` flag is enabled, the dot also
matches line feed characters.

You can enable :cpp:any:`DotAll <erbsland::re::Flag::DotAll>` mode either programmatically or inline using :expression:`(?s)`.

:esc_code:`d`
=============

Matches a digit character.

The exact set depends on the active ASCII or Unicode mode:

* In Unicode mode (default), it matches Unicode decimal digits
  (equivalent to :expression:`\\p{Nd}`).
* In ASCII mode, it matches only ASCII digits
  (equivalent to :expression:`[0-9]`).

Enable ASCII mode programmatically via
:cpp:any:`Flag::Ascii <erbsland::re::Flag::Ascii>` or inline using :expression:`(?a)`.

:esc_code:`D`
=============

Matches any character that is **not** a digit as defined by :esc_code:`d`.

This is the negated form of :esc_code:`d`.

:esc_code:`s`
=============

Matches a whitespace character.

The exact set depends on the active ASCII or Unicode mode.

When the :cpp:any:`Flag::DotAll <erbsland::re::Flag::DotAll>` flag is enabled, the whitespace
set is extended to include the line feed character. This keeps the behavior of
:esc_code:`s` consistent with :expression:`.` when matching across lines.

You can enable :cpp:any:`DotAll <erbsland::re::Flag::DotAll>` mode programmatically or inline using :expression:`(?s)`.
ASCII mode can be enabled via :expression:`(?a)` or
:cpp:any:`Flag::Ascii <erbsland::re::Flag::Ascii>`.

:esc_code:`S`
=============

Matches any character that is **not** whitespace as defined by :esc_code:`s`.

This is the negated form of :esc_code:`s`.

:esc_code:`w`
=============

Matches a *word* character.

The exact definition depends on the active ASCII or Unicode mode:

* In Unicode mode (default), it matches Unicode letters and digits, plus underscore.
* In ASCII mode, it matches only ASCII letters, digits, and underscore.

This definition is consistent with word-boundary handling via :expression:`\\b`.

Enable ASCII mode programmatically using
:cpp:any:`Flag::Ascii <erbsland::re::Flag::Ascii>` or inline with :expression:`(?a)`.

:esc_code:`W`
=============

Matches any character that is **not** a word character as defined by :esc_code:`w`.

This is the negated form of :esc_code:`w`.

:esc_code:`p{<category>}`
=========================

Matches a character belonging to the given Unicode *general category*.

The category name is normalized before matching:

* Matching is **case-insensitive**.
* Underscores are ignored.
* Only category name characters (``a-z``, ``A-Z``, ``_``) are accepted.
* Unknown or invalid category names raise a parse error.

Examples:

* :expression:`\\p{Lu}` matches any uppercase letter.
* :expression:`\\p{Uppercase_Letter}` is accepted as well.

Unicode category matching is **not affected** by the following flags:

* :cpp:any:`Flag::DotAll <erbsland::re::Flag::DotAll>`
* :cpp:any:`Flag::IgnoreCase <erbsland::re::Flag::IgnoreCase>`
* :cpp:any:`Flag::Ascii <erbsland::re::Flag::Ascii>`

For example, even in case-insensitive mode,
:expression:`\\p{UppercaseLetter}` still matches **only uppercase letters**.

:esc_code:`P{<category>}`
=========================

Matches any character that is **not** in the given Unicode general category.

This is the negated form of :esc_code:`p{<category>}`.

Supported Unicode Categories
============================

.. list-table::
    :header-rows: 1
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   Short Name
        -   Long Name
    *   -   :expression:`C`
        -   :expression:`Other`
    *   -   :expression:`L`
        -   :expression:`Letter`
    *   -   :expression:`M`
        -   :expression:`Mark`
    *   -   :expression:`N`
        -   :expression:`Number`
    *   -   :expression:`P`
        -   :expression:`Punctuation`
    *   -   :expression:`S`
        -   :expression:`Symbol`
    *   -   :expression:`Z`
        -   :expression:`Separator`
    *   -   :expression:`Cc`
        -   :expression:`Control`
    *   -   :expression:`Cf`
        -   :expression:`Format`
    *   -   :expression:`Cn`
        -   :expression:`Unassigned`
    *   -   :expression:`Co`
        -   :expression:`PrivateUse`
    *   -   :expression:`Cs`
        -   :expression:`Surrogate`
    *   -   :expression:`Ll`
        -   :expression:`LowercaseLetter`
    *   -   :expression:`Lm`
        -   :expression:`ModifierLetter`
    *   -   :expression:`Lo`
        -   :expression:`OtherLetter`
    *   -   :expression:`Lt`
        -   :expression:`TitlecaseLetter`
    *   -   :expression:`Lu`
        -   :expression:`UppercaseLetter`
    *   -   :expression:`Mc`
        -   :expression:`SpacingMark`
    *   -   :expression:`Me`
        -   :expression:`EnclosingMark`
    *   -   :expression:`Mn`
        -   :expression:`NonspacingMark`
    *   -   :expression:`Nd`
        -   :expression:`DecimalNumber`
    *   -   :expression:`Nl`
        -   :expression:`LetterNumber`
    *   -   :expression:`No`
        -   :expression:`OtherNumber`
    *   -   :expression:`Pc`
        -   :expression:`ConnectorPunctuation`
    *   -   :expression:`Pd`
        -   :expression:`DashPunctuation`
    *   -   :expression:`Pe`
        -   :expression:`ClosePunctuation`
    *   -   :expression:`Pf`
        -   :expression:`FinalPunctuation`
    *   -   :expression:`Pi`
        -   :expression:`InitialPunctuation`
    *   -   :expression:`Po`
        -   :expression:`OtherPunctuation`
    *   -   :expression:`Ps`
        -   :expression:`OpenPunctuation`
    *   -   :expression:`Sc`
        -   :expression:`CurrencySymbol`
    *   -   :expression:`Sk`
        -   :expression:`ModifierSymbol`
    *   -   :expression:`Sm`
        -   :expression:`MathSymbol`
    *   -   :expression:`So`
        -   :expression:`OtherSymbol`
    *   -   :expression:`Zl`
        -   :expression:`LineSeparator`
    *   -   :expression:`Zp`
        -   :expression:`ParagraphSeparator`
    *   -   :expression:`Zs`
        -   :expression:`SpaceSeparator`

Legacy and Compatibility Expressions
====================================

The following escapes exist for compatibility with other regular expression engines.
They are supported as legacy syntax and should not be used in new patterns unless
compatibility is required.

Prefer using :esc_code:`s` / :esc_code:`S` and explicit Unicode categories via
:esc_code:`p{<category>}` instead.

:esc_code:`h` / :esc_code:`H`
-----------------------------

Matches horizontal whitespace (or its negation).

This escape sequence exists for compatibility and may be disabled via the feature set.

:esc_code:`v` / :esc_code:`V`
-----------------------------

Matches vertical whitespace (or its negation).

This escape sequence exists for compatibility and may be disabled via the feature set.

Differences from Common Regex Engines
=====================================

If you are migrating from commonly used regular expression engines such as PCRE, PCRE2,
ECMAScript, RE2, or ``std::regex``, you may notice a few deliberate differences in how
character type escapes behave.

These differences are intentional and aim to make matching semantics **explicit,
predictable, and independent of context**, even when working with Unicode text.

Dot does not silently match newlines
------------------------------------

In many engines, the behavior of :expression:`.` varies subtly depending on configuration,
or differs between default modes.

In this engine:

* :expression:`.` **never** matches a line feed by default.
* Line feed is matched only when
  :cpp:any:`Flag::DotAll <erbsland::re::Flag::DotAll>` is explicitly enabled.

This strict separation avoids accidental cross-line matches and makes it immediately clear
when a pattern is intended to span multiple lines.

Whitespace handling is aligned with DotAll
------------------------------------------

When :cpp:any:`Flag::DotAll <erbsland::re::Flag::DotAll>` is enabled, the definition of
whitespace matched by :esc_code:`s` is extended to include the line feed character.

Some engines treat dot and whitespace independently, which can lead to inconsistent behavior
when switching between ``.`` and :esc_code:`s`. This engine keeps both in sync to avoid
surprising differences when refactoring patterns.

Unicode categories are not affected by flags
--------------------------------------------

Unicode category matching via :esc_code:`p{<category>}` and :esc_code:`P{<category>}` is
**completely independent** of matching flags such as:

* case-insensitive matching
* ASCII mode
* DotAll mode

For example, even when case-insensitive matching is enabled,
:expression:`\\p{UppercaseLetter}` still matches **only uppercase letters**.

In contrast, some engines implicitly fold or reinterpret categories depending on flags,
which can make patterns harder to reason about. This engine keeps Unicode semantics stable
and explicit.

Word characters are explicitly defined
--------------------------------------

The definition of word characters used by :esc_code:`w`, :esc_code:`W`, and word-boundary
anchors is tightly defined:

* In Unicode mode, word characters are Unicode letters and digits plus underscore.
* In ASCII mode, only ASCII letters, digits, and underscore are included.

Some engines expand word characters to include combining marks or additional categories.
This engine intentionally keeps the definition conservative to ensure predictable word
boundary behavior across scripts.

Legacy escapes are opt-in
-------------------------

Escapes such as :esc_code:`h`, :esc_code:`H`, :esc_code:`v`, and :esc_code:`V` exist solely
for compatibility with other engines.

They are clearly marked as legacy and may be disabled via the feature set. For new patterns,
prefer :esc_code:`s`, :esc_code:`S`, and explicit Unicode categories for clarity and
long-term stability.

Summary
-------

In short, this engine favors **stable Unicode semantics, explicit intent, and consistent
behavior across flags**.

