**********************************************
Regular Literal Strings for Erbsland Core APIs
**********************************************

:Rule ID: ``regular_string_literal``
:Severity: medium

At many locations, regular string literals are used for APIs that take a ``String`` argument instead of an Erbsland Core
string literal (``""_el``).
This looks like a minor issue, but it has a significant performance and memory impact.

Regular string literals **always cause a copy of the text**, while Erbsland Core string literals are used as no-copy
references to read-only memory.
They avoid both the copy and a heap allocation.
This is especially important when throwing exceptions or comparing against static text values.

Scope and Exceptions
====================

In headers and templates, a ``throw...Error(std::string_view text)`` wrapper is accepted if using the Erbsland Core string
types would add an unnecessary include dependency to the public API.
Files with ``Windows`` or ``Posix`` in their names are centrally excluded from this rule because they primarily contain
native API integration code where narrow C strings are expected.
These files still require manual inspection.
Plain narrow literals are also correct inside ``static_assert`` and ``assert`` expressions and as messages in the
standard ``deprecated`` attribute.

Correct Solution
================

* Add ``using namespace text::literal;`` inside the primary namespace bracket of a ``cpp`` file.
* In ``hpp`` and template files, add the using directive inside the function. In ``impl`` namespaces, it can be added
  directly to the namespace, as in ``cpp`` files.
* Add the ``_el`` suffix to all string literals where possible.
* An explicit direct construction, such as ``StringLiteral{"text"}``, ``U8StringLiteral<char>{"text"}``, or another
  type that deliberately accepts a C string, is also valid and does not require an ``_el`` suffix.

Mechanical Detection
====================

The scanner reports likely unsuffixed narrow string literals.
Inspect each reported location manually.
