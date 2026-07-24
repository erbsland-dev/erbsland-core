.. index::
    single: Application

***********
Application
***********

:cpp:class:`Application <erbsland::core::Application>` retains non-owning access to the original narrow or wide
``argv`` vector supplied to its constructor.
After option parsing, every suffix reported as sensitive text is overwritten in place with one star per existing byte or
code unit.
The terminating null and native buffer size are preserved.
Its converted :cpp:type:`CommandLineArguments <erbsland::core::CommandLineArguments>` list uses exactly five stars for
each sensitive value.
Masking occurs for successful parsing and parser or validator errors.

The native pointers must remain valid for the application lifetime.
This cleanup only reduces secrets retained in process memory; it cannot retract command-line values already exposed
through process listings, the shell, operating-system facilities, logs, or earlier application code.

Interface
=========

.. doxygenclass:: erbsland::core::Application
    :members:

.. doxygenfunction:: erbsland::core::application() -> Application &
.. doxygenclass:: erbsland::core::ApplicationError
    :members:
.. doxygenclass:: erbsland::core::ApplicationInfo
    :members:
.. doxygentypedef:: erbsland::core::CommandLineArguments
.. doxygentypedef:: erbsland::core::InitializeFn
.. doxygentypedef:: erbsland::core::MainFn
.. doxygenclass:: erbsland::unit::ExitCode
    :members:
