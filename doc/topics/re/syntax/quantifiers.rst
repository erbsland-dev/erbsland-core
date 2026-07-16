.. index::
    single: Syntax; quantifiers
    single: Quantifiers

***********
Quantifiers
***********

Quantifiers control **how many times** the preceding element may occur.

A quantifier always applies to the **immediately preceding atom** (a literal, character
class, escape, or group). To apply a quantifier to more than one element, wrap the sequence
in a group and apply the quantifier to that group.

Quantifiers cannot appear at the start of a pattern and always require a preceding element.

Quantifier Flavors
==================

Each quantifier is available in three distinct flavors:

* **Greedy** – matches as much as possible, then backtracks if needed.
* **Lazy** – matches as little as possible, expanding only if required.
* **Possessive** – matches as much as possible and **never backtracks**.

Lazy and possessive variants are written by appending :expression:`?` or :expression:`+`
directly after the quantifier symbol.

Possessive quantifiers are supported in PCRE- and Java-style engines, but are **not**
available in Python’s standard ``re`` module. Patterns using possessive quantifiers are
therefore not portable to Python.

Use possessive quantifiers when you want to *commit* to a match and prevent expensive
backtracking in ambiguous patterns.

Quantifier Overview
===================

The following table summarizes all supported quantifiers and their available flavors.

Each quantifier applies to the **immediately preceding atom** and may be written in
*greedy*, *lazy*, or *possessive* form.

.. list-table::
    :header-rows: 1
    :class: expressions
    :widths: 25 25 25 25
    :width: 100%

    *   -   Meaning
        -   Greedy
        -   Lazy
        -   Possessive
    *   -   Zero or one
        -   :expression:`?`
        -   :expression:`??`
        -   :expression:`?+`
    *   -   Zero or more
        -   :expression:`*`
        -   :expression:`*?`
        -   :expression:`*+`
    *   -   One or more
        -   :expression:`+`
        -   :expression:`+?`
        -   :expression:`++`
    *   -   Exactly *n*
        -   :expression:`{n}`
        -   –
        -   –
    *   -   *n* to *m*
        -   :expression:`{n,m}`
        -   :expression:`{n,m}?`
        -   :expression:`{n,m}+`
    *   -   At least *n*
        -   :expression:`{n,}`
        -   :expression:`{n,}?`
        -   :expression:`{n,}+`

Lazy and possessive variants are written by appending :expression:`?` or :expression:`+`
directly after the quantifier symbol.

Optional Quantifiers
====================

:expression:`?`
---------------

Match **zero or one** occurrence (greedy).

Use this quantifier to make the preceding element optional.

.. code-block:: text
    :caption: Example

    colou?r

This matches ``color`` and ``colour``.

:expression:`?+`
----------------

Match **zero or one** occurrence (possessive).

Once the optional element is consumed, it is never given back during backtracking.

.. code-block:: text
    :caption: Example

    https?+

This matches ``http`` or ``https`` and prevents backtracking inside the optional ``s``.

:expression:`??`
----------------

Match **zero or one** occurrence (lazy).

The engine first tries to skip the optional element and only consumes it if required for
the overall match to succeed.

.. code-block:: text
    :caption: Example

    <[^>]+??>

This allows following tokens to decide where the tag should close.

Repetition Quantifiers
======================

:expression:`*`
---------------

Match **zero or more** occurrences (greedy).

.. code-block:: text
    :caption: Example

    a*

This matches ``""``, ``"a"``, ``"aa"``, and so on.

:expression:`*+`
----------------

Match **zero or more** occurrences (possessive).

This is useful when you want to consume as much input as possible and explicitly prevent
backtracking.

.. code-block:: text
    :caption: Example

    \S*+

This consumes a whole run of non-whitespace characters and never backtracks.

:expression:`*?`
----------------

Match **zero or more** occurrences (lazy).

.. code-block:: text
    :caption: Example

    <.*?>

This matches the shortest possible tag.

One-or-More Quantifiers
=======================

:expression:`+`
---------------

Match **one or more** occurrences (greedy).

.. code-block:: text
    :caption: Example

    \d+

This matches one or more digits.

:expression:`++`
----------------

Match **one or more** occurrences (possessive).

.. code-block:: text
    :caption: Example

    \w++

This consumes the longest possible word and never backtracks.

:expression:`+?`
----------------

Match **one or more** occurrences (lazy).

.. code-block:: text
    :caption: Example

    \d+?

This matches as few digits as possible while still allowing a full match.

Counted Quantifiers
===================

:expression:`{<n>}`
-------------------

Match **exactly** *n* occurrences (greedy).

.. code-block:: text
    :caption: Example

    \d{4}

This matches a four-digit year.

:expression:`{<n>,<m>}`
-----------------------

Match **at least** *n* and **at most** *m* occurrences (greedy).

The engine rejects values where *m* is smaller than *n*.

.. code-block:: text
    :caption: Example

    [A-F0-9]{2,4}

This matches 2 to 4 hexadecimal digits.

:expression:`{<n>,<m>}+`
------------------------

Match **at least** *n* and **at most** *m* occurrences (possessive).

.. code-block:: text
    :caption: Example

    \w{2,5}+

This avoids backtracking within the counted range.

:expression:`{<n>,<m>}?`
------------------------

Match **at least** *n* and **at most** *m* occurrences (lazy).

.. code-block:: text
    :caption: Example

    .{2,5}?

This expands only as far as necessary.

Open-Ended Counted Quantifiers
==============================

:expression:`{<n>,}`
--------------------

Match **at least** *n* occurrences (greedy).

.. code-block:: text
    :caption: Example

    \w{3,}

This matches a word of length three or more.

:expression:`{<n>,}+`
---------------------

Match **at least** *n* occurrences (possessive).

.. code-block:: text
    :caption: Example

    \d{1,}+

This consumes all following digits without backtracking.

:expression:`{<n>,}?`
---------------------

Match **at least** *n* occurrences (lazy).

.. code-block:: text
    :caption: Example

    .{1,}?

This grows to the smallest length that still permits a match.

Possessive Quantifiers
======================

Possessive quantifiers match as much input as possible and **never give it back** during
backtracking.

Once a possessive quantifier has consumed input, that decision is final. If the remainder
of the pattern fails, the engine does **not** retry with a shorter match.

Why Possessive Quantifiers Exist
--------------------------------

This engine is based on a **Thompson-style NFA implementation**. In such an engine:

* Greedy quantifiers create multiple possible execution paths.
* Backtracking explores these paths when later parts of the pattern fail.
* Certain patterns can cause excessive or even catastrophic backtracking.

Possessive quantifiers explicitly **remove backtracking states** from the automaton.
This turns a potentially exponential search into a linear one.

In other words, possessive quantifiers act as a *commit point* in the pattern.

Practical Example: Avoiding Catastrophic Backtracking
-----------------------------------------------------

Consider the following pattern:

.. code-block:: text

    ^(\w+)*$

On long input like ``"aaaaaaaaaaaa!"``, this pattern creates a large number of overlapping
paths and may take a very long time to fail.

Using a possessive quantifier fixes the problem:

.. code-block:: text

    ^(\w++)*$

Here, ``\w++`` consumes the longest possible word fragment and never backtracks.
The failure happens immediately when the ``!`` is encountered.

Practical Example: Token Consumption
------------------------------------

Suppose you want to consume a token up to the next delimiter:

.. code-block:: text

    \S*:

With a greedy quantifier, the engine may backtrack if later parts fail.
With a possessive quantifier:

.. code-block:: text

    \S*+:

the token is consumed once, and matching proceeds without reconsidering its length.

This is especially useful in parsers, log processing, and protocol grammars.

Relationship to Atomic Groups
-----------------------------

Possessive quantifiers are closely related to atomic groups:

.. code-block:: text

    \w++        ≡        (?>\w+)
    a*+         ≡        (?>a*)

Both constructs prevent backtracking inside their scope.

Use possessive quantifiers when:

* the quantified atom is simple
* you want compact syntax
* performance and predictability matter

Use atomic groups when:

* the quantified expression is complex
* you want to commit a larger sub-pattern

In Thompson-style engines, both forms compile down to equivalent automaton structures.

Counted Quantifier Limits
=========================

Counted quantifiers accept only decimal digits with no surrounding whitespace.

* Numbers may contain **at most five digits**.
* Values are additionally limited by the engine’s maximum quantifier count
  (default: 10 000).

This is stricter than some engines (PCRE, RE2, Python), which allow larger values.
Large ranges should instead be expressed using additional structure, multiple
quantifiers, or by adjusting the engine settings.

Legacy and Compatibility Quantifiers
====================================

:expression:`{,<m>}`
--------------------

Match **zero to** *m* occurrences (greedy).

This syntax is supported for compatibility but is not accepted by many other regex
engines. For new patterns, prefer the explicit form :expression:`{0,<m>}`.

.. code-block:: text

    a{,3}

This matches ``""``, ``"a"``, ``"aa"``, and ``"aaa"``.

Differences from Common Regex Engines
=====================================

If you are migrating from regex engines such as PCRE, PCRE2, Java, RE2, or Python’s
:mod:`re` module, you may notice several intentional differences in quantifier behavior.

Possessive Quantifiers
----------------------

This engine fully supports possessive quantifiers (for example :expression:`*+`,
:expression:`++`, :expression:`{n,m}+`).

Stricter Limits on Counted Quantifiers
--------------------------------------

Counted quantifiers are intentionally restricted:

* numbers may contain at most five digits
* values are capped by a configurable maximum (default: 10 000)

Some engines allow arbitrarily large values, which can lead to excessive memory usage or
slow compilation. This engine rejects such patterns early.

Summary
-------

Quantifiers in this engine prioritize **explicit intent, linear performance, and safe
failure modes**.
