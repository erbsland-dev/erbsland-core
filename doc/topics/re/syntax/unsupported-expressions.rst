.. index::
    single: Syntax; unsupported
    single: Unsupported expressions
    single: Errors; parser

***********************
Unsupported Expressions
***********************

This page documents syntax constructs that are **intentionally not supported** by this
library.

These omissions are deliberate. They avoid ambiguity, improve Unicode correctness, and
guarantee predictable performance characteristics. Where possible, alternatives or
recommended approaches are provided.

Numeric and Ambiguous Escapes
=============================

:esc_code:`0<dd>`
-----------------

Zero-prefixed numeric escapes are interpreted inconsistently across regex engines:

* Some treat them as octal escapes.
* Others interpret them as numeric backreferences.

To avoid this ambiguity entirely, this engine rejects :esc_code:`0<dd>` syntax.

.. rubric:: Use instead:

* Explicit octal escapes: :esc_code:`o{<ddd…>}`
* Unicode escapes: :esc_code:`u{<hh…>}`

:esc_code:`<ddd>`
------------------

A plain numeric escape is ambiguous in many regex dialects and often overlaps with
replacement syntax.

This engine therefore rejects :esc_code:`<ddd>` escapes outright and requires explicit
notation to make intent clear.

.. rubric:: Use instead:

* :esc_code:`o{<ddd…>}`
* :esc_code:`u{<hh…>}`

Byte-Oriented and Line-Oriented Escapes
=======================================

:esc_code:`C`
-------------

In some engines, :esc_code:`C` matches a single raw byte.

This engine operates on **valid UTF-8 code points only**. Matching raw bytes could split
multi-byte sequences and produce invalid Unicode, potentially corrupting downstream
processing.

For safety and correctness, :esc_code:`C` is not supported.

:esc_code:`R`
-------------

The :esc_code:`R` escape usually means “any line break sequence”.

This library normalizes line endings internally. As a result, :esc_code:`R` would collapse
to :esc_code:`n` and behave differently depending on normalization settings.

To avoid surprising behavior, this escape is rejected.

.. rubric:: Use instead:

* :esc_code:`n` for normalized newlines
* Use the :cpp:any:`Flag::CRLF <erbsland::re::Flag::CRLF>` flag, to fold CRLF line-breaks into LF ones.
* An explicit alternation such as :expression:`(\\r\\n|\\n|\\r)` when working with raw input

Unicode and Text Segmentation
=============================

:esc_code:`X`
-------------

In some engines, :esc_code:`X` matches an extended grapheme cluster.

Correct grapheme segmentation requires full Unicode boundary rules and stateful matching
across code points. This is outside the scope of this engine.

.. rubric:: Use instead:

* Explicit Unicode property classes (for example :expression:`[\\p{L}\\p{N}]`)
* Quantifiers to control cluster size

Backtracking-Dependent Constructs
=================================

Backreferences
--------------

Backreferences (numeric or named) turn regular expressions into **non-regular patterns**.
They require backtracking and can lead to exponential runtime behavior.

This engine prioritizes predictable performance and therefore does not support
backreferences.

.. rubric:: Use instead:

* Capture substrings
* Compare them explicitly in application code

:expression:`(?|...)`
---------------------

Branch-reset groups change capture numbering depending on which alternative matched.

This makes group indices unstable and complicates result handling.

.. rubric:: Use instead:

* Non-capturing groups
* Explicit named groups for clarity

Lookahead and Lookbehind
------------------------

Lookaround assertions require evaluating alternative match paths without consuming input.
This often forces backtracking and complicates streaming evaluation.

To keep matching efficient and predictable, lookahead and lookbehind constructs are not
supported.

.. rubric:: Use instead:

* Pattern restructuring
* Additional validation in application code

Recursion and Subroutines
-------------------------

Recursive patterns and subroutine calls can express context-free grammars but introduce
unbounded recursion and complex backtracking behavior.

These constructs are rejected to maintain safety and performance guarantees.

Control-Flow and State-Based Syntax
===================================

:esc_code:`G`
-------------

The :esc_code:`G` anchor matches at the end of the previous match in some engines.

This engine treats matches as independent operations and does not retain implicit state
between searches.

.. rubric:: Use instead:

* Resume matching at a specific offset using the API

:esc_code:`K`
-------------

The :esc_code:`K` escape resets the start of the reported match mid-pattern.

This primarily exists to support replacement shortcuts in engines without rich capture
APIs. In this library, captures are first-class and provide the same functionality without
altering match semantics.

:expression:`(?C)` / :expression:`(?C<n>)`
------------------------------------------

Callout expressions embed user callbacks into the matching process.

This engine exposes structured debugging and inspection facilities through its API instead,
so embedding callbacks into the pattern is unnecessary and unsupported.

:expression:`(?J)`
------------------

Allowing duplicate group names makes it unclear which capture is returned and complicates
maintenance.

This engine enforces unique group names to keep captures deterministic and readable.

:expression:`(?U)`
------------------

This mode flips quantifiers to be ungreedy by default.

Global behavior changes of this kind obscure intent and are easy to confuse with the
Unicode-related :expression:`(?u)` flag.

.. rubric:: Use instead:

* Explicit lazy quantifiers where required

Parser and Engine Directives
============================

:expression:`(*...)`
--------------------

The :expression:`(*...)` verbs act as parser or engine tuning directives in some regex
flavors.

This engine exposes configuration through its API rather than inline syntax, keeping
patterns focused on matching logic and portable across environments.

Conditional Patterns
====================

Conditional groups depend on runtime match state (for example whether a group was matched)
and introduce hidden control flow into the pattern.

This execution model is not supported.

.. rubric:: Use instead:

* Separate expressions
* Conditional logic in application code

Empty Groups and Alternatives
=============================

Empty groups such as :expression:`()` or :expression:`(?:)` are **not allowed by default**.

They match an empty string without performing a meaningful operation and are often the
result of a mistake.

If you explicitly need empty groups, enable them using
:cpp:any:`Feature::EmptyGroups <erbsland::re::Feature::EmptyGroups>`.

Empty alternatives are also rejected by default. If required, enable
:cpp:any:`Feature::EmptyAlternatives <erbsland::re::Feature::EmptyAlternatives>` and ensure
that the group still contains at least one non-empty alternative or surrounding structure.

Summary
=======

Unsupported constructs in this engine are not accidental omissions. Each rejected feature
would either:

* introduce ambiguity
* weaken Unicode guarantees
* complicate performance characteristics
* or obscure pattern intent

When a construct is rejected, the parser error typically points toward a clearer and more
explicit alternative.
