.. index::
    single: Syntax; quoting
    single: Quoting
    single: Escape sequences; quoting

*******
Quoting
*******

Quoting escape sequences let you insert **literal characters** that would otherwise have a special meaning in the
pattern syntax.

They are used whenever you want to match a character *as itself* rather than invoking its syntactic function (for
example, matching ``.`` instead of “any character”).

Strict Escaping
===============

This library uses **strict escaping** rules:

* Only **defined** escape sequences are accepted.
* Unknown, incomplete, or malformed escape sequences raise a **parse error**.

This differs from many other regex engines, which silently accept unknown escapes and either drop the backslash or treat
the escaped character as a literal.

Strict escaping is intentional and has important benefits:

* Typos are caught immediately instead of silently changing pattern behavior.
* Patterns remain readable and reviewable because every escape has a defined meaning.
* There is no ambiguity between “quoted literal” and “unknown syntax”.

In short, patterns either mean exactly what they say — or they fail fast.

Literal Quoting Escapes
=======================

The following escape sequences insert literal characters that would otherwise be interpreted as operators, anchors, or
structural syntax.

.. list-table::
    :header-rows: 1
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   Expression
        -   Description
    *   -   :esc_code:`\\`
        -   Inserts a literal backslash character.
    *   -   :esc_code:`\.`
        -   Inserts a literal dot character. Without quoting, ``.`` matches any character.
    *   -   :esc_code:`\"`
        -   Inserts a literal double-quote character.
    *   -   :esc_code:`\'`
        -   Inserts a literal single-quote character.
    *   -   :esc_code:`#`
        -   Inserts a literal ``#`` character. Without quoting, ``#`` starts a comment in verbose
            mode (see :doc:`special-characters`).
    *   -   :esc_code:`<` / :esc_code:`>`
        -   Inserts a literal ``<`` or ``>`` character.
    *   -   :esc_code:`+`
        -   Inserts a literal plus character. Without quoting, ``+`` is a quantifier.
    *   -   :esc_code:`*`
        -   Inserts a literal asterisk character. Without quoting, ``*`` is a quantifier.
    *   -   :esc_code:`?`
        -   Inserts a literal question mark character. Without quoting, ``?`` is a quantifier and
            part of special group syntax.
    *   -   :esc_code:`{`
        -   Inserts a literal opening brace. Without quoting, ``{`` starts a counted quantifier.
    *   -   :esc_code:`}`
        -   Inserts a literal closing brace.
    *   -   :esc_code:`$`
        -   Inserts a literal dollar sign. Without quoting, ``$`` is an anchor.
    *   -   :esc_code:`\^`
        -   Inserts a literal caret character. Without quoting, ``^`` is an anchor.
    *   -   :esc_code:`(`
        -   Inserts a literal opening parenthesis. Without quoting, ``(`` starts a group.
    *   -   :esc_code:`)`
        -   Inserts a literal closing parenthesis.
    *   -   :esc_code:`|`
        -   Inserts a literal pipe character. Without quoting, ``|`` is the alternation operator.
    *   -   :esc_code:`[`
        -   Inserts a literal opening bracket. Without quoting, ``[`` starts a character class.
    *   -   :esc_code:`]`
        -   Inserts a literal closing bracket.

As a general rule: if a character has a syntactic meaning outside a character class, it must be quoted to be matched
literally.

Quoting Inside Character Classes
================================

Inside character classes (``[...]``), many characters lose their special meaning.

However, some characters still require care:

* ``]`` must be escaped to be used literally.
* ``^`` is only literal if it does not appear first.
* ``-`` must be escaped or placed carefully to avoid creating a range.

See :doc:`character-classes` for the exact rules.

Legacy and Compatibility Expressions
====================================

.. list-table::
    :header-rows: 1
    :class: expressions
    :widths: 20 80
    :width: 100%

    *   -   Expression
        -   Description
    *   -   :esc_code:`Q` … :esc_code:`E`
        -   Legacy quoting mode found in other engines. Accepted for compatibility but not
            recommended; prefer explicit quoting as described above.

Some regex engines (notably PCRE and Java) provide a quoting mechanism using the sequence :expression:`\\Q` …
:expression:`\\E`.
This switches the parser into a mode where almost everything is treated as literal text until :expression:`\\E` is
encountered.

This library intentionally does **not** recommend this style for new patterns.

Why :esc_code:`Q`…:esc_code:`E` is discouraged
----------------------------------------------

While convenient at first glance, broad quoting modes have several drawbacks:

* They are **less explicit** than quoting individual characters, which makes patterns
  harder to review and audit.
* They can introduce **surprising edge cases** when the quoted text contains
  :expression:`\\E` or needs to embed it.
* They encourage treating patterns as opaque strings instead of structured syntax,
  which conflicts with the goal of a clear and predictable pattern language.

Recommended alternatives
------------------------

Instead of relying on a broad quoting mode, prefer one of the following approaches:

* Quote only the characters that actually need quoting using the escape sequences above.
* When matching an arbitrary external string, call
  :cpp:func:`text.toEscaped(text::EscapeFormat::RegEx) <erbsland::text::U8StringView::toEscaped>` before inserting the
  text into the pattern.

Strict escaping and the absence of a broad quoting mode serve the same purpose: they make patterns **explicit,
reviewable, and fail-fast** when something is wrong.
