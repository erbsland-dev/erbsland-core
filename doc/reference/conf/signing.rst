.. index::
    single: Configuration Signing

*********************
Configuration Signing
*********************

Signing APIs use :cpp:class:`erbsland::path::Path <erbsland::path::Path>` for source and destination files and Core
strings for textual signature data.
:cpp:class:`erbsland::conf::Signer <erbsland::conf::Signer>` reads the complete source, validates its UTF-8 encoding and
line limits, calculates the document digest, and asks a user-supplied
:cpp:class:`erbsland::conf::SignatureSigner <erbsland::conf::SignatureSigner>` to create the signature text.

The output is a copy of the source with an initial signature line inserted or replaced.
The signer preserves the document's line-ending convention and the byte-level digest rules used by the parser.
It does not validate ELCL syntax, so applications should parse a document successfully before signing it.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    auto implementation = std::make_shared<MySignatureSigner>();
    erbsland::conf::Signer signer{implementation};
    signer.sign(
        erbsland::path::Path{"settings.elcl"_el},
        erbsland::path::Path{"settings.signed.elcl"_el},
        "Release Service"_el);

Interface
=========

.. doxygenclass:: erbsland::conf::SignatureSigner
    :members:
.. doxygenstruct:: erbsland::conf::SignatureSignerData
    :members:
.. doxygenclass:: erbsland::conf::Signer
    :members:
