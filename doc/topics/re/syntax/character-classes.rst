.. index::
    single: Syntax; character classes
    single: Character classes
    single: POSIX; character classes

*****************
Character Classes
*****************

Character classes define a **set of characters** that are allowed (or disallowed) at a single
position in the input. They always match **exactly one character** and never more.

Use character classes when you want to express *“any one of these characters”* concisely and
efficiently.

.. note::

    If you are migrating from another regular expression engine, be aware that this library
    intentionally differs in a few areas. See
    `Differences from Common Regex Engines`_ at the end of this page.

:expression:`[...]`
===================

Matches a single character from a set.

A character class may contain a combination of:

* Literal characters (for example :expression:`[abc]`)
* Ranges using :expression:`-` (for example :expression:`[a-z]`)
* Character type escapes such as :esc_code:`d`, :esc_code:`s`, :esc_code:`w`,
  :esc_code:`p{<category>}`, :esc_code:`h`, and :esc_code:`v`

Character classes are evaluated atomically: exactly one character must match the class for the
pattern to continue.

Rules and special cases
-----------------------

The following rules are enforced by the parser. Many of them differ from permissive regex
engines and are explained in more detail at the end of this page.

* Empty classes are not allowed. :expression:`[]` always raises a parse error.
* The closing bracket :expression:`]` must be escaped to be used literally:
  :expression:`[\\]]`.
* The opening bracket :expression:`[` must always be escaped:
  :expression:`[\\[]`.
* The caret :expression:`^` is only allowed as the *first* character to negate the class.
  To match a literal caret, use :expression:`[\\^]`.
* The dash :expression:`-` introduces a range only when it follows a literal character.
  To match a literal dash, use :expression:`[-a]` or :expression:`[\\-]`.
  A trailing dash such as :expression:`[a-]` is a parse error.
* Ranges must be literal-to-literal. A range cannot end in a character class or category.
  For example, :expression:`[a-\\d]` is invalid.
* In case-insensitive mode, literals and range endpoints are case-folded.
  If only one endpoint changes under case folding (for example :expression:`[A-z]`),
  the range is rejected.
* Negated character type escapes such as :esc_code:`D`, :esc_code:`S`, :esc_code:`W`,
  :esc_code:`H`, :esc_code:`V`, and :esc_code:`P{...}` are not allowed inside character
  classes.
* Anchors such as :esc_code:`b`, :esc_code:`A`, or :esc_code:`Z` are not allowed inside
  character classes.

The exact meaning of character type escapes depends on the active ASCII or Unicode mode.
See :doc:`character-types` for details.

:expression:`[^...]`
====================

Negates the character class. It matches any single character **not** listed in the class.

The caret must appear immediately after the opening bracket. A caret in any other position is a
parse error and must be escaped as :expression:`[\\^]` if used literally.

Negation always applies to the *entire* class, not to individual elements within it.

Legacy and Compatibility Features
=================================

The following constructs exist for compatibility with other regular expression engines.
They are clearly marked as legacy to discourage their use in new patterns unless compatibility
is required.

:expression:`[\\Q...\\E]`
-------------------------

Legacy quoted-literal mode inside a character class. Everything between :esc_code:`Q` and
:esc_code:`E` is treated as literal text, including characters that would normally terminate
the class, such as :expression:`]`.

The quoted block must be properly terminated. A missing :esc_code:`E` always results in a
parse error.

:expression:`[[:<name>:]]`
--------------------------

POSIX-style character classes. These are supported for compatibility and are only available when
the :cpp:any:`Feature::PosixClasses <erbsland::re::Feature::PosixClasses>` option is enabled
(enabled by default).

The supported class names are:

* :expression:`alnum`
* :expression:`alpha`
* :expression:`ascii`
* :expression:`blank`
* :expression:`cntrl`
* :expression:`digit`
* :expression:`graph`
* :expression:`lower`
* :expression:`print`
* :expression:`punct`
* :expression:`space`
* :expression:`upper`
* :expression:`word` (PCRE-style extension)
* :expression:`xdigit`

The exact set of characters depends on the active ASCII or Unicode mode and may therefore differ
from engines that always interpret POSIX classes as ASCII-only. See :doc:`modes-and-flags`
for details.

POSIX classes cannot be mixed with literals or ranges inside the same character class. For example,
:expression:`[[:digit:]x]` is a parse error. Use alternation instead:

:expression:`(?:[[:digit:]]|x)`

:expression:`[[:^<name>:]]`
---------------------------

Negated POSIX class. This negates the **entire** class.

This form cannot be combined with other POSIX classes and cannot be used together with the
leading :expression:`^` negation syntax.

Use :expression:`[[:^digit:]]` instead of :expression:`[^[:digit:]]`.

:expression:`[[:...:][:...:]]`
------------------------------

Multiple POSIX classes may be combined by concatenation, for example:

* :expression:`[[:alpha:][:digit:]]`

This syntax is limited strictly to POSIX classes. It cannot include literal characters or ranges.
If you need a mix, use alternation instead.

Differences from Common Regex Engines
=====================================

If you are migrating from widely used regular expression engines such as PCRE, PCRE2,
ECMAScript, RE2, or ``std::regex``, you may notice a few deliberate differences in how
character classes behave.

These differences are designed to make patterns **more explicit, less ambiguous, and safer
to refactor**, even if that means rejecting some constructs that other engines accept.

No empty character classes
--------------------------

Empty character classes are rejected unconditionally:

* :expression:`[]` is always a parse error.

Some engines treat empty classes as undefined behavior or allow them in edge cases.
This engine forbids them outright, because an empty class can never match and almost
always indicates a mistake.

Strict range validation
-----------------------

Ranges must always be **literal-to-literal** and well-defined:

* :expression:`[a-z]` is valid
* :expression:`[a-\\d]` is invalid
* :expression:`[A-z]` is rejected in case-insensitive mode

In many engines, ranges like :expression:`[A-z]` are accepted but include unintended
characters such as ``[``, ``\\``, ``]``, ``^``, ``_``, and ````` due to ASCII ordering.
This engine rejects such ranges to prevent subtle bugs and portability issues.

No anchors inside character classes
-----------------------------------

Anchors such as :expression:`\\b`, :expression:`\\B`, :expression:`\\A`, or :expression:`\\Z`
are not allowed inside character classes.

Other engines may silently reinterpret these constructs or assign them special meanings
(e.g. backspace vs. word boundary). This engine enforces a clear separation:

* Character classes match **characters**
* Anchors match **positions**

If you need positional logic, use grouping and alternation instead.

No negated escapes inside classes
---------------------------------

Negated character type escapes like :esc_code:`D`, :esc_code:`S`, or :esc_code:`W` are not
allowed inside character classes.

While some engines allow constructs such as :expression:`[\\D_]`, this engine requires you
to express negation explicitly:

* Use class negation: :expression:`[^\\d_]`
* Or alternation when appropriate

This avoids double-negation constructs that are difficult to reason about and easy to misread.

Case folding is explicit and conservative
------------------------------------------

In case-insensitive mode, both literals and range endpoints are case-folded.

If case folding would change only one endpoint of a range, the range is rejected instead of
being silently widened or altered. This prevents patterns whose meaning changes depending on
locale, Unicode version, or implementation details.

POSIX classes are Unicode-aware
-------------------------------

Unlike some engines that always interpret POSIX classes as ASCII-only, POSIX classes in this
engine respect the active ASCII or Unicode mode.

As a result, the set of matched characters may differ from engines that hard-code POSIX
behavior to ASCII. This design keeps character classes consistent with the rest of the
matching engine.

Legacy features are explicit
----------------------------

Features such as quoted literals (:expression:`[\\Q...\\E]`) and POSIX classes exist purely
for compatibility. They are clearly marked as legacy and can be disabled via feature flags.

For new patterns, prefer modern character escapes and explicit alternation for clarity and
long-term maintainability.

Summary
-------

In short, this engine favors **explicit intent, predictable behavior, and early error
detection** over permissive syntax. Patterns that compile successfully are easier to read,
easier to maintain, and less likely to change meaning during refactoring.

If a construct is rejected, the parser error usually points to a clearer and more explicit
alternative.
