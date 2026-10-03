# Public Suffix List snapshot

- Source: `https://publicsuffix.org/list/public_suffix_list.dat`
- Version: `2026-08-19_19-18-48_UTC`
- Commit: `e8c9a2b2b2856b6449999dd0ec0d118f364ed0cd`
- Retrieved: `2026-08-24`
- License: Mozilla Public License 2.0, as stated in the source file header.

The complete ICANN and PRIVATE sections are retained verbatim in `public_suffix_list.dat`.
The sibling `manifest.elcl` pins the normalized text with SHA3-256 and the snapshot version.
Normal generation is offline.

Run `./erbsland-core-tool generate-public-suffix-data` to regenerate the compact C++ lookup table, or add `--check` to
verify freshness without changing files.

The shared source configuration is `tool/conf/source/public_suffix.elcl`.
See `doc/development/tool_usage/update_public_suffix.rst` for deliberate manual snapshot updates.
Source initialization verifies the existing snapshot and never downloads or replaces it.

All other bytes, including a BOM and the final line terminator, remain significant in text checksums.
