*****************************
Missing Default Group Comment
*****************************

:Rule ID: ``missing_default_group_comment``
:Severity: medium

Explicitly defaulted or deleted constructors, destructors, and assignment operators are usually self-describing.
Group consecutive declarations beneath a ``// defaults`` or ``// defaults/deletions`` comment instead.

An explicitly defaulted default constructor may have API documentation when default construction has meaningful
semantics.
A documented default constructor does not require a defaults-group comment.
Other explicitly defaulted or deleted special members must remain in a defaults group.

.. code-block:: cpp

    // defaults/deletions
    Value() = default;
    Value(const Value &) = delete;
    auto operator=(const Value &) -> Value & = delete;

Defaulted comparison operators, such as ``operator==`` and ``operator<=>``, do not need API documentation or a
defaults-group comment.

Mechanical Detection
====================

The scanner reports explicitly defaulted or deleted special members in ``*.hpp`` files that are outside a defaults
group.
