*****************************
Unnecessary Type Construction
*****************************

:Rule ID: ``unnecessary_type_construction``
:Severity: low

Pass values directly when the receiving API already performs the required implicit construction.
Explicit temporaries obscure the operation when they add no type information needed for overload selection.
For example:

.. code-block:: cpp

    const auto characters = text::CharSet{U'x'};
    const auto prefix = value.slice({unit::ByteIndex{0}, unit::ByteCount{2}});
    const auto found = value.contains("needle"_el);

Mechanical Detection
====================

The detector recognizes these deliberately narrow forms:

* ``CharSet{Char{U'x'}}`` with a single UTF-32 character literal.
* A ``String{"text"_el}`` first argument to the member functions ``contains``, ``startsWith``, ``endsWith``, and ``find``.
  These Core string APIs accept a string literal through implicit construction.
* A ``ByteRange`` first argument to ``slice`` containing exactly two explicitly brace-constructed coordinates:
  ``ByteIndex`` followed by either ``ByteCount`` or ``ByteIndex``.
  The typed coordinates retain overload selection after removing ``ByteRange``.

Unqualified, domain-qualified, ``el::`` -qualified, and ``erbsland::`` -qualified Core type spellings are recognized.
Comments and directives are not treated as expressions, and literals are inspected only in the recognized argument
positions.

Arbitrary functions, variable arguments, empty ranges, different character widths, and unknown type qualifiers are
excluded.
In particular, ``slice({})`` can be ambiguous between byte and code-point range overloads.
The scanner does not resolve receiver types or overloads; a matching method name on an unrelated type still requires
manual review.
Do not remove construction needed for ownership, lifetime, overload selection, or template argument deduction.
