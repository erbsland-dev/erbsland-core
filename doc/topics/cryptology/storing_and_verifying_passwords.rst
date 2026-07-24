..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Storing and Verifying Passwords
    single: Password
    single: Password Hash
    single: Password Verification
    single: Pepper
    single: Password Key Rotation
    single: String; Sensitive
    single: Unicode Normalization
    single: Rate Limiting

*******************************
Storing and Verifying Passwords
*******************************

This page presents a complete local password-storage workflow using the compiled ``password-handler`` demo.
It reads passwords through a masked terminal editor, keeps active password values in protected storage, persists
canonical password-hash records, and migrates records when a pepper changes.

The demo is intentionally small enough to inspect.
Its two local ELCL files make the persistence boundary visible, but they are not a production authentication database or
secret store.

Run the Demo
============

The executable provides five commands:

.. code-block:: console

    $ password-handler list-users
    $ password-handler add-user alice
    $ password-handler login alice
    $ password-handler set-password alice
    $ password-handler remove-user alice

By default, it stores the pepper in ``~/.password-handler/pepper.elcl`` and the user records in
``~/.password-handler/users.elcl``.
The home directory comes from
:cpp:func:`Path::userHomeDirectoryOrThrow() <erbsland::path::Path::userHomeDirectoryOrThrow>`, which uses the
operating-system account or profile database rather than an environment variable.
Use ``--pepper/-p`` and ``--database/-d`` after a command to select explicit paths.

The first command creates both files when neither exists.
If only the pepper exists, it creates an empty user database.
If a database exists without its pepper, it stops instead of generating a key that cannot verify the stored records.
Both options must identify different files.

Create the Local Pepper
=======================

:cpp:class:`PasswordHasher <erbsland::cryptology::PasswordHasher>` normally uses a
:cpp:class:`PasswordHashKey <erbsland::cryptology::PasswordHashKey>` containing at least 32 bytes of secret material.
This application key is commonly called a *pepper*.
A stolen password database then does not contain everything required to test password guesses.

The demo generates 32 bytes with :cpp:func:`Application::secureRandom() <erbsland::core::Application::secureRandom>`.
The secure generator marks the returned :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` allocation before filling it.
It gives the key a public random identifier so future records can identify the pepper that protected them.

.. erbsland-demo::
    :source: cryptology/PasswordHandler/src/PepperStore_create.cpp
    :source-sha256: 1e341e7eb7cc8466456a0467bd0048f20e322dbe65c9519256be65b24e9fc7f3

.. code-block:: cpp

    /// Generate and persist the initial identified password-hash pepper.
    ///
    /// The key identifier is public, random, and filename-safe. The 32 random bytes are generated directly in protected
    /// storage. The ELCL file uses a native byte literal and collision-stop creation so an existing key is never silently
    /// replaced.
    void PepperStore::create(const el::Path &path) {
        constexpr auto pepperLength = el::ByteLength{32U};
        static const auto identifierCharacters = el::CharSet{"abcdefghijklmnopqrstuvwxyz0123456789"_el};
        const auto identifier = el::String::fromJoined(
            {"key-"_el, el::application().secureRandom().buildString(el::CpLength{16U}, identifierCharacters)});

        const auto protectedBytes = el::application().secureRandom().buildByteBlock(pepperLength);
        const auto activeKey = el::PasswordHashKey::identified(identifier, protectedBytes);
        auto output = el::StringEditor{"@version: \"1.0\"\n\n[Pepper]\n"_el};
        appendElclText(output, "Identifier"_el, identifier);
        output.append(el::StringFormat{"Key: <{}>\n"_el}.build(protectedBytes));
        static_cast<void>(activeKey);
        writeStorageFile(path, el::String{output}, el::PathCollisionMode::Stop);
    }

.. erbsland-demo-end::

The key is written as an ELCL native byte literal in a separate, user-only file.
This is useful for demonstrating loading, backup, and rotation on one machine.
A production service should obtain the pepper from a secret manager, hardware-backed facility, or operating-system key
store and keep it outside the password database and ordinary deployment configuration.
Back it up separately: losing every applicable pepper makes the corresponding records unverifiable.

Load Pepper Material
====================

Every storage read is parsed through :cpp:class:`conf::Parser <erbsland::conf::Parser>`.
The loader accepts one identified active key and a list of historical fallbacks.
It rejects unexpected fields, incorrect value types, invalid key lengths, empty identifiers, duplicate identifiers, and
more than one identifier-less fallback.
The pepper file is opened with sensitive stream settings, so file and decoder buffers are securely erased.
The demo currently hands an ordinary source string to the configuration parser.
Ordinary byte values returned by the parser are copied into protected storage and erased immediately.

.. erbsland-demo::
    :source: cryptology/PasswordHandler/src/PepperStore.cpp
    :source-sha256: e1efed75f61d9402ce00c5bef02c449e50f303dd5907f1d702024dce092f2ba9

.. code-block:: cpp

    /// Load the active and fallback peppers from a strict ELCL document.
    ///
    /// Every byte value is marked as sensitive at its shared allocation. The active key has a public identifier. Fallback
    /// keys may be identified or, for legacy records, contain one unnamed key.
    auto PepperStore::loadHasher(const el::Path &path) -> el::PasswordHasher {
        el::conf::DocumentPtr document;
        auto source = el::String{};
        try {
            auto readOptions = el::path::PathReadTextOptions{};
            readOptions.setSensitive(true);
            source = path.content().readTextOrThrow(readOptions);
            document = el::conf::Parser{}.parseTextOrThrow(source);
        } catch (const el::Exception &) {
            throwInvalidPepper(path, "The file is not a valid ELCL document."_el);
        }
        if (document->size() != 1U || !document->hasValue("Pepper"_el)) {
            throwInvalidPepper(path, "The document must contain only one Pepper section."_el);
        }
        const auto pepper = document->valueOrThrow("Pepper"_el);
        const auto fallbackValue = pepper->value("Fallback"_el);
        const auto expectedSize = fallbackValue == nullptr ? 2U : 3U;
        if (pepper->size() != expectedSize || !pepper->hasValue("Identifier"_el) || !pepper->hasValue("Key"_el)) {
            throwInvalidPepper(path, "Pepper must contain Identifier and Key, plus an optional Fallback section list."_el);
        }

        el::String activeIdentifier;
        try {
            activeIdentifier = pepper->getTextOrThrow("Identifier"_el);
        } catch (const el::Exception &) {
            throwInvalidPepper(path, "The active key Identifier must be text."_el);
        }
        if (activeIdentifier.isEmpty()) {
            throwInvalidPepper(path, "The active key Identifier must not be empty."_el);
        }
        auto activeKey = el::PasswordHashKey::identified(activeIdentifier, readKeyBytes(pepper, path));
        auto fallbackKeys = el::List<el::PasswordHashKey>{};

        if (fallbackValue != nullptr) {
            if (fallbackValue->type() != el::conf::ValueType::SectionList) {
                throwInvalidPepper(path, "Pepper.Fallback must be a section list."_el);
            }
            for (const auto &entry : *fallbackValue) {
                const auto hasIdentifier = entry->hasValue("Identifier"_el);
                const auto expectedEntrySize = hasIdentifier ? 2U : 1U;
                if (entry->size() != expectedEntrySize || !entry->hasValue("Key"_el)) {
                    throwInvalidPepper(path, "Fallback entries contain only an optional Identifier and a required Key."_el);
                }
                const auto keyBytes = readKeyBytes(entry, path);
                if (hasIdentifier) {
                    el::String identifier;
                    try {
                        identifier = entry->getTextOrThrow("Identifier"_el);
                    } catch (const el::Exception &) {
                        throwInvalidPepper(path, "A fallback Identifier must be text."_el);
                    }
                    if (identifier.isEmpty()) {
                        throwInvalidPepper(path, "A fallback Identifier must not be empty."_el);
                    }
                    fallbackKeys.append(el::PasswordHashKey::identified(identifier, keyBytes));
                } else {
                    fallbackKeys.append(el::PasswordHashKey{keyBytes});
                }
            }
        }
        // Let PasswordHasher validate identifier uniqueness and the single unnamed-fallback rule.
        try {
            return buildPasswordHasher(activeKey, fallbackKeys);
        } catch (const el::Exception &) {
            throwInvalidPepper(path, "The active and fallback key identifiers do not form a valid rotation set."_el);
        }
    }

.. erbsland-demo-end::

Read a Password
===============

Password commands require an interactive terminal and reject redirected input before creating or modifying storage.
Each prompt constructs a fresh blocking :cpp:class:`cterm::ReadSecret <erbsland::cterm::ReadSecret>`.
It is a cleaned-up single-line editor with a fixed bullet mask and a 1024-code-point limit.
Escape cancels the command without changing user records.

New passwords are entered twice and compared as ordinary strings.
They require at least 15 Unicode code points and otherwise preserve the input exactly.
This follows the length-oriented approach in `NIST SP 800-63B-4 <https://pages.nist.gov/800-63-4/sp800-63b.html>`_
without imposing composition rules.

.. erbsland-demo::
    :source: cryptology/PasswordHandler/src/PasswordPrompt.cpp
    :source-sha256: 182c55a13e85e7985a1c2b4be165771e9877c247de81a3eade455486a4112f25

.. code-block:: cpp

    /// Read a new password twice and compare the protected values.
    ///
    /// New passwords require at least 15 Unicode code points. Spaces and Unicode are accepted exactly as entered without
    /// trimming, case folding, composition rules, or normalization.
    auto PasswordPrompt::readNewPassword(const el::cterm::TerminalPtr &terminal) -> el::String {
        auto password = read(terminal, "Enter new password"_el);
        if (password.characterLength() < el::CpLength{15U}) {
            throw el::ApplicationError{el::core::ApplicationErrorContext{
                "Password is too short"_el, "New passwords must contain at least 15 Unicode code points."_el}};
        }
        const auto confirmation = read(terminal, "Confirm new password"_el);
        if (password != confirmation) {
            throw el::ApplicationError{el::core::ApplicationErrorContext{
                "Passwords do not match"_el, "Enter the same password in both prompts."_el}};
        }
        return password;
    }

    /// Read one masked password through a fresh blocking `ReadSecret`.
    auto PasswordPrompt::read(const el::cterm::TerminalPtr &terminal, const el::String &title) -> el::String {
        auto options = el::cterm::ReadLineOptions{}.setTitle(title).setPlaceholder("Password"_el);
        const auto editor = el::cterm::ReadSecret::create(terminal, options);
        auto result = editor->waitForInput();
        if (!result.isCommitted()) {
            throw el::ApplicationError{el::core::ApplicationErrorContext{
                "Password entry cancelled"_el, "No password-storage changes were made."_el}};
        }
        return result.takeData();
    }

.. erbsland-demo-end::

``ReadSecret`` keeps editing state in fixed storage, displays only bullets, wipes vacated and final editor storage, and
commits directly to a marked :cpp:type:`String <erbsland::text::String>`.
It also purges pending terminal key state when entering and leaving the editor.

These guarantees cover library-owned process buffers, not every copy in the system.
Kernel and terminal-driver queues, register remnants, swapping, crash dumps, and a compromised process remain outside
the boundary.

Hash and Store a Password
=========================

The demo checks the exact username before prompting.
It hashes the protected password with the active pepper and stores only
:cpp:func:`PasswordHash::toString() <erbsland::cryptology::PasswordHash::toString>`.

.. erbsland-demo::
    :source: cryptology/PasswordHandler/src/PasswordHandlerApp_addUser.cpp
    :source-sha256: a926ba221d330e7b9c3e4f3cbcab67cd6779be05dff998293d855f8dd5ba818c

.. code-block:: cpp

    /// Hash a new user's password and store only its canonical password-hash record.
    ///
    /// The exact username is checked before prompting. The password remains in protected storage after the prompt's
    /// unavoidable ordinary-memory handoff. `PasswordHasher::hash()` creates a fresh salt and applies the active pepper;
    /// only the returned canonical record is written to the user database.
    auto PasswordHandlerApp::addUser(const el::OptionValuesPtr &values) -> el::ExitCode {
        requireInteractiveTerminal();
        const auto username = values->getText("username"_el);
        requireUsername(username);
        const auto paths = storagePaths(values);
        validateStorageState(paths);
        auto database = paths.database.info().exists() ? UserDatabase::load(paths.database) : UserDatabase{};
        if (database.contains(username)) {
            throw el::ApplicationError{"The exact username already exists."_el};
        }

        const auto password = PasswordPrompt::readNewPassword(terminal());
        initializeStorage(paths);
        auto hasher = PepperStore::loadHasher(paths.pepper);
        const auto passwordHash = hasher.hash(password);
        static_cast<void>(database.tryAdd(username, passwordHash.toString()));
        database.save(paths.database, el::PathCollisionMode::Overwrite);
        el::io::printLine("User added: "_el, displayUsername(username));
        return el::ExitCode::success();
    }

.. erbsland-demo-end::

Each :cpp:func:`PasswordHasher::hash() <erbsland::cryptology::PasswordHasher::hash>` call obtains a fresh 16-byte salt
from secure randomness.
The canonical record includes its format version, algorithm, costs, pepper mode, public key identifier, salt, and
protected verifier.
It contains neither the password nor the pepper.
Treat this format as opaque and use :cpp:class:`PasswordHash <erbsland::cryptology::PasswordHash>` to parse and
serialize it.

The user database contains ordered repeated ``*[Users]*`` sections.
Each section has exactly one ``Username`` and one ``Password Hash`` text field.
Ordering by exact username produces deterministic ELCL output.
Malformed structure, missing or unexpected fields, and duplicate usernames are rejected.
Malformed hash text remains intact during loading so login can still route it through dummy verification.

Verify a Login
==============

:cpp:func:`PasswordHash::fromString() <erbsland::cryptology::PasswordHash::fromString>` returns an invalid sentinel for
malformed, unsupported, or excessive records.
The demo uses the same sentinel for unknown users and always calls
:cpp:func:`PasswordHasher::verify() <erbsland::cryptology::PasswordHasher::verify>`.
Incorrect passwords, unknown usernames, and malformed hashes therefore use identical rejection text and failure status.

.. erbsland-demo::
    :source: cryptology/PasswordHandler/src/PasswordHandlerApp_login.cpp
    :source-sha256: efc12fab39c97197dd7a9e51cdd38c57c99418f24e33bdd4bcfe6c0f2247447d

.. code-block:: cpp

    /// Verify a login and persist any replacement hash before reporting success.
    ///
    /// Unknown usernames and malformed records use the same invalid `PasswordHash` sentinel and always reach
    /// `PasswordHasher::verify()`. Rejected attempts therefore share one message and status. A successful verification
    /// through a fallback pepper returns a replacement record made with the active pepper.
    auto PasswordHandlerApp::login(const el::OptionValuesPtr &values) -> el::ExitCode {
        requireInteractiveTerminal();
        const auto username = values->getText("username"_el);
        requireUsername(username);
        const auto paths = storagePaths(values);
        validateStorageState(paths);
        auto database = paths.database.info().exists() ? UserDatabase::load(paths.database) : UserDatabase{};
        const auto storedHash = database.contains(username) ? el::PasswordHash::fromString(database.passwordHash(username))
                                                            : el::PasswordHash{};
        const auto password = PasswordPrompt::readPassword(terminal());
        initializeStorage(paths);
        auto hasher = PepperStore::loadHasher(paths.pepper);
        const auto verification = hasher.verify(password, storedHash);
        if (verification.isRejected()) {
            el::io::printLine("Login rejected."_el);
            return el::ExitCode::failure();
        }
        if (verification.replacementHash().has_value()) {
            static_cast<void>(database.trySetPassword(username, verification.replacementHash()->toString()));
            database.save(paths.database, el::PathCollisionMode::Overwrite);
        }
        el::io::printLine("Login accepted."_el);
        return el::ExitCode::success();
    }

.. erbsland-demo-end::

Verification returns a :cpp:class:`PasswordVerification <erbsland::cryptology::PasswordVerification>` rather than a
boolean.
A successful login may include a replacement hash when the algorithm, costs, record format, pepper mode, or applicable
key differs from the active configuration.
The demo persists this replacement before it reports success.

Uniform verification work is only part of user-enumeration resistance.
A production system must also keep response text, status, redirects, logging, database behavior, and rate limits
uniform, then measure the complete request path under realistic load.

Rotate Peppers
==============

The pepper file contains an identified active key and may contain repeated ``*[Pepper.Fallback]*`` sections for
historical keys.
One identifier-less fallback is allowed for legacy records created before key identifiers were introduced.

.. erbsland-demo::
    :source: cryptology/PasswordHandler/src/PepperRotation.cpp
    :source-sha256: ec4168672ea20ae5c39a8ce04177b48148440b558360660b5259f8da17c0b43b

.. code-block:: cpp

    /// Configure active and fallback peppers for gradual password-record migration.
    ///
    /// The active key has a public identifier stored in new password records. Historical keys verify existing records.
    /// There may be at most one unnamed fallback for legacy hashes. A successful fallback verification returns a
    /// replacement hash that the login command persists before reporting success.
    auto buildPasswordHasher(const el::PasswordHashKey &activeKey, const el::List<el::PasswordHashKey> &fallbackKeys)
        -> el::PasswordHasher {
        return el::PasswordHasher::withKeyRotation(activeKey, fallbackKeys);
    }

.. erbsland-demo-end::

When a fallback key verifies a password, the result contains a replacement record protected by the active key.
Persisting that replacement gradually migrates active users without asking them to choose a new password.
Retain each fallback until all applicable records have migrated or the remaining users have reset their passwords.

Unkeyed Records
===============

Erbsland Core provides an explicitly unsafe unkeyed hasher for deployments that truly cannot protect a separate
application secret.
Unkeyed records store the raw password-derivation result and lose the additional protection supplied by a pepper.
The demo deliberately provides no unkeyed mode.
Use one only after documenting the threat-model tradeoff and why a secret manager or operating-system key store is
unavailable.
A keyed hasher can verify an older unkeyed record and return a keyed replacement after successful verification.

Exact Unicode Behavior
======================

Passwords and usernames preserve their exact valid UTF-8 spelling.
Passwords may contain spaces and arbitrary Unicode.
The demo does not trim, fold case, normalize Unicode, alter line endings, or apply composition rules.
Consequently, visually identical Unicode strings can represent different passwords or usernames.

If an application adopts Unicode normalization, it must apply the same form before marking the resulting ``String`` on
every creation and verification path.
Changing that policy later is an authentication-format migration requiring an explicit compatibility strategy.

Safe Local Persistence
======================

Missing directories and files are created with
:cpp:enum:`PathAccessProfile::UserOnly <erbsland::path::PathAccessProfile>`.
Existing directories are never made more permissive.
Each save uses a user-only temporary text stream in the destination directory, closes it successfully, and moves it over
the destination.
Initial creation stops on collision; user database updates replace the destination.
Released temporary files are removed after a failed move.

This same-directory replacement prevents readers from observing a partially written file, but the demo is not a
transactional multi-user store.
A production implementation still needs concurrency control, durable transactional storage, crash-recovery semantics,
and a coordinated transaction when related state spans multiple records or systems.

Operational Protection Still Matters
====================================

Password hashing protects stored verifiers, not the complete authentication workflow.
A production system must additionally:

* check new passwords against an appropriate compromised-password blocklist;
* carry credentials only over authenticated transport such as TLS;
* apply authorization after authentication;
* enforce persistent per-account and per-source rate limits across processes and restarts;
* prevent concurrent lost updates and use production-grade transactional storage;
* keep database and secret-store access narrow and monitor unusual verification volume;
* exclude passwords, peppers, raw derivation output, and request bodies from logs, traces, metrics, and crash reports.

Sensitive Erasure Boundary
==========================

A marked :cpp:type:`String <erbsland::text::String>` and marked
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` allocations are securely erased when their final owner releases them.
Copies and slices share that allocation.
These types reduce recoverable heap remnants, but they do not lock pages, prevent swapping, sanitize CPU registers,
erase caller-owned sources, protect crash dumps, or defend a compromised process.
Keep ordinary copies as few and short-lived as the surrounding input and serialization APIs permit.
