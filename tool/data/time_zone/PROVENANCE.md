# Time-zone source snapshot

The nine region files and `version` under `tzdb/` are from the supplied IANA
tzdb 2026b snapshot. Its `README`, `NEWS`, `LICENSE`, and tabulation files are
retained for provenance. The CLDR `windowsZones.xml` snapshot is from release 48:

https://raw.githubusercontent.com/unicode-org/cldr/release-48/common/supplemental/windowsZones.xml

`manifest.elcl` pins both releases and records SHA3-256 hashes of every file
used by `erbsland-core-tool generate-time-zone-data`. The generator uses these
local files as the sole authority for time-zone histories, aliases, and Windows
mapping facts. Calendar and XML handling use Erbsland Core without consulting
its generated time-zone database.

The CLDR file retains its Unicode Consortium copyright and terms-of-use notice.

Text checksums represent CRLF and lone CR as LF. All other bytes, including a BOM
and the final line terminator, remain significant.
