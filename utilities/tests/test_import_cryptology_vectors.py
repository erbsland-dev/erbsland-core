# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from test.import_cryptology_vectors import ImportCryptologyVectorsApp, SHA3_FILES, SHA_FILES, SOURCES


class ImportCryptologyVectorsTest(unittest.TestCase):
    """Tests for deterministic cryptology fixture filtering."""

    def test_supported_sha_file_lists_are_complete(self) -> None:
        self.assertEqual(len(SOURCES), 22)
        self.assertEqual(len(SHA_FILES), 12)
        self.assertEqual(len(SHA3_FILES), 9)
        self.assertEqual(sum(name.endswith("LongMsg.rsp") for name in SHA_FILES + SHA3_FILES), 7)
        self.assertEqual(sum(name.endswith("ShortMsg.rsp") for name in SHA_FILES + SHA3_FILES), 7)
        self.assertEqual(sum(name.endswith("Monte.rsp") for name in SHA_FILES + SHA3_FILES), 7)

    def test_write_normalizes_text_fixtures(self) -> None:
        with tempfile.TemporaryDirectory(dir="/private/tmp") as temporary_directory:
            app = ImportCryptologyVectorsApp()
            app.output_directory = Path(temporary_directory)
            app.write("sample.rsp", b"value = 1 \r\nnext = 2\t\r\n")
            self.assertEqual((app.output_directory / "sample.rsp").read_bytes(), b"value = 1\nnext = 2\n")

    def test_x509_limbo_pem_conversion_is_exact(self) -> None:
        pem = "-----BEGIN CERTIFICATE-----\nMAA=\n-----END CERTIFICATE-----\n"
        self.assertEqual(ImportCryptologyVectorsApp.certificate_pem_to_der_hex(pem), "3000")

    def test_gcm_filter_retains_exact_supported_matrix(self) -> None:
        lines = ["# GCM fixture", "[Keylen = 128]"]
        for plaintext_length in range(5):
            for aad_length in range(5):
                lines.extend(
                    [
                        "[IVlen = 96]",
                        f"[PTlen = {plaintext_length * 8}]",
                        f"[AADlen = {aad_length * 8}]",
                        "[Taglen = 128]",
                    ]
                )
                for count in range(15):
                    lines.extend([f"Count = {count}", "Key = 00", ""])
        lines.extend(["[IVlen = 64]", "[PTlen = 0]", "[AADlen = 0]", "[Taglen = 128]", "Count = 0"])

        filtered, groups, records = ImportCryptologyVectorsApp.filter_gcm("\r\n".join(lines).encode("ascii"))

        self.assertEqual(groups, 25)
        self.assertEqual(records, 375)
        self.assertEqual(filtered.count(b"Count = "), 375)
        self.assertNotIn(b"[IVlen = 64]", filtered)
        self.assertNotIn(b"\r", filtered)

    def test_hmac_filter_retains_complete_sha2_sections(self) -> None:
        lines = ["# HMAC fixture", "[L=20]", "Count = 0", "", "[L=32]"]
        for count in range(225):
            lines.extend([f"Count = {count}", "Key = 00", ""])
        lines.append("[L=48]")
        for count in range(300):
            lines.extend([f"Count = {count}", "Key = 00", ""])
        lines.extend(["[L=64]", "Count = 0"])

        source = "\r\n".join(lines).encode("ascii")
        filtered256, records256 = ImportCryptologyVectorsApp.filter_hmac(source, 32)
        filtered384, records384 = ImportCryptologyVectorsApp.filter_hmac(source, 48)

        self.assertEqual(records256, 225)
        self.assertEqual(records384, 300)
        self.assertEqual(filtered256.count(b"Count = "), 225)
        self.assertEqual(filtered384.count(b"Count = "), 300)
        self.assertIn(b"[L=32]", filtered256)
        self.assertNotIn(b"[L=48]", filtered256)
        self.assertIn(b"[L=48]", filtered384)
        self.assertNotIn(b"[L=32]", filtered384)

    def test_blake2b_filter_retains_all_unkeyed_lengths(self) -> None:
        source = []
        for length in range(256):
            source.append({"hash": "blake2b", "key": "", "in": "00" * length, "out": "11" * 64})
        source.append({"hash": "blake2b", "key": "22", "in": "", "out": "33" * 64})

        filtered, records = ImportCryptologyVectorsApp.filter_blake2b(json.dumps(source).encode("ascii"))

        self.assertEqual(records, 256)
        self.assertEqual(filtered.count(b"Count = "), 256)
        self.assertIn(b"Len = 0\n", filtered)
        self.assertIn(b"Len = 2040\n", filtered)

    def test_wycheproof_chacha20_poly1305_filter_retains_all_cases(self) -> None:
        groups = []
        test_id = 1
        invalid_iv_sizes = [0, 64, 88, 104, 112, 128, 160, 192, 256]
        for iv_size, valid_count, invalid_count in [
            (96, 256, 60),
            *[(size, 0, 1) for size in invalid_iv_sizes],
        ]:
            tests = []
            for result, count in [("valid", valid_count), ("invalid", invalid_count)]:
                for _ in range(count):
                    tests.append(
                        {
                            "tcId": test_id,
                            "flags": ["Synthetic"],
                            "key": "00" * 32,
                            "iv": "00" * (iv_size // 8),
                            "aad": "",
                            "msg": "",
                            "ct": "",
                            "tag": "00" * 16,
                            "result": result,
                        }
                    )
                    test_id += 1
            groups.append(
                {
                    "ivSize": iv_size,
                    "keySize": 256,
                    "tagSize": 128,
                    "type": "AeadTest",
                    "source": {"name": "google-wycheproof", "version": "test-version"},
                    "tests": tests,
                }
            )
        source = {"algorithm": "CHACHA20-POLY1305", "numberOfTests": 325, "testGroups": groups}

        filtered, group_count, valid, authentication_failures, invalid_nonces = (
            ImportCryptologyVectorsApp.filter_wycheproof_chacha20_poly1305(json.dumps(source).encode("ascii"))
        )

        self.assertEqual((group_count, valid, authentication_failures, invalid_nonces), (10, 256, 60, 9))
        self.assertEqual(filtered.count(b"Count = "), 325)
        self.assertEqual(filtered.count(b"[IVlen = "), 10)
        self.assertIn(b"# Upstream vector source: google-wycheproof test-version", filtered)
        self.assertIn(b"# SPDX-License-Identifier: Apache-2.0", filtered)

    def test_wycheproof_header_preserves_generator_attribution_fallback(self) -> None:
        header = ImportCryptologyVectorsApp.wycheproof_header(
            {"generatorVersion": "0.9", "testGroups": [{}]}, "RSA-PSS", "rsa_pss_test.json"
        )

        self.assertIn("# Upstream Wycheproof generator version: 0.9", header)
        self.assertIn("# Modified by Erbsland Core: converted from JSON into deterministic response-file form", header)
        self.assertIn("# License: Apache-2.0; see the repository LICENSE file", header)

    def test_manifest_group_counts_are_semantic(self) -> None:
        self.assertEqual(ImportCryptologyVectorsApp.group_count(Path("aes/ECB.rsp"), "[ENCRYPT]\n[DECRYPT]\n"), 2)
        self.assertEqual(ImportCryptologyVectorsApp.group_count(Path("gcm/GCM.rsp"), "[IVlen = 96]\n[PTlen = 0]\n"), 1)
        self.assertEqual(
            ImportCryptologyVectorsApp.group_count(
                Path("chacha20_poly1305/wycheproof.rsp"), "[IVlen = 96]\n[Keylen = 256]\n"
            ),
            1,
        )
        self.assertEqual(ImportCryptologyVectorsApp.group_count(Path("hash/BLAKE2b.rsp"), "[Hash = BLAKE2b]\n"), 1)
        self.assertEqual(ImportCryptologyVectorsApp.group_count(Path("sha/SHA256.rsp"), "[L = 32]\n"), 1)

    def test_rsa_algorithm_identifiers_are_exact(self) -> None:
        self.assertEqual(
            ImportCryptologyVectorsApp.rsa_algorithm_der("PKCS1", "SHA-256", 0),
            "300d06092a864886f70d01010b0500",
        )
        self.assertEqual(
            ImportCryptologyVectorsApp.rsa_algorithm_der("PSS", "SHA-384", 48),
            "304106092a864886f70d01010a3034"
            "a00f300d06096086480165030402020500"
            "a11c301a06092a864886f70d010108300d06096086480165030402020500"
            "a203020130",
        )
        self.assertTrue(ImportCryptologyVectorsApp.rsa_algorithm_der("PSS", "SHA-256", 0).endswith("a203020100"))

    def test_ecdsa_der_encodings_are_exact(self) -> None:
        self.assertEqual(
            ImportCryptologyVectorsApp.ecdsa_algorithm_der("SHA-256"),
            "300a06082a8648ce3d040302",
        )
        self.assertEqual(
            ImportCryptologyVectorsApp.ecdsa_public_key_der("P-256", "11" * 32, "22" * 32),
            "3059301306072a8648ce3d020106082a8648ce3d03010703420004" + "11" * 32 + "22" * 32,
        )
        self.assertEqual(ImportCryptologyVectorsApp.der_integer("80"), "02020080")
        self.assertEqual(ImportCryptologyVectorsApp.der_integer("0001"), "020101")
        self.assertEqual(ImportCryptologyVectorsApp.ecdsa_signature_der("01", "80"), "300702010102020080")

    def test_wycheproof_ed25519_filter_preserves_all_fields(self) -> None:
        public_key = "11" * 32
        public_key_der = "302a300506032b6570032100" + public_key
        source = {
            "algorithm": "EDDSA",
            "numberOfTests": 2,
            "testGroups": [
                {
                    "type": "EddsaVerify",
                    "publicKey": {
                        "type": "EDDSAPublicKey",
                        "curve": "edwards25519",
                        "keySize": 255,
                        "pk": public_key,
                    },
                    "publicKeyDer": public_key_der,
                    "source": {"name": "google-wycheproof", "version": "test-version"},
                    "tests": [
                        {"tcId": 1, "result": "valid", "flags": ["Valid"], "msg": "", "sig": "22" * 64},
                        {
                            "tcId": 2,
                            "result": "invalid",
                            "flags": ["InvalidSignature"],
                            "msg": "01",
                            "sig": "33" * 64,
                        },
                    ],
                }
            ],
        }

        filtered, valid, invalid = ImportCryptologyVectorsApp.filter_wycheproof_ed25519(
            json.dumps(source).encode("ascii"),
            expected_count=2,
            expected_groups=1,
            expected_valid=1,
            expected_invalid=1,
        )

        self.assertEqual((valid, invalid), (1, 1))
        self.assertEqual(filtered.count(b"Count = "), 2)
        self.assertIn(f"[PublicKeyDer = {public_key_der}]".encode("ascii"), filtered)
        self.assertIn(b"[AlgorithmDer = 300506032b6570]", filtered)
        self.assertIn(b"Result = invalid\nFlags = InvalidSignature", filtered)

    def test_wycheproof_ecdsa_filter_preserves_supported_cases(self) -> None:
        source = {
            "algorithm": "ECDSA",
            "numberOfTests": 2,
            "testGroups": [
                {
                    "type": "EcdsaVerify",
                    "sha": "SHA-256",
                    "publicKeyDer": "3000",
                    "publicKey": {"curve": "secp256r1", "keySize": 256},
                    "source": {"name": "google-wycheproof", "version": "test-version"},
                    "tests": [
                        {"tcId": 1, "result": "valid", "flags": [], "msg": "", "sig": "3000"},
                        {"tcId": 2, "result": "invalid", "flags": ["Synthetic"], "msg": "01", "sig": "3001"},
                    ],
                }
            ],
        }

        filtered = ImportCryptologyVectorsApp.filter_wycheproof_ecdsa(
            json.dumps(source).encode("ascii"),
            "ecdsa_test.json",
            "secp256r1",
            "SHA-256",
            2,
            1,
        )

        self.assertEqual(filtered.count(b"Count = "), 2)
        self.assertIn(b"[Curve = secp256r1]", filtered)
        self.assertIn(b"[AlgorithmDer = 300a06082a8648ce3d040302]", filtered)
        self.assertIn(b"Result = invalid\nFlags = Synthetic", filtered)

    def test_nist_ecdsa_filter_selects_matched_pairs(self) -> None:
        sections = []
        for curve, sha, width in [("P-256", "SHA-256", 64), ("P-384", "SHA-384", 96)]:
            sections.append(f"[{curve},{sha}]")
            for count in range(15):
                sections.extend(
                    [
                        f"Msg = {count:02x}",
                        f"Qx = {'1' * width}",
                        f"Qy = {'2' * width}",
                        "R = 01",
                        "S = 80",
                        "Result = P (0 )",
                        "",
                    ]
                )
        sections.extend(["[P-256,SHA-384]", "Msg = 00", "Qx = 00", ""])

        filtered, count = ImportCryptologyVectorsApp.filter_nist_ecdsa("\n".join(sections).encode("ascii"))

        self.assertEqual(count, 30)
        self.assertEqual(filtered.count(b"Count = "), 30)
        self.assertEqual(filtered.count(b"[Curve = P-256]"), 15)
        self.assertEqual(filtered.count(b"[Curve = P-384]"), 15)

    def test_wycheproof_rsa_filter_preserves_supported_cases(self) -> None:
        source = {
            "algorithm": "RSASSA-PSS",
            "numberOfTests": 2,
            "testGroups": [
                {
                    "type": "RsassaPssVerify",
                    "keySize": 2048,
                    "sha": "SHA-256",
                    "mgf": "MGF1",
                    "mgfSha": "SHA-256",
                    "sLen": 32,
                    "publicKeyDer": "3000",
                    "source": {"name": "google-wycheproof", "version": "test-version"},
                    "tests": [
                        {"tcId": 1, "result": "valid", "flags": [], "msg": "", "sig": "00"},
                        {"tcId": 2, "result": "invalid", "flags": ["Synthetic"], "msg": "01", "sig": "02"},
                    ],
                }
            ],
        }

        filtered = ImportCryptologyVectorsApp.filter_wycheproof_rsa(
            json.dumps(source).encode("ascii"),
            "rsa_pss_test.json",
            "RSASSA-PSS",
            "SHA-256",
            2,
        )

        self.assertEqual(filtered.count(b"Count = "), 2)
        self.assertIn(b"[PublicKeyDer = 3000]", filtered)
        self.assertIn(b"Result = invalid\nFlags = Synthetic", filtered)
        self.assertIn(b"[AlgorithmDer = 3041", filtered)
        self.assertIn(b"# Upstream file: rsa_pss_test.json", filtered)
        self.assertIn(b"# Upstream vector source: google-wycheproof test-version", filtered)


if __name__ == "__main__":
    unittest.main()
