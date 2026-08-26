
*******************************
Platform Integration Guidelines
*******************************

These guideline describes how Erbsland Core code should use native Windows, macOS and Linux APIs.

General Rules
=============

* Isolate native APIs behind narrow ``impl`` classes or backend interfaces.
* Prefer virtual backend contracts over platform macro logic inside implementation files.
* Keep ``#if`` platform switching minimal and close to backend creation and platform-only implementation files.
* Keep platform dependent code in isolated compilation units (``Windows....cpp/hpp`` / ``Posix....cpp/hpp``)
* Use CMake ``if (WIN32)`` to only include the units for the compiled platform.
* Use ``ERBSLAND_CORE_OS_...`` macros to detect the platform (in ``core/Definitions.hpp``)

Recommended Pattern
-------------------

.. code-block: cpp

    // File impl/Example.hpp
    class Example {
    public:
        [[nodiscard]] static auto create() -> ExamplePtr;
        virtual ~Example() = default;
    public:
        // API
    };

    // File impl/Example.cpp
    auto Example::create() -> ExamplePtr {
    #ifdef ERBSLAND_CORE_OS_WINDOWS
        return std::make_shared<WindowsExample>();
    #else
        return std::make_shared<PosixExample>();
    #endif
    };

    // File impl/WindowsExample.hpp
    class WindowsExample : public Example {
        // ...
    };

    // File impl/WindowsExample.cpp
    // ...

    // File impl/PosixExample.hpp
    class PosixExample : public Example {
        // ...
    };

    // File impl/PosixExample.cpp
    // ...

Strings and Buffers
===================

* Use ``text::String`` for regular library code and POSIX/macOS API inputs.
* Use ``text::U16String`` for Windows API inputs that require UTF-16.
* Use ``text::StringConverter`` for explicit UTF-8/UTF-16 conversion at platform boundaries.
* Use ``text::impl::PlatformU8StringAccess`` and ``text::impl::PlatformU16StringAccess`` for null-terminated
  read-only platform API inputs. These accessors safely materialize sliced strings.
* Use ``text::impl::UnsafeU8StringAccess`` and ``text::impl::UnsafeU16StringAccess`` only for bounded span or data-view
  access inside the library. They deliberately do not expose null-terminated pointers.
* Use ``text::impl::UnsafeU8StringBuffer`` and ``text::impl::UnsafeU16StringBuffer`` for APIs that fill caller-owned
  buffers.
* Track buffer sizes with strong unit types such as ``unit::ByteLength`` and ``unit::U16DataLength`` whenever the size
  is part of library logic.

Platform Notes
==============

* Wrap Windows headers and platform details through existing core helpers, such as ``core/impl/WindowsApi.hpp``.
* Use RAII wrappers for handles, allocated native buffers and other resources.
* Avoid leaking platform naming, separators, encodings or handle types into public cross-platform APIs.

Tests
=====

* Make sure, backend logic has an abstract interface, that allows to write mock-backends and backend-proxies.
* Add platform-specific suites for behavior that depends on real OS APIs.
* Let CMake select Windows or POSIX test files instead of guarding whole tests with large macro blocks.
