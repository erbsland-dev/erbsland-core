# Unicode 17.0.0 source snapshot

All Unicode-sensitive generation decisions use these checked-in official sources. The
manifest records the expected version and uppercase SHA3-256 hashes of text with CRLF and lone CR normalized to LF.
The shared Unicode source verifies all thirteen production inputs before deriving data; it does not
expand placeholders in manifests. Normal generation is offline.

The UCD inputs came from https://www.unicode.org/Public/17.0.0/ucd/ (the repository's
`tmp/UCD` import). `DerivedJoiningType.txt` is from the `extracted` subdirectory.
Shared preparation also uses `CompositionExclusions.txt`, `IdnaMappingTable.txt`,
`CaseFolding.txt`, `EastAsianWidth.txt`, and `ReadMe.txt`. Together with the eight
property inputs, these are the thirteen production inputs. The last three were
used by the data generators; they share this verified snapshot.
Verification alone loads `NormalizationTest.txt`, `Idna2008.txt`, and `IdnaTestV2.txt`;
these additional files are not required during ordinary source initialization.

`IdnaMappingTable.txt`, `Idna2008.txt`, and `IdnaTestV2.txt` were retrieved on 2026-10-09
from https://www.unicode.org/Public/17.0.0/idna/. The mapping table is a generation input;
the category and test tables are verification inputs. `Idna2008.txt` supplies an official
exhaustive status reference. `IdnaTestV2.txt` tests nontransitional cases that also satisfy
the strict IDNA2008 profile, with its expected error cases covered by focused tests.

`ReadMe.txt` records upstream UCD version information. `LICENSE.txt` contains the Unicode
license; the additional sources carry Unicode copyright and terms-of-use headers.

Upgrade by importing a consistent release, replacing the manifest hashes and configured
version in `tool/conf/source/unicode.elcl`, then reviewing conformance and regenerated output.
See `doc/development/tool_usage/update_unicode.rst` for the manual update workflow.
Synthetic test versions remain fixed; frozen migration references are no longer needed. No compiled Core
Unicode table or installed Python IDNA package controls this snapshot.

All other bytes, including a BOM and the final line terminator, remain significant in text checksums.
