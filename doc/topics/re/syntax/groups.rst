.. index::
    single: Syntax; groups
    single: Groups
    single: Capturing groups
    single: Named groups

******
Groups
******

Groups provide structure to a pattern, control operator precedence, and optionally capture
matched substrings for later use.

Use groups whenever you need to scope alternatives, apply quantifiers to a sub-expression,
or extract parts of a match.

:expression:`(...)`
===================

Capturing groups collect the substring matched by the enclosed pattern and assign it a
**group index**.

Group indices are assigned by the order of opening parentheses from left to right:

* Group ``0`` always refers to the entire match.
* The first capturing group has index ``1``, the next index ``2``, and so on.

Use capturing groups when you need to retrieve submatches from the result or reference them
in replacement expressions.

To keep patterns predictable and resource usage bounded, this engine enforces configurable
limits on both the **number of capturing groups** and the **maximum nesting depth**. See
:cpp:class:`erbsland::re::Settings` for details.

:expression:`(?<<name>>...)`
============================

Named capturing groups behave like numbered capturing groups but also associate the captured
substring with a **name**.

Named groups can be accessed by name in replacement expressions and in match result APIs,
while still receiving a numeric group index.

Group names must follow these rules:

* Allowed characters are ASCII letters, digits, and underscores.
* The name *must not* start with a digit.
* Names are case-insensitive.
* Names must be unique within the pattern.
* The maximum name length is 100 characters.

:expression:`(?:...)`
=====================

Non-capturing groups provide grouping and precedence without creating a capture group.

They are commonly used to:

* scope alternatives
* apply quantifiers to a sub-pattern
* limit the reach of inline flags

Non-capturing groups are also the container for inline flags, for example
:expression:`(?i:...)`.

A flag-only group such as :expression:`(?i)` is only allowed at the **very start** of the
pattern. Elsewhere, the parser requires the scoped form :expression:`(?i:...)` to make the
flag’s effect explicit.

:expression:`(?>...)`
=====================

Atomic groups match their content **without allowing backtracking** once a match has been
found inside the group.

If the remainder of the pattern fails, the engine will *not* retry alternative matches
within the atomic group. This can significantly improve performance and helps prevent
catastrophic backtracking in ambiguous patterns.

While the syntax is similar to PCRE and Python ``re``, this engine does not support
lookarounds or backreferences. As a result, atomic groups are often the primary tool for
controlling backtracking behavior.

Legacy and Compatibility Syntax
===============================

The following group syntaxes exist for compatibility with other regular expression engines.
They are accepted for legacy patterns but are not recommended for new ones.

:expression:`(?'<name>'...)`
----------------------------

Alternative named capturing group syntax using single quotes.

It behaves exactly like :expression:`(?<name>...)`.

:expression:`(?P<<name>>...)`
-----------------------------

Python-style named capturing group syntax.

Only the form :expression:`(?P<<name>>...)` is accepted. Backreference syntax such as
:expression:`(?P=name)` is rejected because backreferences are not supported.

:expression:`(?#...)`
---------------------

Inline comment. The content is ignored until the next closing parenthesis.

Use :esc_code:`)` to include a literal closing parenthesis inside the comment.

Unlike PCRE or Python ``re``, inline comments:

* cannot span beyond the next ``)``
* do not support nesting

These restrictions keep comments simple and prevent ambiguity during parsing.

Differences from Common Regex Engines
=====================================

If you are migrating from commonly used regular expression engines such as PCRE, PCRE2,
ECMAScript, RE2, or Python’s ``re`` module, you may encounter a few deliberate differences
in how groups behave.

These differences are intentional and focus on **clarity, predictability, and controlled
resource usage**, even when that means rejecting features that other engines provide.

No backreferences or recursive groups
-------------------------------------

This engine does **not** support:

* backreferences (for example ``\1`` or ``(?P=name)``)
* subpattern calls
* recursive group constructs

While these features are powerful, they can easily introduce exponential backtracking
and make both performance characteristics and matching behavior difficult to reason about.

By omitting them entirely, this engine guarantees more predictable execution and avoids
entire classes of pathological patterns.

Named group rules are stricter
------------------------------

Named capturing groups follow a stricter and more uniform naming scheme:

* Group names must not start with a digit.
* Names are case-insensitive.
* Names must be unique within the pattern.

Some engines allow case-sensitive names or permit names that start with digits. This
engine enforces a single, unambiguous naming model to avoid subtle portability issues and
to simplify match result handling across APIs.

Atomic groups are the primary backtracking control
--------------------------------------------------

While the syntax for atomic groups :expression:`(?>...)` is similar to PCRE, their role is
more central in this engine.

Because lookarounds and backreferences are not supported, atomic groups are the primary
mechanism for:

* preventing unwanted backtracking
* improving performance in ambiguous patterns
* expressing “commit points” in a match

This encourages a more explicit and intentional approach to controlling pattern behavior.

Group limits are enforced
-------------------------

Unlike many engines that allow patterns to grow without practical limits, this engine
enforces configurable bounds on:

* the total number of capturing groups
* the maximum nesting depth of groups

These limits help prevent accidental denial-of-service patterns and make memory usage
more predictable. See :cpp:class:`erbsland::re::Settings` for configuration details.

Legacy syntaxes are accepted but discouraged
--------------------------------------------

Alternative group syntaxes such as:

* :expression:`(?'<name>'...)`
* :expression:`(?P<name>...)`

exist purely for compatibility with other engines.

They behave exactly like the preferred modern syntax but are clearly marked as legacy to
discourage their use in new patterns. This keeps the recommended syntax surface small and
consistent.

Summary
-------

In short, grouping in this engine prioritizes **explicit structure, bounded complexity,
and maintainable patterns** over maximum expressiveness.
