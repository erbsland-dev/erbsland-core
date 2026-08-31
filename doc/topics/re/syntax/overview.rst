.. index::
    !single: Syntax
    single: Syntax Overview

***************
Syntax Overview
***************

This chapter gives you a focused overview of the regular expression syntax supported by this library.
Whenever you want to dive deeper, each item links to the relevant detail sections so you can explore the topics at your
own pace.

You will notice that this syntax is intentionally strict: only explicitly supported escape sequences are accepted.
Unknown sequences produce parser errors, which helps you spot mistakes early and keeps your patterns predictable and
safe.

Quoting
=======

The following escape sequences let you insert literal characters that would otherwise have a special meaning in pattern
syntax.
Use them whenever you want the regex engine to treat these characters exactly as written.

.. list-table::
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   .. grid:: 3
                :margin: 0

                .. grid-item::
                    :esc_code:`\\`

                .. grid-item::
                    :esc_code:`\.`

                .. grid-item::
                    :esc_code:`\"`

                .. grid-item::
                    :esc_code:`\'`

                .. grid-item::
                    :esc_code:`#`

                .. grid-item::
                    :esc_code:`< >`

                .. grid-item::
                    :esc_code:`+`

                .. grid-item::
                    :esc_code:`*`

                .. grid-item::
                    :esc_code:`?`

                .. grid-item::
                    :esc_code:`{`

                .. grid-item::
                    :esc_code:`}`

                .. grid-item::
                    :esc_code:`$`

                .. grid-item::
                    :esc_code:`\^`

                .. grid-item::
                    :esc_code:`(`

                .. grid-item::
                    :esc_code:`|`

                .. grid-item::
                    :esc_code:`)`

                .. grid-item::
                    :esc_code:`[`

                .. grid-item::
                    :esc_code:`]`

        -   These escape sequences produce the corresponding literal character [#q1]_.

.. rubric:: Legacy and Compatibility Expressions

The following constructs are accepted for compatibility with older syntaxes.
For new code, prefer the recommended approach to keep your patterns clean and explicit.

.. list-table::
    :class: expressions
    :widths: 20 60 20
    :width: 100%
    :header-rows: 1

    *   -   Expression
        -   Details
        -   Please Use
    *   -   :esc_code:`Q`…:esc_code:`E`
        -   Treats all characters inside the ``Q…E`` block as literal.
            Even backslashes lose their special meaning until ``E`` is encountered.
        -   Escaping via API

.. button-ref:: quoting
    :ref-type: doc
    :color: primary
    :outline:
    :expand:
    :class: sd-fs-6 sd-font-weight-bold sd-p-2 sd-mb-4

    See chapter "Quoting" for all details →


Special Characters
==================

The escape sequences below let you insert common control characters and Unicode code points directly into your patterns.
Using them keeps your regular expressions both readable and intentional, especially when working with non-printable or
invisible characters.

.. list-table::
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   :esc_code:`n`
        -   Newline character :unicode:`a`.
    *   -   :esc_code:`r`
        -   Carriage return character :unicode:`d`.
    *   -   :esc_code:`t`
        -   Horizontal tab character :unicode:`9`.
    *   -   :esc_code:`u<hhhh>`
        -   Unicode code point given as a *four-digit* hexadecimal value ``hhhh`` [#u1]_.
    *   -   :esc_code:`u{<hh…>}`
        -   Unicode code point given as a hexadecimal value of one to eight digits ``hh…`` [#u1]_.

.. rubric:: Legacy and Compatibility Expressions

These expressions are supported for compatibility with older regex syntaxes.
For clarity and consistency, you should prefer the recommended forms whenever possible.

.. list-table::
    :class: expressions
    :widths: 20 60 20
    :width: 100%
    :header-rows: 1

    *   -   Expression
        -   Details
        -   Please Use
    *   -   :esc_code:`a`
        -   Alarm (BEL) character :unicode:`7`.
        -   :esc_code:`u{7}`
    *   -   :esc_code:`c<X>`
        -   ASCII control character from :unicode:`1` to :unicode:`1f`.
            ``X`` must be a single ASCII letter and is interpreted case-insensitively.
        -   :esc_code:`u{<hh>}`
    *   -   :esc_code:`e`
        -   Escape character :unicode:`1b`.
        -   :esc_code:`u{1b}`
    *   -   :esc_code:`f`
        -   Form feed character :unicode:`c`.
        -   :esc_code:`u{c}`
    *   -   :esc_code:`N`
        -   Matches any character *except* the newline.
            Useful in legacy patterns but directly equivalent to a simple negated class.
        -   :expression:`[^\\n]`
    *   -   :esc_code:`o{<ddd…>}`
        -   Character with an octal code ``ddd…`` [#o1]_.
            Octal escape sequences are still parsed but can be confusing in modern Unicode patterns.
        -   :esc_code:`u{<hh…>}`
    *   -   :esc_code:`x<hh>`
        -   Character with a two-digit hexadecimal code ``hh`` [#x1]_.
            This form cannot express code points above ``FF``.
        -   :esc_code:`u{<hh…>}`
    *   -   :esc_code:`x{<hh…>}`
        -   Character with a hexadecimal code ``hh…`` [#x1]_.
            Supports more digits than the compact ``x<hh>`` form, but still superseded by ``u{}``.
        -   :esc_code:`u{<hh…>}`
    *   -   :esc_code:`U<hhhhhhhh>`
        -   Unicode code point given as an *eight-digit* hexadecimal number ``hhhhhhhh`` [#u1]_.
            Works, but is unnecessarily rigid.
        -   :esc_code:`u{<hh…>}`

.. button-ref:: special-characters
    :ref-type: doc
    :color: primary
    :outline:
    :expand:
    :class: sd-fs-6 sd-font-weight-bold sd-p-2 sd-mb-4

    See chapter "Special Characters" for all details →


Character Types
===============

Character type escapes let you quickly match common categories of characters without writing long character classes.
They are especially helpful when working with Unicode text, where “letters” and “digits” extend far beyond the ASCII
ranges.

.. list-table::
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   :expression:`\.`
        -   Matches any character *except* the newline (:unicode:`a`) [#d1]_.
            This is your general-purpose wildcard.
    *   -   :esc_code:`d`
        -   Matches any Unicode digit character [#d2]_.
            Equivalent to :esc_code:`p{Nd}`.
    *   -   :esc_code:`D`
        -   Matches anything *except* Unicode digit characters [#d2]_.
            Equivalent to :esc_code:`P{Nd}`.
    *   -   :esc_code:`s`
        -   Matches any Unicode space separator character [#s1]_.
            Equivalent to :esc_code:`p{Zs}`.
    *   -   :esc_code:`S`
        -   Matches anything *except* Unicode space characters [#s1]_.
            Equivalent to :esc_code:`P{Zs}`.
    *   -   :esc_code:`w`
        -   Matches any “word” character: all Unicode letters, all Unicode digits, and the underscore [#w1]_.
            Equivalent to :expression:`[\\p{Nd}\\p{L}_]`.
    *   -   :esc_code:`W`
        -   Matches anything *except* “word” characters [#w1]_.
            Equivalent to :expression:`[^\\p{Nd}\\p{L}_]`.
    *   -   :esc_code:`p{<category>}`
        -   Matches all characters belonging to a specific Unicode category.
            This is ideal when you want precision and full Unicode awareness.
    *   -   :esc_code:`P{<category>}`
        -   Matches all characters *not* belonging to the given Unicode category.

.. rubric:: Legacy and Compatibility Expressions

These escape sequences stem from older regex engines such as PCRE.
They are still recognized, but they represent fixed, narrow character sets and should not be used in modern
Unicode-aware patterns.

.. list-table::
    :class: expressions
    :widths: 20 60 20
    :width: 100%
    :header-rows: 1

    *   -   Expression
        -   Details
        -   Please Use
    *   -   :esc_code:`h` :esc_code:`H`
        -   Matches a small, predefined set of “horizontal whitespace” characters from PCRE.
            These sets are not Unicode-complete.
        -   :expression:`[…]`
    *   -   :esc_code:`v` :esc_code:`V`
        -   Matches a small, predefined set of “vertical whitespace” characters from PCRE.
            Also not Unicode-complete and best avoided in new patterns.
        -   :expression:`[…]`

.. button-ref:: character-types
    :ref-type: doc
    :color: primary
    :outline:
    :expand:
    :class: sd-fs-6 sd-font-weight-bold sd-p-2 sd-mb-4

    See chapter "Character Types" for all details →


Character Classes
=================

Character classes give you fine-grained control over which characters your pattern should match.
They are one of the most powerful building blocks of regular expressions, especially when you want to define your own
sets rather than rely on predefined categories.

.. list-table::
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   :expression:`[...]`
        -   A *positive* character class.
            It matches **any** character listed inside the brackets.
    *   -   :expression:`[^...]`
        -   A *negative* character class.
            It matches **any** character *not* listed inside the brackets.

.. rubric:: Legacy and Compatibility Expressions

These constructs are recognized for compatibility with older regex syntaxes.
While they may look familiar if you have used POSIX or PCRE in the past, the modern Unicode-aware forms are more
expressive and easier to read.

.. list-table::
    :class: expressions
    :widths: 20 60 20
    :width: 100%
    :header-rows: 1

    *   -   Expression
        -   Details
        -   Please Use
    *   -   :expression:`[\\Q...\\E]`
        -   A quoted literal inside a character class.
            This often obscures intent and is unnecessary in Unicode-aware patterns.
        -   Avoid this.
    *   -   :expression:`[[:<name>:]]`
        -   POSITIVE POSIX named set (e.g. ``alpha``, ``digit``).
            Works, but comes from a legacy alphabetic system that predates modern Unicode categories.
        -   :esc_code:`p{<name>}`
    *   -   :expression:`[[:^<name>:]]`
        -   NEGATIVE POSIX named set.
            Matches everything *not* in the POSIX set.
        -   :esc_code:`P{<name>}`
    *   -   :expression:`[[:...:][:...:]]`
        -   Union of two or more POSIX named sets.
            While functional, it is harder to maintain and less explicit than Unicode category classes.
        -   :expression:`[\\p{...}\\p{...}]`

.. button-ref:: character-classes
    :ref-type: doc
    :color: primary
    :outline:
    :expand:
    :class: sd-fs-6 sd-font-weight-bold sd-p-2 sd-mb-4

    See chapter "Character Classes" for all details →


Quantifiers
===========

Quantifiers let you control **how many times** a preceding element may occur.
Each quantifier comes in three variants:

* **Greedy** – tries to match as much as possible.
* **Lazy** – tries to match as little as possible.
* **Possessive** – matches as much as possible and never backtracks.

Use these intentionally to make your patterns both fast and predictable.

.. list-table::
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   :expression:`?`
        -   Match 0 or 1 occurrence (greedy).
    *   -   :expression:`?+`
        -   Match 0 or 1 occurrence (possessive).
    *   -   :expression:`??`
        -   Match 0 or 1 occurrence (lazy).
    *   -   :expression:`*`
        -   Match 0 or more occurrences (greedy).
    *   -   :expression:`*+`
        -   Match 0 or more occurrences (possessive).
    *   -   :expression:`*?`
        -   Match 0 or more occurrences (lazy).
    *   -   :expression:`+`
        -   Match 1 or more occurrences (greedy).
    *   -   :expression:`++`
        -   Match 1 or more occurrences (possessive).
    *   -   :expression:`+?`
        -   Match 1 or more occurrences (lazy).
    *   -   :expression:`{<n>}`
        -   Match exactly *n* occurrences.
    *   -   :expression:`{<n>,<m>}`
        -   Match at least *n* and at most *m* occurrences (greedy).
    *   -   :expression:`{<n>,<m>}+`
        -   Match at least *n* and at most *m* occurrences (possessive).
    *   -   :expression:`{<n>,<m>}?`
        -   Match at least *n* and at most *m* occurrences (lazy).
    *   -   :expression:`{<n>,}`
        -   Match at least *n* occurrences (greedy).
    *   -   :expression:`{<n>,}+`
        -   Match at least *n* occurrences (possessive).
    *   -   :expression:`{<n>,}?`
        -   Match at least *n* occurrences (lazy).

.. button-ref:: quantifiers
    :ref-type: doc
    :color: primary
    :outline:
    :expand:
    :class: sd-fs-6 sd-font-weight-bold sd-p-2 sd-mb-4

    See chapter "Quantifiers" for all details →


Anchors
=======

Anchors match **positions** in the text rather than actual characters.
Use them to precisely define where a match may occur.

.. list-table::
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   :esc_code:`A`
        -   Match only at the start of the entire text.
    *   -   :esc_code:`b`
        -   Match at a word boundary (between a word and a non-word character).
    *   -   :esc_code:`B`
        -   Match anywhere *except* at a word boundary.
    *   -   :esc_code:`Z`
        -   Match at the end of the text [#Z1]_.
            Note that this does *not* match before a trailing newline.
    *   -   :expression:`^`
        -   Match at the start of the text, or after a newline when multi-line mode is active.
    *   -   :expression:`$`
        -   Match at the end of the text, or before a newline when multi-line mode is active.

.. rubric:: Legacy and Compatibility Expressions

The following anchor is kept for compatibility, but its behavior duplicates :esc_code:`Z`.

.. list-table::
    :class: expressions
    :widths: 20 60 20
    :width: 100%
    :header-rows: 1

    *   -   Expression
        -   Details
        -   Please Use
    *   -   :esc_code:`z`
        -   Behaves the same as :esc_code:`Z` and also does *not* match before the final newline.
        -   :esc_code:`Z`

.. button-ref:: anchors
    :ref-type: doc
    :color: primary
    :outline:
    :expand:
    :class: sd-fs-6 sd-font-weight-bold sd-p-2 sd-mb-4

    See chapter "Anchors" for all details →


Alternatives
============

Alternatives allow you to express “match one of several options” directly inside your pattern.
They are particularly useful when several text variants should be treated equally.

.. list-table::
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   :expression:`A|B|C`
        -   Match any of the alternatives ``A``, ``B`` or ``C``.
            This form creates a capturing group unless you wrap it in a non-capturing group.
    *   -   :expression:`(?:A|B|C)`
        -   Match any of the given alternatives without creating a capturing group.
            Prefer this form when you do not need to extract the matched part.

.. button-ref:: alternatives
    :ref-type: doc
    :color: primary
    :outline:
    :expand:
    :class: sd-fs-6 sd-font-weight-bold sd-p-2 sd-mb-4

    See chapter "Alternatives" for all details →


Groups
======

Groups give structure to your regular expressions.
They help you control scope, apply quantifiers to sequences, extract matched parts, or influence how the regex engine
performs backtracking.

.. list-table::
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   :expression:`(...)`
        -   A capturing group.
            Use this when you want to extract or refer to the matched content.
    *   -   :expression:`(?<<name>>...)`
        -   A named capturing group.
            This makes your patterns more readable and avoids relying on numeric group indices.
    *   -   :expression:`(?:...)`
        -   A non-capturing group.
            Ideal when grouping is needed only for precedence or quantifiers.
    *   -   :expression:`(?>...)`
        -   An atomic group.
            The content is matched without backtracking, which can significantly improve performance
            in specific situations and prevents ambiguous matches.

.. rubric:: Legacy and Compatibility Expressions

The following group syntaxes exist for compatibility with older regex engines.
Prefer the modern, :expression:`(?<<name>>...)` and verbose-mode comments whenever possible.

.. list-table::
    :class: expressions
    :widths: 20 60 20
    :width: 100%
    :header-rows: 1

    *   -   Expression
        -   Details
        -   Please Use
    *   -   :expression:`(?'<name>'...)`
        -   A legacy form of named capturing group.
        -   :expression:`(?<<name>>...)`
    *   -   :expression:`(?P<<name>>...)`
        -   Another legacy syntax for named capturing groups.
        -   :expression:`(?<<name>>...)`
    *   -   :expression:`(?#...)`
        -   An inline comment.
            It cannot be nested, but literal :esc_code:`)` is allowed inside.
            Prefer verbose mode for more readable, maintainable patterns.
        -   Verbose Mode

.. button-ref:: groups
    :ref-type: doc
    :color: primary
    :outline:
    :expand:
    :class: sd-fs-6 sd-font-weight-bold sd-p-2 sd-mb-4

    See chapter "Groups" for all details →


Modes and Flags
===============

Flags let you change how the regular expression engine behaves.
You can enable or disable them **globally** (only allowed at the very start of the pattern [#F1]_) or **locally** for a
specific group.
Local flags apply only inside that group and restore the previous settings afterward.

.. list-table::
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   :expression:`(?i)`
        -   Enable case-insensitive matching (global only).
    *   -   :expression:`(?m)`
        -   Enable multi-line mode (``^`` and ``$`` match at line boundaries; global only).
    *   -   :expression:`(?s)`
        -   Enable dot-all mode (``.`` also matches newlines; global only).
    *   -   :expression:`(?x)`
        -   Enable verbose mode (whitespace and comments ignored; global only).
    *   -   :expression:`(?a)`
        -   Enable ASCII mode (character escapes follow ASCII semantics; global only).
    *   -   :expression:`(?u)`
        -   Enable Unicode mode and disable ASCII mode (global only).
    *   -   :expression:`(?<flags>)`
        -   Enable multiple flags at once (global only).
            Example: ``(?ims)`` activates ``i``, ``m`` and ``s``.
    *   -   :expression:`(?-<flags>)`
        -   Disable the listed flags (``i``, ``m``, ``s`` or ``x``) globally.
            Example: ``(?-i)`` turns off case-insensitive matching.
    *   -   :expression:`(?<flags>:...)`
        -   Apply or remove flags *only inside this group*.
            Example: ``(?i:abc)`` matches ``abc`` case-insensitively, but the rest of the pattern
            is unaffected.
            Flags can also be mixed: ``(?im-s:...)`` enables ``i`` and ``m`` and disables ``s`` for
            the enclosed group.

.. button-ref:: modes-and-flags
    :ref-type: doc
    :color: primary
    :outline:
    :expand:
    :class: sd-fs-6 sd-font-weight-bold sd-p-2 sd-mb-4

    See chapter "Modes and Flags" for all details →


Unsupported Expressions
=======================

The expressions listed below are *intentionally not supported* by this library.
Most of them introduce ambiguity, reduce safety, or add complexity that conflicts with the design goals of a predictable
and Unicode-aware regex engine.
When encountering one of these constructs, the parser will raise an error.

.. list-table::
    :class: expressions
    :widths: 20 80
    :width: 100%
    :header-rows: 1

    *   -   Expression
        -   Reason
    *   -   :esc_code:`0<dd>`
        -   Zero-prefixed octal escapes conflict with backreferences and replacement patterns.
            Using :esc_code:`0` produces a parser error.
    *   -   :esc_code:`<ddd>`
        -   Ambiguous octal syntax that conflicts with backreferences and replacement patterns.
            Using :esc_code:`<ddd>` produces a parser error.
    *   -   :esc_code:`C`
        -   Unsafe: would break proper UTF-8 parsing and could lead to invalid byte sequences.
    *   -   :esc_code:`R`
        -   Redundant. Because the engine can normalize CRLF to LF automatically, :esc_code:`R`
            would collapse to :esc_code:`n`. To avoid surprising behavior, it is disabled.
    *   -   :esc_code:`X`
        -   Too similar to :esc_code:`x` and easily confused with it.
            Unicode category unions are already supported via character classes like
            :expression:`[\\p{L}\\p{N}]`.
    *   -   :esc_code:`<n>` :esc_code:`g<n>` :esc_code:`g{<n>}` :esc_code:`g{-<n>}` :esc_code:`k<<name>>`
            :esc_code:`k'<name>'` :esc_code:`g{<name>}` :esc_code:`k{<name>}` :expression:`(?P=<name>)`
        -   Backreferences are not supported in this version of the library.
    *   -   :expression:`[[:^...:][:^...:]]`
        -   Combining multiple negative POSIX sets, or mixing negative and positive sets, adds
            unnecessary complexity to this compatibility feature.
    *   -   :esc_code:`G`
        -   Not meaningful for this implementation; omitted for clarity.
    *   -   :esc_code:`K`
        -   Extremely rare in practice and not worth the added complexity.
    *   -   :expression:`(?|...)`
        -   Introduces unnecessary complexity; can be replaced with straightforward program logic.
    *   -   :expression:`(?C)` :expression:`(?C<n>)`
        -   These debugging constructs are not needed; the library exposes clearer ways to inspect its behavior.
    *   -   :expression:`(?J)`
        -   Allows duplicate group names. This breaks clarity and maintainability.
    *   -   :expression:`(?U)`
        -   Adds unnecessary mode complexity and easily confused with :expression:`(?u)`.
    *   -   :expression:`(*...)`
        -   Parser tuning directives are provided via the API, not as inline syntax.
    *   -   :expression:`(?=...)` :expression:`(?!...)` :expression:`(?<=...)` :expression:`(?<!...)`
        -   Lookahead and lookbehind assertions are not supported in this version.
    *   -   :expression:`(?R)` :expression:`(?<n>)` :expression:`(?+<n>)` :expression:`(?-<n>)`
            :expression:`(?&<name>)` :expression:`(?P><name>)` :esc_code:`g<<name>>`
            :esc_code:`g'<name>'` :esc_code:`g<n>` :esc_code:`g'n'`
        -   These recursion and subroutine constructs cannot be implemented safely.
    *   -   :expression:`(?(<cond>)<yes>)` :expression:`(?(<cond>)<yes>|<no>)`
        -   Conditional patterns are not supported in this version of the library.

.. button-ref:: unsupported-expressions
    :ref-type: doc
    :color: primary
    :outline:
    :expand:
    :class: sd-fs-6 sd-font-weight-bold sd-p-2 sd-mb-4

    See chapter "Unsupported Expressions" for all details →


.. rubric:: Footnotes

.. [#q1] The Erbsland Core regular expression syntax accepts only *documented escape sequences*.
         Any escape sequence not listed as supported will result in a parser error.

.. [#u1] Hexadecimal letters are case-insensitive.
         For the dynamic form :esc_code:`u{<hh…>}`, up to eight hexadecimal digits are allowed.
         Invalid Unicode code points produce a parser error.

.. [#o1] Octal escapes may contain up to 11 digits.
         Invalid Unicode code points produce a parser error.

.. [#x1] Hexadecimal letters are case-insensitive.
         For the extended form :esc_code:`x{<hh…>}`, up to eight hexadecimal digits are allowed.
         Invalid Unicode code points produce a parser error.

.. [#d1] In dot-all mode (enabled via the ``s`` flag or ``Flag::DotAll``),
         :expression:`\.` matches *all* Unicode characters, including line breaks.

.. [#d2] In ASCII mode (enabled via the ``a`` flag or ``Flag::Ascii``),
         :esc_code:`d` and :esc_code:`D` are limited to the ASCII digits ``0–9``.

.. [#s1] In dot-all mode (``s`` flag or ``Flag::DotAll``),
         :esc_code:`s` and :esc_code:`S` also include or ignore line-break characters.
         In ASCII mode (``a`` flag or ``Flag::Ascii``), these escapes are restricted to
         :unicode:`9`, :unicode:`b`, :unicode:`c`, :unicode:`d` and :unicode:`20`.
         With dot-all mode active, they additionally include :unicode:`a`.
         The tab character (:unicode:`9`) is included in all space variants. This behaviour
         is different from other implementation that do not include it in its Unicode variants.

.. [#w1] In ASCII mode (``a`` flag or ``Flag::Ascii``),
         :esc_code:`w` and :esc_code:`W` are limited to the ASCII characters
         ``A–Z``, ``a–z``, ``0–9`` and ``_``.

.. [#Z1] The :esc_code:`Z` anchor does *not* match just before a trailing newline at the end of the text.

.. [#F1] Mode-changing expressions such as :expression:`(?i)` or :expression:`(?msx)`
         are only valid at the very beginning of a regular expression.
