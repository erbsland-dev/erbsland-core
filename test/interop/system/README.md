# System Interoperability Tests

This directory contains standalone system interoperability tests that exercise operating-system process behavior.
It uses Erbsland Unit Test for reporting and selection, but is deliberately not registered with CTest because the
executable starts and controls subprocesses.

Enable it explicitly and run it manually:

```shell
cmake -S . -B cmake-build-interop -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DERBSLAND_CORE_ENABLE_INTEROP_TESTS=ON
cmake --build cmake-build-interop --target erbsland-core-system-interop
cmake-build-interop/test/interop/system/erbsland-core-system-interop
```

The executable also serves as its own controlled child counterpart. Private command-line helper modes provide known
exit codes, output, delays, environment inspection, and marker-file behavior without invoking a shell or depending on
external programs.
