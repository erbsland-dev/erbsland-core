.. index::
    single: Configuration Signature Validation

**********************************
Configuration Signature Validation
**********************************

When a document contains signature metadata, the parser calculates the document digest and passes the signature, digest,
signing-person text, and source information to the configured
:cpp:class:`erbsland::conf::SignatureValidator <erbsland::conf::SignatureValidator>`.
The validator is responsible for interpreting the signature text and checking it against the application's trust policy.

No validator is installed by default.
Unsigned documents can then be parsed, while signed documents are rejected because their authenticity cannot be
established.
Install a validator with
:cpp:func:`erbsland::conf::Parser::setSignatureValidator <erbsland::conf::Parser::setSignatureValidator>` before parsing signed input.

Validation is performed on the exact signature data produced by the parser.
Implementations must not normalize digest or signature bytes, and should return the defined validation result or throw a
configuration error when the failure needs additional diagnostic context.

Interface
=========

.. doxygenclass:: erbsland::conf::SignatureValidator
    :members:

.. doxygentypedef:: erbsland::conf::SignatureValidatorPtr
.. doxygenstruct:: erbsland::conf::SignatureValidatorData
    :members:
.. doxygenenum:: erbsland::conf::SignatureValidatorResult
