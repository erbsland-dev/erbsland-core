.. index::
    !single: TLS Certificate Creation
    single: Certificate Authority; Creating
    single: Certificate Signing Request; Creating
    single: PKCS#8; Encrypting

**************************************
Creating TLS Certificates and Requests
**************************************

Applications often need two versions of the same identity workflow.
A test setup creates its own certificate authority and issues server and client certificates immediately, while an
enterprise setup creates a private key and sends a certificate signing request to the organization's CA. The
profile-driven builder supports both paths with the same subject and extension configuration.

Choosing a Key Profile
======================

:cpp:func:`SigningPrivateKey::generate() <erbsland::cryptology::SigningPrivateKey::generate>` defaults to ECDSA P-256,
which is a compact and broadly interoperable choice for new TLS identities.
Select P-384 when the surrounding system requires its higher security strength, or an RSA profile when compatibility
with RSA-only software matters.

.. code-block:: cpp

    using namespace el::cryptology;

    auto defaultKey = SigningPrivateKey::generate();
    auto p384Key = SigningPrivateKey::generate(SigningKeyProfile::EcdsaP384);
    auto rsaKey = SigningPrivateKey::generate(SigningKeyProfile::Rsa3072);

The RSA choices fix the public exponent and modulus size.
The API intentionally has no controls for primes, exponent, curve parameters, signature padding, or digest selection.

Creating a Test Certificate Hierarchy
=====================================

A self-signed certificate is available only from the certificate-authority profile.
The following root permits one subordinate CA level; the intermediate keeps the default path length of zero and can
therefore issue leaves but no further CA.

.. code-block:: cpp

    using namespace el::cryptology;
    using namespace el::text::literals;

    auto rootKey = SigningPrivateKey::generate();
    auto root = X509CertificateBuilder::certificateAuthority("Example Test Root"_el)
                    .setOrganization("Example Application"_el)
                    .setCaPathLength(1)
                    .createSelfSignedCertificate(rootKey);

    auto intermediateKey = SigningPrivateKey::generate();
    auto intermediate = X509CertificateBuilder::certificateAuthority("Example Test Issuing CA"_el)
                            .setOrganization("Example Application"_el)
                            .createCertificate(intermediateKey, root, rootKey);

For a server, add every DNS name and literal IP address through which clients connect.
The common name is descriptive but does not replace a SAN during service-identity validation.

.. code-block:: cpp

    auto serverKey = SigningPrivateKey::generate(SigningKeyProfile::EcdsaP384);
    auto serverBuilder = X509CertificateBuilder::tlsServer("api.example.test"_el);
    serverBuilder.setOrganization("Example Application"_el)
        .addDnsName("api.example.test"_el)
        .addDnsName("localhost"_el)
        .addIpAddress(el::network::IpAddress::fromStringOrThrow("127.0.0.1"_el));
    auto server = serverBuilder.createCertificate(serverKey, intermediate, intermediateKey);

    auto clientKey = SigningPrivateKey::generate();
    auto client = X509CertificateBuilder::tlsClient("integration-client"_el)
                      .setOrganization("Example Application"_el)
                      .createCertificate(clientKey, intermediate, intermediateKey);

Use ``tlsServerAndClient()`` only when the same identity really serves both roles.
It requires a SAN like the server profile and includes both TLS purposes.

Writing the Setup Safely
========================

File output infers PEM or DER from an artifact-specific suffix.
Private-key files are created with user-only access, and no write operation replaces an existing file.
This makes accidental key replacement a visible setup error.

.. code-block:: cpp

    rootKey.writeToFile(outputDirectory / "root-ca.key"_el);
    root.writeToFile(outputDirectory / "root-ca.crt"_el);
    intermediateKey.writeToFile(outputDirectory / "issuing-ca.key"_el);
    intermediate.writeToFile(outputDirectory / "issuing-ca.crt"_el);
    serverKey.writeToFile(outputDirectory / "server.key"_el);
    server.writeToFile(outputDirectory / "server.crt"_el);
    clientKey.writeToFile(outputDirectory / "client.key"_el);
    client.writeToFile(outputDirectory / "client.crt"_el);

``.key`` and ``.crt`` select PEM in this example.
Use ``.p8`` and ``.cer`` for DER, or pass :cpp:enum:`PemDerFormat <erbsland::cryptology::PemDerFormat>` explicitly when
a naming convention does not use a recognized suffix.

Protecting a Private Key with a Password
========================================

Unencrypted keys are convenient for unattended test processes whose directory is already protected.
When the key must be protected at rest, write EncryptedPrivateKeyInfo instead.

.. code-block:: cpp

    auto password = el::String{"correct horse battery staple"_el};
    password.markAsSensitive();
    serverKey.writeEncryptedToFile(outputDirectory / "server-encrypted.key"_el, password);

    auto loadedKey = SigningPrivateKey::fromEncryptedFileOrThrow(
        outputDirectory / "server-encrypted.key"_el, password);

An empty password is rejected.
Keep password input marked sensitive and erase it as soon as the application no longer needs it.
The encrypted-key reader deliberately gives the same failure diagnostic for a wrong password, invalid padding, or
malformed plaintext.

Creating an Enterprise Signing Request
======================================

For enterprise enrollment, keep the private key inside the application and send only the request.
The request contains the subject public key plus one ``extensionRequest`` attribute carrying the profile's SAN, Key
Usage, Extended Key Usage, and Basic Constraints request.

.. code-block:: cpp

    auto enterpriseKey = SigningPrivateKey::generate(SigningKeyProfile::Rsa3072);
    auto requestBuilder = X509CertificateBuilder::tlsServer("api.corp.example"_el);
    requestBuilder.setCountry("CH"_el)
        .setOrganization("Example AG"_el)
        .setOrganizationalUnit("Platform Services"_el)
        .addDnsName("api.corp.example"_el);
    auto request = requestBuilder.createSigningRequest(enterpriseKey);

    enterpriseKey.writeEncryptedToFile(outputDirectory / "api.key"_el, password);
    request.writeToFile(outputDirectory / "api.csr"_el);

The enterprise CA decides the final issuer, serial, validity, and extensions.
After receiving the certificate, parse it and verify that its public key matches ``enterpriseKey.publicKey()`` before
installing the identity.

Validity and Reissuance
=======================

Default validity begins five minutes in the past to tolerate modest clock differences.
CA certificates last ten calendar years and leaf certificates last 397 days unless the builder specifies exact times or
a lifetime.
Issuer-signed defaults are clipped to the issuer range; explicit ranges outside that range are rejected rather than
silently changed.

Use ``fromCertificate()`` when renewing an existing identity.
It copies supported subject and SAN data while generating a new serial, validity, key identifiers, issuer, and
signature.
Unknown noncritical extensions are retained, but an unknown critical extension stops reissuance so its semantics cannot
be lost accidentally.
