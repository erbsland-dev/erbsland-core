# Public Suffix List snapshot

- Source: `https://publicsuffix.org/list/public_suffix_list.dat`
- Version: `2026-08-19_19-18-48_UTC`
- Commit: `e8c9a2b2b2856b6449999dd0ec0d118f364ed0cd`
- Retrieved: `2026-08-24`
- License: Mozilla Public License 2.0, as stated in the source file header.

The complete ICANN and PRIVATE sections are retained verbatim in `public_suffix_list.dat`.
Run `utilities/run.py generate_public_suffix_data` to regenerate the compact C++ lookup table, or add `--check` to
verify freshness without changing files.
