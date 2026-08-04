..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Cryptology
    single: X.509 Certificate
    single: PEM Certificate
    single: DER Certificate

******************
X.509 Certificates
******************

:cpp:class:`X509Certificate <erbsland::cryptology::X509Certificate>` is an immutable certificate facade.
It reads and writes strict PEM and canonical DER, exposes commonly needed RFC 5280 fields as typed values, and retains
the exact encoded certificate, ``TBSCertificate``, signature, and ``SubjectPublicKeyInfo`` bytes needed for signature
verification.
An empty certificate is only a missing-storage state; it says nothing about trust, hostname matching, signature
validity, intended purpose, or the current time.

Reading and Writing
===================

The non-throwing ``fromPem()``, ``fromDer()``, and ``fromFile()`` factories return an empty certificate for every
failure.
The matching ``OrThrow`` factories preserve parse, range, and file diagnostics.
Singular factories require exactly one certificate, while
:cpp:class:`X509CertificateBundle <erbsland::cryptology::X509CertificateBundle>` accepts one or more ordered PEM
blocks.
A bundle can produce DER only when it contains exactly one certificate.

:cpp:enum:`X509CertificateFormat <erbsland::cryptology::X509CertificateFormat>` controls file conversion.
``.pem`` is a strong PEM hint and ``.der`` is a strong DER hint.
The common ambiguous ``.crt`` and ``.cer`` suffixes are sniffed when reading; when writing, ``.crt`` selects PEM and
``.cer`` selects DER.
Unknown output suffixes require an explicit format.

PEM parsing accepts only exact ``CERTIFICATE`` boundaries and strict Base64, with whitespace outside blocks.
PEM output uses the canonical boundary and 64-character Base64 lines.
DER parsing requires one complete, canonical, definite-length value and rejects trailing data.
The DER implementation follows ITU-T X.690, while certificate schema and profile checks follow RFC 5280 and PEM
boundaries follow RFC 7468. DER parse errors carry the byte index at which the malformed or noncanonical value was
detected; PEM structure errors carry a code-point index in the decoded text.
Fixed limits bound the encoded size, tree depth, node count, certificate count, and PEM input size before allocation or
recursion can grow without control.

Typed Certificate Data
======================

:cpp:class:`X509Name <erbsland::cryptology::X509Name>` preserves the ordered relative distinguished names and their
attributes.
:cpp:class:`X509GeneralName <erbsland::cryptology::X509GeneralName>` represents DNS, email, URI, IP-address,
directory-name, and registered-ID alternatives while retaining unsupported alternatives as raw nodes.
:cpp:class:`X509Extension <erbsland::cryptology::X509Extension>` preserves every extension in encoded order.
Convenience accessors decode Subject Alternative Name, Subject and Authority Key Identifier, Basic Constraints, Key
Usage, and Extended Key Usage.

:cpp:class:`X509AlgorithmIdentifier <erbsland::cryptology::X509AlgorithmIdentifier>` retains an algorithm OID and its
optional parameters.
:cpp:class:`PublicKey <erbsland::cryptology::PublicKey>` retains the exact ``SubjectPublicKeyInfo`` and key BIT STRING
and verifies supported RSA, ECDSA, or Ed25519 signatures over caller-supplied exact message bytes.
Both types can also parse their standalone canonical DER representation, which lets certificate-chain validation use an
issuer key and a child's complete signature ``AlgorithmIdentifier`` without reconstructing either value.

RSA Signature Verification
==========================

RSA verification follows RSAVP1 and the EMSA verification procedures in RFC 8017 sections 5.2.2, 8, and 9. It accepts
RSASSA-PSS and RSASSA-PKCS1-v1_5 with SHA-256 or SHA-384. PSS algorithm and key parameters follow RFC 4055 sections 3.1
and 3.3: the message and MGF1 hashes must match, only trailer field 1 is accepted, and the salt length cannot exceed the
selected hash length.
General-purpose RSA public-key encoding follows RFC 3279 section 2.3.1.

The RSA modulus must be odd and between 2048 and 8192 bits.
The public exponent must be odd, greater than 65536, and at most 256 bits.
These checks implement the lower security bounds from FIPS 186-5 section 5.1 and impose a fixed upper work and storage
bound before modular arithmetic starts.
The verifier uses a dedicated, statically bounded integer representation rather than the library's general-purpose
arithmetic types.
Its running time can depend on the modulus, public exponent, and signature because every RSA operand in verification is
public; no private exponent, prime, secret scalar, or other secret state is created or retained.

ECDSA Signature Verification
============================

ECDSA verification follows FIPS 186-5 section 6.4.2 and supports exactly P-256 with SHA-256 and P-384 with SHA-384.
Curve parameters come from NIST SP 800-186 sections 3.2.1.3--3.2.1.4, public-key encoding follows RFC 5480 sections
2.1.1--2.2, and signature identifiers follow RFC 5758 section 3.2. Signature values are one complete canonical DER
``SEQUENCE`` containing exactly two positive ``INTEGER`` values.
Mathematically valid high-S signatures are accepted; this certificate-verification API does not impose a low-S
normalization policy.

The key must use ``id-ecPublicKey`` with a present named-curve OID for one of the two supported curves.
Implicit, explicit, missing, and unsupported curve parameters are rejected.
Public points may use the exact uncompressed form or the ``0x02`` /``0x03`` compressed forms.
Coordinates must be in the field and satisfy the curve equation.
Compressed points recover ``y`` with the fixed ``(p+1)/4`` exponent available because both primes are 3 modulo 4, verify
the square, and select the encoded parity.
Both curves have cofactor one, so a finite point on the curve is already in the required prime-order subgroup.

Field elements and scalars use a fixed maximum of twelve 32-bit limbs.
Jacobian point operations avoid inversions during public scalar multiplication, while fixed Montgomery precomputations
bound modular arithmetic.
The verification double-scalar calculation is deliberately variable-time: the branch schedule may depend on ``u``,
``v``, the public key, and signature, all of which are public verification data.
Verification creates no private scalar or other secret state, so no secret-erasure lifecycle is involved.
A future ECDHE implementation may reuse constants and field arithmetic only; its secret-scalar multiplication must use a
separately reviewed constant-schedule path and explicitly erase private intermediates.

Ed25519 Signature Verification
==============================

Ed25519 verification follows RFC 8032 sections 5.1.2--5.1.4 and 5.1.7. It supports only pure Ed25519: Ed25519ctx,
Ed25519ph, and Ed448 are distinct algorithms and are not accepted.
Public-key and signature identifiers use ``id-Ed25519`` from RFC 8410 sections 3, 4, and 6, and their
``AlgorithmIdentifier`` parameters must be absent.
A DER ``NULL`` parameter is rejected rather than treated as an interoperable alternative.
The public key is exactly one 32-octet encoded point, and a signature is the raw 64-octet ``ENC(R) || ENC(S)`` value
without additional ASN.1 wrapping.

Point decoding requires the canonical little-endian ``y`` representative below ``2^255-19``, reconstructs ``x`` with the
RFC 8032 square-root procedure, and rejects negative zero or values that are not on the curve.
The scalar ``S`` must be below the subgroup order ``L``; this prevents the signature malleability described by RFC 8032
section 8.4. The public key must be a nonidentity point in the prime-order subgroup.
The signature point is decoded canonically, and verification uses the sufficient non-cofactored equation
``[S]B = R + [k]A`` explicitly permitted by RFC 8032 section 5.1.7. This exact equation also rejects a signature point
with a nontrivial torsion component.

Field elements use ten alternating 26/25-bit limbs, while SHA-512 reduction uses eight 32-bit scalar limbs.
These fixed representations bound all storage and arithmetic without a general-purpose big integer or nonportable
128-bit integer.
Scalar multiplication is deliberately variable-time because ``S``, ``k``, the public key, and signature point are all
public verification inputs.
Verification creates no private scalar or other secret state, so no secret-erasure lifecycle is involved.

TLS 1.3 Signature Schemes
=========================

:cpp:class:`TlsSignatureScheme <erbsland::cryptology::TlsSignatureScheme>` represents the supported two-octet
``SignatureScheme`` values from RFC 8446 section 4.2.3. It supports ECDSA with P-256/SHA-256 or P-384/SHA-384, pure
Ed25519, RSA-PSS-RSAE and RSA-PSS-PSS with SHA-256 or SHA-384, plus certificate-only RSA-PKCS1/SHA-256 and
RSA-PKCS1/SHA-384. The raw-value factories reject every unsupported registry value rather than preserving an unknown
construction.

The certificate and TLS-message policies are intentionally separate.
PKCS#1 v1.5 schemes can describe signatures appearing in certificates but are never allowed for a TLS 1.3
``CertificateVerify`` message.
RSA-PSS-RSAE requires a SubjectPublicKeyInfo using ``rsaEncryption``, while RSA-PSS-PSS requires ``id-RSASSA-PSS``.
Both PSS variants use the selected SHA-2 algorithm for the message and MGF1, set the salt length to the digest length,
and use trailer field 1 as required by RFC 8446 section 4.2.3.

:cpp:func:`PublicKey::verifyTlsCertificateVerifySignature()
<erbsland::cryptology::PublicKey::verifyTlsCertificateVerifySignature>` verifies the exact caller-supplied content
covered by the signature.
The caller remains responsible for constructing the 64 space octets, context string, zero separator, and transcript hash
specified by RFC 8446 section 4.4.3. The scheme, public key, signed content, and signature are all public verification
inputs; the mapping creates no secret state and has no secret-erasure lifecycle.

Verification Results and Policy
===============================

``verifySignature()`` returns ``false`` for a well-formed supported signature that does not verify.
It throws a parse error for malformed, unsupported, or out-of-policy key and algorithm encodings, so callers cannot
silently treat an unsupported construction as an ordinary bad signature.
PKCS#1 v1.5 support exists for certificate-chain compatibility.
RFC 8446 section 4.2.3 permits these schemes for certificate signatures but not for TLS 1.3 ``CertificateVerify``; the
dedicated TLS verification operation enforces this distinction before signature dispatch.
Signing and private-key loading remain separate work.
Portable explicit-anchor server authentication is described below; platform trust-store policy remains deferred.

Explicit-Anchor Server Authentication
=====================================

:cpp:class:`X509ServerCertificatePolicy <erbsland::cryptology::X509ServerCertificatePolicy>` authenticates an
already-parsed peer certificate set for one :cpp:class:`network::Host <erbsland::network::Host>` reference identity.
The first peer certificate is the target and all remaining peer certificates are unordered issuer candidates.
Configured intermediates provide a second candidate source.
Explicit certificate trust anchors are mandatory; the policy does not consult a platform trust store.

Path construction follows RFC 4158 sections 2.4 and 5. It performs deterministic depth-first traversal, preferring peer
intermediates and then configured intermediates in their source order before anchors, while trying alternate branches
after a failure.
Certificates are deduplicated by exact DER.
Issuer linkage currently requires byte-identical canonical DER for the child's issuer and candidate's subject.
Authority and Subject Key Identifiers eliminate a candidate only when both are present and unequal.
RFC 4518 distinguished-name equivalence is intentionally not implemented in this initial portable policy.

An accepted path terminates only at an explicitly configured anchor and includes that anchor in target-to-anchor order.
The anchor supplies trusted subject-name and public-key input; its self-signature, validity, extensions, Basic
Constraints, and Key Usage are not checked.
If the target certificate itself is explicitly anchored, its end-entity profile, validity, purpose, critical extensions,
and service identity are still checked.

Path Validation and Purpose
===========================

Path validation follows RFC 5280 sections 4.1.2.5, 4.2.1.3, 4.2.1.9, 4.2.1.12, and 6.1. Every child signature covers the
retained exact ``TBSCertificate`` and must have zero unused BIT STRING bits.
Every non-anchor certificate must have no compatible-parser profile issue and must satisfy the inclusive
``notBefore <= validationTime <= notAfter`` interval.
The caller may supply a validation time explicitly; the convenience overload uses ``DateTime::now()``.

An intermediate requires a critical Basic Constraints extension with ``cA=true``.
If Key Usage exists it must include ``keyCertSign``; if Extended Key Usage exists it must include ``serverAuth`` or
``anyExtendedKeyUsage``.
``pathLenConstraint`` counts only non-self-issued intermediate CA certificates below the constrained CA. The target must
not assert ``cA=true``; an existing Key Usage must include ``digitalSignature`` and an existing Extended Key Usage must
permit server authentication, as required for the TLS certificate role by RFC 8446 section 4.4.2.2.

The validator recognizes the currently decoded Subject Key Identifier, Authority Key Identifier, Subject Alternative
Name, Basic Constraints, Key Usage, and Extended Key Usage extensions.
Every other critical extension rejects the path; unknown noncritical extensions are retained by the certificate parser
and ignored by this policy.
Revocation, name constraints, policy constraints, and certificate policies are unavailable in this increment and are
never reported as successfully checked.

DNS and IP Service Identities
=============================

Service-identity matching follows RFC 9525 section 6 and RFC 9549 and uses Subject Alternative Name only.
A
:cpp:class:`network::HostName <erbsland::network::HostName>` selects DNS-ID matching, while a
:cpp:class:`network::IpAddress <erbsland::network::IpAddress>` selects IP-ID matching. Common Name is never a fallback,
and DNS and IP alternatives cannot satisfy a reference of the other type.

DNS references arrive as validated :cpp:class:`network::HostName <erbsland::network::HostName>` values and are converted
to their canonical strict-IDNA2008 ASCII form before matching.
Presented ``dNSName`` alternatives remain ASCII as required by X.509; each label and ``xn--`` A-label is validated,
round-tripped, and compared label by label in canonical A-label form with ASCII case folding.
Malformed or disallowed presented identities do not prevent a later valid SAN from matching.
When no valid SAN matches, a relevant malformed identity produces ``InvalidPresentedIdentity`` with its underlying IDNA
reason; ordinary valid mismatches produce ``ServerIdentityMismatch``.
A presented wildcard is valid only as the complete leftmost label, occurs once, and matches exactly one reference label.
IP subject alternatives compare through the canonical byte representation of ``IpAddress``.

Results, Diagnostics, and Bounds
================================

:cpp:class:`X509CertificateValidation <erbsland::cryptology::X509CertificateValidation>` deliberately has no boolean
conversion.
``isAccepted()`` and ``isRejected()`` expose the outcome.
Acceptance carries the validated target-to-anchor path.
Rejection carries an
:cpp:class:`X509CertificateValidationFailure <erbsland::cryptology::X509CertificateValidationFailure>` with a stable
category, optional affected certificate and issuer candidate, partial path, and human-readable diagnostic.
No outcome uses an empty certificate as a signal.

Attacker-controlled work is bounded to 256 aggregate unique certificates, 16 certificates in one prospective path, and
1024 signature-verification attempts.
Repeated certificates are rejected as path loops and verified edges are cached.
Exhausting any bound produces an explicit resource-limit failure category.

Strict and Compatible Profiles
==============================

:cpp:enum:`X509CertificateProfileMode <erbsland::cryptology::X509CertificateProfileMode>` affects certificate-profile
checks, never DER safety or canonicality.
Strict mode rejects a certificate when, for example, its inner and outer signature algorithm identifiers disagree.
Compatible mode retains supported interoperability exceptions as
:cpp:class:`X509CertificateProfileIssue <erbsland::cryptology::X509CertificateProfileIssue>` values so callers can
make an explicit policy decision.
Malformed ASN.1, noncanonical DER, invalid lengths, invalid primitive encodings, and resource-limit violations remain
errors in both modes.

Raw ASN.1 Views and Backends
============================

:cpp:class:`Asn1Node <erbsland::cryptology::Asn1Node>` is a read-only view into a successfully parsed certificate.
Each node independently retains its shared immutable DER storage and exposes its exact encoding, content octets,
children, tag, and selected primitive conversions.
There is deliberately no public factory for parsing arbitrary ASN.1 with this view API.

Certificate shared data uses a virtual backend contract.
Portable certificates store parsed values directly; future Windows and macOS integrations can retain a native
certificate-store reference and materialize the same immutable portable view only when an accessor or serialization
operation needs it.
Native handles are not exposed by the public facade.

Interface
=========

.. doxygenclass:: erbsland::cryptology::Asn1Node
    :members:
.. doxygenclass:: erbsland::cryptology::Asn1ObjectIdentifier
    :members:
.. doxygenenum:: erbsland::cryptology::Asn1TagClass
.. doxygenenum:: erbsland::cryptology::Asn1UniversalType
.. doxygenclass:: erbsland::cryptology::PublicKey
    :members:
.. doxygenclass:: erbsland::cryptology::TlsSignatureScheme
    :members:
.. doxygenclass:: erbsland::cryptology::X509AlgorithmIdentifier
    :members:
.. doxygenclass:: erbsland::cryptology::X509BasicConstraints
    :members:
.. doxygenclass:: erbsland::cryptology::X509Certificate
    :members:
.. doxygenclass:: erbsland::cryptology::X509CertificateBundle
    :members:
.. doxygenenum:: erbsland::cryptology::X509CertificateFormat
.. doxygenclass:: erbsland::cryptology::X509CertificateProfileIssue
    :members:
.. doxygenenum:: erbsland::cryptology::X509CertificateProfileIssueCategory
.. doxygenenum:: erbsland::cryptology::X509CertificateProfileMode
.. doxygenclass:: erbsland::cryptology::X509CertificateValidation
    :members:
.. doxygenclass:: erbsland::cryptology::X509CertificateValidationFailure
    :members:
.. doxygenenum:: erbsland::cryptology::X509CertificateValidationFailureCategory
.. doxygenclass:: erbsland::cryptology::X509Extension
    :members:
.. doxygenclass:: erbsland::cryptology::X509GeneralName
    :members:
.. doxygenenum:: erbsland::cryptology::X509KeyUsage

.. doxygentypedef:: erbsland::cryptology::X509KeyUsages
.. doxygenclass:: erbsland::cryptology::X509Name
    :members:
.. doxygenclass:: erbsland::cryptology::X509NameAttribute
    :members:
.. doxygenclass:: erbsland::cryptology::X509RelativeDistinguishedName
    :members:
.. doxygenclass:: erbsland::cryptology::X509ServerCertificatePolicy
    :members:
.. doxygenenum:: erbsland::cryptology::X509Version
