.. index::
    single: Text
    single: Base-N
    single: Base16
    single: Base32
    single: Base64
    single: PEM

****************************
Base-N Encoding and Decoding
****************************

The non-flattened ``erbsland::text::base_n`` namespace provides bulk conversion between
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` and all supported string widths.
The predefined formats cover RFC 4648 Base16, Base32, Base32hex, Base64, and Base64url.
The PEM format factory adds 64-character LF-separated payload wrapping without armor.

Use the explicit namespace because these names are intentionally not added to the flattened ``erbsland`` namespace:

.. code-block:: cpp

    namespace base_n = erbsland::text::base_n;

    const auto encoded = base_n::BaseNEncoder{data, base_n::BaseNFormat::base64()}.toString();
    const auto decoded = base_n::BaseNDecoder{encoded}.toDataOrThrow();

Decoding is strict after configured whitespace is removed.
It rejects unknown characters, malformed or missing required padding, incomplete groups, and non-zero unused bits.
Use :cpp:func:`BaseNDecoder::toData() <erbsland::text::base_n::BaseNDecoder::toData>` when malformed input and size
limit failures should both produce an empty optional.
Use :cpp:func:`BaseNDecoder::toDataOrThrow() <erbsland::text::base_n::BaseNDecoder::toDataOrThrow>` when diagnostics
must distinguish parse failures from decoded-size limits.

Interface
=========

.. doxygenclass:: erbsland::text::base_n::BaseNDecoder
    :members:
.. doxygenclass:: erbsland::text::base_n::BaseNEncoder
    :members:
.. doxygenclass:: erbsland::text::base_n::BaseNFormat
    :members:
.. doxygenenum:: erbsland::text::base_n::BaseNFormatFlag

.. doxygentypedef:: erbsland::text::base_n::BaseNFormatFlags
