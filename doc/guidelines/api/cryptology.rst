********************************
Cryptology Domain API Guidelines
********************************

Core Semantics
==============

.. code-block:: text

    algorithm status = disallowed, legacy verification only, or acceptable for new results
    stable identifier = persisted canonical name independent of changing recommendation metadata
    recommendation = versioned library policy ordered by security family and implementation properties

Primary Types
=============

.. code-block:: text

    HashAlgorithm // fixed-output cryptographic hash algorithm with intrinsic metadata
    HashSelector // requirements and application-policy-aware hash selection
    Hasher // copy-on-write streaming hash state
    Hmac // move-only keyed streaming authentication state
    Hkdf // algorithm-only HMAC-based extract-and-expand key derivation
    ProtectedByteBlock // copyable authenticated ciphertext with scoped plaintext access
    KeyAgreementPrivateKey // move-only protected private material with a cached ordinary public key
    KeyAgreementSharedSecret // move-only protected agreement result for direct key derivation
    SymmetricEncryptionSelector // requirements and application-policy-aware construction selection
    SymmetricEncryptor, SymmetricDecryptor // move-only streaming encryption state
    PasswordHash // opaque canonical password storage record
    PasswordHasher // password record creation and verification service
    X509Certificate // immutable certificate facade for portable and platform-backed data
    X509CertificateBundle // ordered one-or-more certificate representation
    X509ServerCertificatePolicy // explicit-anchor TLS server-authentication policy
    X509CertificateValidation // explicit accepted/rejected path-validation result without boolean conversion
    SigningPrivateKey // move-only generated or imported PKCS#8 signing key in protected storage
    X509CertificateBuilder // profile-driven certificate and certificate-request creation
    X509CertificateSigningRequest // immutable generated PKCS#10 request
    TlsRecordEncryptor, TlsRecordDecryptor // independent move-only TLS 1.3 traffic directions
    TlsConfigurationParser // side-effect-free ELCL validation and TLS material loading

Secondary Types
===============

.. code-block:: text

    HashRequirements // explicit status, security, and throughput selection requirements
    CryptographicStatus, CryptographicSecurity, HashThroughput // algorithm selection metadata
    CryptologyConfiguration // application-wide acceleration permission and downgrade-only status limits
    ProtectedDataMode // native preference, native requirement, or internal-only protected-data selection
    KeyAgreementAlgorithm, KeyAgreementPublicKey // generic agreement metadata and ordinary public material
    SymmetricEncryptionType // complete symmetric encryption construction and selection metadata
    SymmetricCipher, SymmetricEncryptionRequirements // cipher family and selection requirements
    SymmetricKey, SymmetricNonce, SymmetricIv, SymmetricTag // strongly typed cryptographic byte blocks
    PasswordHashAlgorithm // supported password-hashing algorithm identifier
    PasswordHashKey // marked application pepper and optional public rotation identifier
    PasswordHashPolicy // reviewed password-hashing construction and cost preset
    PasswordVerification // explicit verification and migration result
    X509Name, X509GeneralName, X509Extension // typed certificate fields
    X509AlgorithmIdentifier, PublicKey // algorithm and subject-public-key containers
    X509CertificateValidationFailure // structured rejection category and path context
    TlsSignatureScheme // supported TLS 1.3 signature code point and usage policy
    TlsCipherSuite // supported TLS 1.3 AEAD and HKDF-hash binding
    TlsTrafficSecret // move-only protected traffic-secret generation without plaintext access
    TlsRecordPlaintext // authenticated sensitive record content and inner content type
    TlsRecordContentType, TlsRecordErrorCategory // stable record semantics and failure classification
    Asn1Node, Asn1ObjectIdentifier // read-only raw certificate structure and stable identifiers
    SigningKeyProfile // ECDSA P-256/P-384 and RSA 2048/3072/4096 generation choices
    X509CertificateProfile // CA, TLS server, TLS client, and dual-use extension choices
    PemDerFormat // shared automatic, PEM, or DER file format choice
    TlsConfigurationEntry // owned validated label and complete parsed TLS configuration

X.509 Identity Patterns
=======================

.. code-block:: text

    o.validate(peer, network::HostName, time) -> X509CertificateValidation // compare the canonical IDNA2008 ASCII form
    o.dnsNames() -> text::StringList // expose presented IA5 ASCII names without Unicode conversion
    o.category() -> X509CertificateValidationFailureCategory // distinguish malformed SANs from ordinary mismatches
    T::generate([profile]) -> SigningPrivateKey // default to ECDSA P-256
    T::certificateAuthority(commonName) -> X509CertificateBuilder // create a safe CA configuration
    T::tlsServer(commonName) -> X509CertificateBuilder // require a DNS/IP SAN before creation
    o.createSelfSignedCertificate(key) -> X509Certificate // CA profiles only
    o.createCertificate(subjectKey, issuerCertificate, issuerKey) -> X509Certificate // validated issuance
    o.createSigningRequest(subjectKey) -> X509CertificateSigningRequest // reuse identity and requested extensions
    o.toDer() -> mem::ByteBlock // serialize using the artifact's canonical binary container
    o.toPem() -> text::String // serialize using the artifact's canonical RFC 7468 label
    o.writeToFile(path[, format]) // create a file selected by the artifact-specific suffix

Hash Selection Patterns
=======================

.. code-block:: text

    o.digestSize() -> unit::ByteLength // get the fixed digest size
    o.security()/throughput() -> T // inspect intrinsic algorithm metadata
    T::all() -> span<HashAlgorithm> // enumerate the stable algorithm set
    o.status(algorithm)/isSafe(algorithm) -> T // inspect effective global policy
    o.matches(algorithm) -> bool // test every stored selection requirement
    o.allAccepted()/matching() -> List<HashAlgorithm> // enumerate one coherent policy snapshot
    o.recommended() -> optional<HashAlgorithm> // select from one coherent policy snapshot
    o.toString() -> text::String // get the stable lowercase algorithm identifier
    T::fromString/fromStringOrThrow(text) -> T // parse an exact canonical identifier

Hashing Patterns
================

.. code-block:: text

    T(algorithm) // create active state for an algorithm
    o.isValid() -> bool // test whether hashing state is present
    o.reset()/secureErase() // begin a new stream, optionally erasing the previous message state
    o.update(bytes/text) // append exact bytes without representation conversion
    o.finalize() -> mem::ByteBlock // finalize once or return the cached digest

Message Authentication Patterns
===============================

.. code-block:: text

    T(algorithm, key) // create keyed SHA-256 or SHA-384 HMAC state
    o.isValid() -> bool // test whether keyed state is present
    o.reset() // begin another message with the same key
    o.update(bytes/text) // append exact message bytes without representation conversion
    o.finalize() -> mem::ByteBlock // return the complete authenticator in ordinary storage
    o.verify(authenticator) -> bool // compare one complete authenticator in constant time after its length matches
    o.secureErase() // erase the key and message state and become invalid

Key Derivation Patterns
=======================

.. code-block:: text

    T(algorithm) // create algorithm-only HKDF for SHA-256 or SHA-384
    o.extract(inputKeyMaterial[, salt]) -> mem::ByteBlock // sensitive PRK; empty salt means one all-zero digest
    o.extract(sharedSecret[, salt]) -> mem::ByteBlock // consume protected agreement output through scoped plaintext
    o.expand(pseudoRandomKey, info, length) -> mem::ByteBlock // create up to 255 sensitive digest blocks

Symmetric Encryption Patterns
=============================

.. code-block:: text

    o.maximumEncryptedLength(originalLength) -> unit::ByteLength // reserve for encrypted payload, excluding AEAD tag
    T(type, key, nonce/iv) // create AEAD or IV-based streaming state with exact-size parameters
    o.addAuthenticatedData(bytes) // add AEAD data before the first non-empty payload
    o.encrypt/decrypt(bytes) -> mem::ByteBlock // process the next payload bytes
    o.finalize() -> mem::ByteBlock // finish encryption or unauthenticated CBC processing
    o.tag() -> SymmetricTag // retrieve the separate AEAD tag after encryption finalization
    o.finalize(tag) -> mem::ByteBlock // authenticate AEAD decryption and all previously returned plaintext
    o.secureErase() // erase retained secrets immediately and restore the default empty state

Cryptology Configuration Patterns
=================================

.. code-block:: text

    application().cryptologyConfiguration() -> CryptologyConfiguration& // access lazy shared application policy
    o.setHardwareAccelerationEnabled(false) // force portable backends for subsequently constructed workers
    o.setMaximumStatus(algorithm, status) // install a downgrade-only selector ceiling
    o.clearMaximumStatus(algorithm)/reset() // remove one limit or restore every default
    o.status(value) -> CryptographicStatus // combine library policy with the current ceiling
    o.setProtectedDataMode(mode) // select native preference, native requirement, or internal-only protection
    o.validateProtectedDataSupport() // initialize, self-test, and lock provider selection at application startup

TLS Configuration Parser Patterns
=================================

.. code-block:: text

    T::validationRules() -> const RulesPtr& // expose the complete compiled ELCL schema
    T::version() -> Integer // expose the ELCL schema format version
    o.parse(section) -> TlsConfigurationEntry // validate one section-list entry without globals
    o.label() -> const String& // inspect the validated registry label
    o.configuration() -> const TlsConfiguration& // inspect the parsed configuration
    o.takeConfiguration() -> TlsConfiguration // transfer the profile into the application registry

Protected Data Patterns
=======================

.. code-block:: text

    T(bytes) // protect non-empty bytes with the current application provider
    o.isEmpty()/byteLength() -> T // inspect metadata without resolving the application provider
    o.unprotect() -> mem::ByteBlock // authenticate into sensitive owning plaintext storage
    o.withUnprotectedData(callback) // authenticate into temporary storage erased after callback return or exception
    o.secureErase() // erase the opaque envelope and become empty

Key Agreement Patterns
======================

.. code-block:: text

    T::generate(algorithm) -> KeyAgreementPrivateKey // generate protected private material
    T::fromBytes(algorithm, bytes) -> KeyAgreementPrivateKey // import exact bytes without a later raw accessor
    o.publicKey() -> KeyAgreementPublicKey // return cached ordinary public material without decrypting the private key
    o.agree(peerPublicKey) -> KeyAgreementSharedSecret // require the same algorithm and reject invalid results
    o.extract(sharedSecret[, salt]) -> mem::ByteBlock // derive a sensitive pseudorandom key with HKDF

Password Hashing Patterns
=========================

.. code-block:: text

    T(key[, policy]) // create a password hasher using reviewed defaults
    T::withKeyRotation(active, fallbacks[, policy]) -> T // configure an active key and bounded legacy keys
    o.hash(text::String) -> PasswordHash // create a salted canonical record from preferably marked text
    o.verify(password, record) -> PasswordVerification // return acceptance and any migration record
    T::fromString/fromStringOrThrow(text) -> PasswordHash // parse a canonical storage record
    o.toString() -> text::String // create the canonical storage representation
    o.replacementHash() -> PasswordHash // access an upgraded record after accepted verification

X.509 Certificate Patterns
==========================

.. code-block:: text

    T() // create an empty certificate facade
    o.isEmpty() -> bool // test storage state only; never imply signature, trust, hostname, or time validation
    T::fromPem/fromDer/fromFile(...) -> T // parse tolerantly by returning an empty certificate on any error
    T::fromPemOrThrow/fromDerOrThrow/fromFileOrThrow(...) -> T // parse with explicit failure diagnostics
    o.toPem() -> text::String // serialize canonical PEM
    o.toDer() -> mem::ByteBlock // return exact canonical DER
    o.writeToFile(path[, format]) // select output from a strong suffix or an explicit format
    o.subject()/issuer() -> X509Name // access ordered typed distinguished names
    o.subjectAlternativeNames() -> List<X509GeneralName> // preserve encoded GeneralName alternatives
    o.publicKey() -> PublicKey // access exact SubjectPublicKeyInfo and supported signature verification
    o.tbsCertificateDer()/signatureData() -> mem::ByteBlock // access exact certificate-signature validation inputs
    o.asn1() -> Asn1Node // inspect a read-only node owned by the parsed certificate document
    T(anchors[, intermediates]) // create an explicit-anchor server-authentication policy
    o.validate(peerCertificates, network::Host[, validationTime]) -> X509CertificateValidation // authenticate a server
    o.isAccepted()/isRejected() -> bool // inspect an explicit validation outcome; no boolean conversion is provided
    o.validatedPath() -> List<X509Certificate> // get target-to-anchor order after acceptance
    o.failure() -> optional<X509CertificateValidationFailure> // get structured context after rejection

Signature Verification Patterns
===============================

.. code-block:: text

    T::fromDer/fromDerOrThrow(der) -> T // parse an exact SubjectPublicKeyInfo or standalone AlgorithmIdentifier
    o.verifySignature(algorithm, message, signature) -> bool // false for mismatch; throw for unsupported input
    T::fromRawValue/fromRawValueOrThrow(value) -> T // parse a supported TLS SignatureScheme wire value
    o.isAllowedForCertificateVerify/isAllowedForCertificateSignature() -> bool // inspect distinct TLS usage policy
    o.verifyTlsCertificateVerifySignature(scheme, message, signature) -> bool // reject certificate-only schemes

TLS 1.3 Record Protection Patterns
==================================

.. code-block:: text

    T::fromRawValue/fromRawValueOrThrow(value) -> T // parse one supported TLS cipher-suite wire value
    T::fromBytes(hash, bytes) -> TlsTrafficSecret // protect one exact digest-sized traffic secret
    T(suite, trafficSecret) // create one independent sending or receiving record direction
    o.protect(type, content[, paddingLength]) -> mem::ByteBlock // create one complete TLSCiphertext record
    o.unprotect(record) -> TlsRecordPlaintext // authenticate one complete record before exposing content
    o.isKeyUpdateRequired() -> bool // signal the proactive or sequence-exhaustion update threshold
    o.updateApplicationTrafficKeys() // derive traffic upd, replace key/IV, and reset the sequence
    o.secureErase() // erase traffic secret, record key, IV, and counters immediately
