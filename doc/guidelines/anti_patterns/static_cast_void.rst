***************************************************
Silencing nodiscard with ``static_cast<void>(...)``
***************************************************

:Rule ID: ``static_cast_void``
:Severity: medium

If code silences a ``[[nodiscard]]`` result using ``static_cast<void>(...)``:

* the API may accidentally mark a return value as non-discardable even though a common use case does not require it; or
* the usage location may use the wrong API or discard an important return value.

Scope and Exceptions
====================

* This form of silencing is allowed in unit tests. This exception is configured centrally in
  ``utilities/conf/anti_patterns.elcl``.
* It may be accepted for very narrow cases in templates only if there is no alternative and every error case is safely
  covered before the call. The reasoning must be documented with an inline anti-pattern acceptance comment.

Correct Solution
================

* Analyze the situation thoughtfully.
* Remove ``[[nodiscard]]`` from the API if the result is routinely optional; or
* use the correct API for the use case, such as an ``...OrThrow`` variant; or
* handle the return value.

Mechanical Detection
====================

The scanner detects ``static_cast<void>(...)`` outside comments and literals.
