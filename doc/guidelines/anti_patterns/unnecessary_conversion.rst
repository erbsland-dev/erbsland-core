*************************************************
Inefficient and Unnecessary Conversions or Copies
*************************************************

:Rule ID: ``unnecessary_conversion``
:Severity: low
:Automated detection: unavailable

Inefficient and unnecessary conversions occur when a type is converted only to use a specific kind of API, not because
the new representation is actually needed.
Worse, code sometimes converts the value back after using the API, causing unnecessary allocations and copies.

Mechanical Detection
====================

The scanner does not detect general conversion and copying inefficiencies.
The narrower :doc:`unnecessary_unit_conversion` and :doc:`unnecessary_type_construction` rules cover known mechanical
forms.
