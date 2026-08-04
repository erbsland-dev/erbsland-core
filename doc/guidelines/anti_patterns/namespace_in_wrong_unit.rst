*****************************
Namespaces in the Wrong Units
*****************************

:Rule ID: ``namespace_in_wrong_unit``
:Severity: high

Namespace declarations below ``src/erbsland`` must match the API boundary expressed by their source directory.
Mismatches make declarations difficult to find and can accidentally expose implementation details as public API, or hide
public declarations in implementation directories.

Correct Placement
=================

For example, ``erbsland::example::impl`` belongs below ``src/erbsland/example/impl``.
Conversely, ``erbsland::example`` must not be declared below that ``impl`` directory.
Additional directories may group related files without introducing another namespace.

Mechanical Detection
====================

Below ``src/erbsland``, the scanner reports namespaces whose domain or ``impl`` component does not match the file path.
