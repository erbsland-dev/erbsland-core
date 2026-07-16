.. index::
    single: Syntax; alternatives
    single: Alternatives
    single: Operator; |

************
Alternatives
************

Alternatives allow you to express *“match one of several options”* within a single pattern.
You define alternatives with the :expression:`|` operator, which splits the *current group*
into multiple branches that are tried from left to right.

The :expression:`|` operator has the **lowest precedence** in the syntax. This means it always
applies to the entire *current group* unless you explicitly introduce grouping. Keeping this
rule in mind is essential for writing correct and readable patterns.

:expression:`A|B|C`
===================

Use the :expression:`|` operator to separate alternatives that should be matched in order.
For example, :expression:`cat|dog` matches either “cat” or “dog”.

Alternatives are scoped to the *current group*. If you do not define a group explicitly, the
entire pattern becomes the scope. As a result, :expression:`A|BC` is interpreted as
“match ``A`` **or** ``BC``”.

If your intent is to match “``AB`` or ``C``”, you must introduce grouping explicitly:

:expression:`(?:AB|C)`

Being explicit about grouping avoids subtle precedence errors and makes your patterns easier
to understand when revisiting them later.

The number of alternatives per group is limited by the parser settings. By default, a group
may contain up to **1000 alternatives**. Exceeding this limit results in a parsing error.

If you need to match a literal :expression:`|` character, escape it as :esc_code:`|`
or place it inside a character class, for example :expression:`[|]`.

:expression:`(?:A|B|C)`
=======================

Use a *non-capturing group* to scope alternatives without creating a capture group. This is
essential whenever alternatives are part of a larger expression.

For example:

* :expression:`(?:A|B)C` matches “AC” or “BC”
* :expression:`A|BC` matches “A” or “BC”

Non-capturing groups are also particularly useful when you apply quantifiers to an entire
set of alternatives. For instance, :expression:`(?:cat|dog)s?` matches:

* “cat”
* “cats”
* “dog”
* “dogs”

Using non-capturing groups in these cases keeps your patterns precise and avoids unnecessary
captures, which simplifies both matching logic and long-term maintenance.

Empty alternatives and empty groups
===================================

Unlike many regular expression libraries, empty alternatives and empty groups are
*forbidden by default*.

This means patterns such as:

* :expression:`A|`
* :expression:`|A`
* :expression:`A||B`
* :expression:`()`
* :expression:`(?:A|)`

will result in a **parser error** unless explicitly enabled via feature flags.

Why empty alternatives are forbidden
------------------------------------

Empty alternatives often look convenient, but in practice they tend to obscure intent and
introduce ambiguity. For example:

:expression:`(?:a|b|)`

At first glance, this might look like a compact way to express “``a`` or ``b`` or nothing”.
However, it actually mixes two different concepts:

* choosing between alternatives
* making a sub-expression optional

In this library, these concepts are intentionally kept **separate and explicit**. The
recommended and clearer form is:

:expression:`(?:a|b)?`

This version makes the optional nature of the expression obvious at a glance and avoids
hidden empty branches that can complicate reasoning about the pattern.

Another common pitfall is accidental emptiness caused by refactoring. Removing or commenting
out a branch in a large alternative list can silently introduce an empty alternative in other
regex engines, changing the meaning of the pattern without any immediate warning. By
forbidding empty alternatives, the parser turns such cases into explicit errors.

Why empty groups are forbidden
------------------------------

Empty groups, such as :expression:`()`, rarely convey useful intent on their own. In most
cases, they are either:

* a mistake
* a leftover from an earlier refactoring step
* an attempt to create optional structure without a clear semantic meaning

Disallowing empty groups by default helps keep patterns intentional and self-explanatory,
and prevents constructs that match the empty string without making this behavior explicit.

Enabling empty alternatives and groups explicitly
-------------------------------------------------

If your use case genuinely requires empty alternatives or empty groups, you can enable them
programmatically:

* Empty alternatives can be enabled with
  :cpp:any:`Feature::EmptyAlternatives <erbsland::re::Feature::EmptyAlternatives>`
* Empty groups can be enabled with :cpp:any:`Feature::EmptyGroups`

These feature flags make the decision explicit in code, ensuring that anyone reading or
maintaining the pattern understands that empty constructs are intentional and not accidental.

In short, the default behavior favors **clarity, explicit intent, and safer refactoring**,
while still allowing advanced users to opt into more permissive syntax when needed.
