****************************
Unnecessary Unit Conversions
****************************

:Rule ID: ``unnecessary_unit_conversion``
:Severity: low

Keep sequence coordinates typed when comparing them with a constant.
Converting to a native integer loses the unit and intent without helping the comparison.
Construct the constant in the receiver's actual type:

.. code-block:: cpp

    items.count() >= unit::ItemCount{16U};
    text.length() >= unit::ByteCount{16U};
    position >= unit::ByteIndex{16U};

Do not assume that every ``length()`` returns an item count.
Preserve the specific unit and the distinction between an index, an amount, and an offset.
Native API boundaries still need explicit conversions.

Mechanical Detection
====================

The detector flags a member ``toSizeT()`` call directly followed by ``==``, ``!=``, ``<``, ``<=``, ``>``, or ``>=`` and
an unsigned or unsuffixed integer literal.
Decimal, hexadecimal, and binary literals with standard integer suffixes are recognized.
A larger right-hand arithmetic expression, signed literal, floating-point literal, user-defined suffix, and calls to
``toSizeTOrThrow()`` are excluded.
Comments, quoted source examples, and preprocessing directives do not produce findings.

This is a best-effort check: ``toSizeT()`` is assumed to belong to a Core unit type.
It does not resolve types, infer the replacement type, recognize every equivalent comparison spelling, or rewrite code.
Review special states and representation limits before changing a comparison.
