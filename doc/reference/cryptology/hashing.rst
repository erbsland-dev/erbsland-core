.. index::
    single: Cryptology
    single: HashAlgorithm
    single: Hasher

*********************
Cryptographic Hashing
*********************

:cpp:class:`HashAlgorithm <erbsland::cryptology::HashAlgorithm>` describes a fixed-output hash and its current
selection metadata.
:cpp:class:`Hasher <erbsland::cryptology::Hasher>` calculates a digest incrementally with copy-on-write state.
See :ref:`cryptology-hashing` for lifecycle, text hashing, selection, and persistence guidance.

Interface
=========

.. doxygenenum:: erbsland::cryptology::CryptographicSecurity
.. doxygenenum:: erbsland::cryptology::CryptographicStatus
.. doxygenclass:: erbsland::cryptology::HashAlgorithm
    :members:
.. doxygenclass:: erbsland::cryptology::Hasher
    :members:
.. doxygenstruct:: erbsland::cryptology::HashRequirements
    :members:
.. doxygenenum:: erbsland::cryptology::HashThroughput
