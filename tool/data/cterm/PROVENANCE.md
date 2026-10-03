# Terminal frame character repertoire

`box_frame_characters.txt` records the ordered character selection from the
retired Python frame generators. Each line is a Unicode scalar; line order
decides ties in the selection algorithm. The verified UCD `UnicodeData.txt`
supplies every character name and stroke interpretation. The sibling manifest
records the SHA3-256 hash of the repertoire text with CRLF and lone CR normalized to LF.

To change the repertoire, edit the file, review the selection and generated
tables, then update its manifest hash. Regenerate both terminal frame outputs.

All other bytes, including a BOM and the final line terminator, remain significant in text checksums.
