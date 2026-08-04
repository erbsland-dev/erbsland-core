
*******************************
Platform Integration Guidelines
*******************************

These guidelines describe how Erbsland Core code should integrate with Windows, macOS and Linux APIs.
Keep this page compact and extend it when new platform integration patterns become stable.

General Rules
=============

1.  Isolate native APIs behind narrow ``impl`` classes or backend interfaces.
2.  Prefer virtual backend contracts over scattered platform macros.
3.  Keep ``#if`` platform switching minimal and close to backend creation and platform-only implementation files.
4.  Keep platform dependent code in isolated compilation units (``...Windows.cpp/hpp``/``...Posix.cpp/hpp``)
5.  Use CMake ``if (WIN32)`` to only include the units for the compiled platform.
6.  Use ``ERBSLAND_CORE_OS_...`` macros to detect the platform (in ``core/Definitions.hpp``)

Strings and Buffers
===================

1.  Use ``text::String`` (or ``text::StringEditor``) for regular library code and POSIX/macOS API inputs.
2.  Use ``text::U16String`` (or ``text::U16StringEditor``) for Windows API inputs that require UTF-16.
3.  Use ``text::StringConverter`` for explicit UTF-8/UTF-16 conversion at platform boundaries.
4.  Use ``text::impl::UnsafeU8StringEditorAccess`` and ``text::impl::UnsafeU16StringEditorAccess`` only for read-only,
    null-terminated access to existing strings.
5.  Use ``text::impl::UnsafeU8StringBuffer`` and ``text::impl::UnsafeU16StringBuffer`` for APIs that fill caller-owned
    buffers.
6.  Track buffer sizes with strong unit types such as ``unit::ByteLength`` and ``unit::U16DataLength`` whenever the size
    is part of library logic.

Platform Notes
==============

1.  Wrap Windows headers and platform details through existing core helpers, such as ``core/impl/WindowsApi.hpp``.
2.  Use RAII wrappers for handles, allocated native buffers and other resources.
3.  Avoid leaking platform naming, separators, encodings or handle types into public cross-platform APIs.

Native Event Reactors
=====================

1.  ``EventLoopDriver`` is the only public native wait and wake abstraction.
2.  Default drivers use ``epoll`` with ``eventfd`` on Linux, ``kqueue`` with ``EVFILT_USER`` on macOS, and IOCP with
    posted wake completions on Windows.
3.  Descriptor and ``HANDLE`` registration APIs remain in platform-specific ``event::impl`` interfaces.
4.  Native registrations use RAII and a unique generation token. Unregistration removes the token before stale
    readiness or completion records can be delivered.
5.  Backends receive the loop driver when attached and must use that single driver-controlled wait path.
6.  Keep socket and resolver policy in domain backends; native drivers only multiplex readiness, completion and wake
    notifications.

Protected Data
==============

1.  Keep native protected-data handles and envelope formats inside provider implementations.
2.  Resolve the provider through application data; portable protected blocks store only opaque ciphertext and length.
3.  Self-test a provider before lock-in. Automatic fallback is allowed only while selecting the initial provider.
4.  macOS uses a Secure Enclave P-256 key with Security framework ECIES/AES-GCM.
5.  Windows uses DPAPI-NG with an application-held logon-local protection descriptor.
6.  The Windows provider loads ``ncrypt.dll`` from the system directory only when selected and resolves its required
    DPAPI-NG entry points explicitly. Do not add a static Ncrypt import dependency.
7.  Linux uses the internal userspace AES-256-GCM provider. Do not probe or use AF_ALG, kernel keyrings, TPM services,
    or desktop key stores for this feature.
8.  Release or delete native key references and securely erase application-owned keys during application destruction.

Tests
=====

1.  Make sure, backend logic has an abstract interface, that allows to write mock-backends and backend-proxies.
2.  Add platform-specific suites for behavior that depends on real OS APIs.
3.  Let CMake select Windows or POSIX test files instead of guarding whole tests with large macro blocks.
