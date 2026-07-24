# Porting Notes
This document lists porting issues found in the code, that need to be addressed before the final release.

## Domain `cterm`
- The class `Input` and the whole backend uses `std::chrono` types to specify timeouts.
  Correct would be a time amount like `time::Milliseconds` or `time::TimeDelta`.
  I added an overload for `time::Milliseconds` at the public API boundary, but this is not ideal.
