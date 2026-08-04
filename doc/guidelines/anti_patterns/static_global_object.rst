*********************************
Static Global Object Construction
*********************************

:Rule ID: ``static_global_object``
:Severity: high

Erbsland Core is a large library and every static global constructed object adds to the startup cost of any application
that links the library.
Errors while constructing these static global objects also cannot be caught and are hard to pin down.

Scope and Exceptions
====================

* Yes: Any non-trivial constructed object in the global namespace that will be constructed at application startup.
* Yes: Data arrays, vectors, and lists in the global namespace that are not ``constexpr``.
* No: Trivial numeric constants or pointers initialized with ``nullptr``.

Correct Solution
================

* Make initialization lazy by wrapping the static data in a function call.
* Ensure the data is initialized only if it is actually used by an application.

Mechanical Detection
====================

The scanner reports namespace-scope constructed objects and non-``constexpr`` arrays.
