.. index::
    single: Subprocesses

************
Subprocesses
************

Introduction
============

``Subprocess`` starts and owns one operating-system child process without invoking a command shell.
The executable and each argument are passed as separate values, so shell quoting, expansion, redirection, and pipelines
are never applied.

An owned child is reaped when it exits.
Destroying a ``Subprocess`` that is still running first requests termination, waits briefly, then forcefully terminates
and reaps the child.
This makes the type suitable for helpers whose lifetime must not outlive an application scope or test.
``startDetached()`` is the explicit launch-and-forget operation; an internal waiter still prevents a zombie process on
POSIX systems.

Starting a Process
==================

``start()`` requires a direct executable path and an optional ``StringList`` of arguments.
It does not search the ``PATH`` environment variable.
``SubprocessOptions`` can select a working directory, environment inheritance and changes, standard-input inheritance,
and output handling.

Environment changes are applied after optional inheritance.
A value assigns the variable and ``std::nullopt`` removes it.
Variable-name case sensitivity follows the operating system.

Waiting and Termination
=======================

``wait()`` blocks until the child exits.
The timed overload returns ``std::nullopt`` when its non-negative deadline expires.
``isRunning()`` performs a non-blocking status update.
Once observed, the ``SubprocessExitStatus`` remains available through ``exitStatus()``.

On POSIX, ``terminate()`` sends ``SIGTERM`` and ``kill()`` sends ``SIGKILL``.
Windows has no equivalent graceful process signal, so both operations use native forced process termination.
Normal Windows termination is reported as an exit code; POSIX signal termination is reported separately.

Standard Streams
================

Standard output and standard error can independently be inherited, discarded, or captured.
Standard error can instead be merged into the selected standard-output destination.
Standard input can only be inherited or connected to an immediately closed null stream; interactive input piping is
intentionally outside this API.

Capture retains a bounded prefix while background readers continue draining the native pipes.
This prevents a verbose child from blocking after the capture limit is reached.
The default limit is one MiB per stream and the maximum configurable limit is 64 MiB.
Use the truncation accessors to detect discarded suffixes.
Detached processes cannot use capture because no owner remains to consume the result.

Interface
=========

.. doxygenclass:: erbsland::system::Subprocess
    :members:
.. doxygenclass:: erbsland::system::SubprocessExitStatus
    :members:
.. doxygenclass:: erbsland::system::SubprocessOptions
    :members:
.. doxygenenum:: erbsland::system::SubprocessOutputMode
