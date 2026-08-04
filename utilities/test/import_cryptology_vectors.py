#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import re
import zipfile
from dataclasses import dataclass
from pathlib import Path

from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import require_directory
from lib.utility import UtilityApp


@dataclass(frozen=True)
class SourceFile:
    name: str
    sha256: str
    url: str


SOURCES = {
    "KAT_AES.zip": SourceFile(
        "KAT_AES.zip",
        "a203b16c9246b2ebae31dee5de21a606be80cf78ceabaca37150236fa098eb60",
        "https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/"
        "documents/aes/KAT_AES.zip",
    ),
    "aesmct.zip": SourceFile(
        "aesmct.zip",
        "6a2a72c00b1daacb9a7d20ab92617d322fa1c5dee493968660990e3f4571426b",
        "https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/"
        "documents/aes/aesmct.zip",
    ),
    "aesmmt.zip": SourceFile(
        "aesmmt.zip",
        "12d1616f7a713e807714055973f04efc402f46a14e3d81869717c7ace4ecbaf0",
        "https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/"
        "documents/aes/aesmmt.zip",
    ),
    "gcmtestvectors.zip": SourceFile(
        "gcmtestvectors.zip",
        "f9fc479e134cde2980b3bb7cddbcb567b2cd96fd753835243ed067699f26a023",
        "https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/"
        "documents/mac/gcmtestvectors.zip",
    ),
    "hmactestvectors.zip": SourceFile(
        "hmactestvectors.zip",
        "418c3837d38f249d6668146bd0090db24dd3c02d2e6797e3de33860a387ae4bd",
        "https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/"
        "documents/mac/hmactestvectors.zip",
    ),
    "blake2-kat.json": SourceFile(
        "blake2-kat.json",
        "5031ac14800798ae15cee79c04d65e326a575f2c968c7e2846a79bd07a1c0e61",
        "https://raw.githubusercontent.com/BLAKE2/BLAKE2/master/testvectors/blake2-kat.json",
    ),
    "shabytetestvectors.zip": SourceFile(
        "shabytetestvectors.zip",
        "929ef80b7b3418aca026643f6f248815913b60e01741a44bba9e118067f4c9b8",
        "https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/"
        "documents/shs/shabytetestvectors.zip",
    ),
    "sha-3bytetestvectors.zip": SourceFile(
        "sha-3bytetestvectors.zip",
        "cd07701af2e47f5cc889d642528b4bf11f8b6eb55797c7307a96828ed8d8fc8c",
        "https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/"
        "documents/sha3/sha-3bytetestvectors.zip",
    ),
    "chacha20_poly1305_test.json": SourceFile(
        "chacha20_poly1305_test.json",
        "fe61d25f90e1bde4461d00eafe61049e5f29bd999f36b766df9cda90906ad53d",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/chacha20_poly1305_test.json",
    ),
    "x25519_test.json": SourceFile(
        "x25519_test.json",
        "35c3f5231cf25cc640b524d403461deee9e49441d5d915a3a25b2c8ff5adbe7d",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/x25519_test.json",
    ),
    "ed25519_test.json": SourceFile(
        "ed25519_test.json",
        "752d2ea7d7c6cf4736381b6cbacb61f8182b126ab7cd9b058f00c50084975536",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/ed25519_test.json",
    ),
    "ecdsa_secp256r1_sha256_test.json": SourceFile(
        "ecdsa_secp256r1_sha256_test.json",
        "182db4f3e230f6f9fa9f800d2a614dede30284b8e8438bbfe1171905402e9332",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/ecdsa_secp256r1_sha256_test.json",
    ),
    "ecdsa_secp384r1_sha384_test.json": SourceFile(
        "ecdsa_secp384r1_sha384_test.json",
        "8a5b3ae1760975143414811f13588c24d951d9d8c904195087ba327591dfe9cc",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/ecdsa_secp384r1_sha384_test.json",
    ),
    "186-4ecdsatestvectors.zip": SourceFile(
        "186-4ecdsatestvectors.zip",
        "fe47cc92b4cee418236125c9ffbcd9bb01c8c34e74a4ba195d954bcb72824752",
        "https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/"
        "documents/dss/186-4ecdsatestvectors.zip",
    ),
    "rsa_pss_2048_sha256_mgf1_32_test.json": SourceFile(
        "rsa_pss_2048_sha256_mgf1_32_test.json",
        "7f6efafc160f4816b96cbf1c12188a31051d7e3f001e27505d9edb5f2a0e325c",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/rsa_pss_2048_sha256_mgf1_32_test.json",
    ),
    "rsa_pss_2048_sha256_mgf1_0_test.json": SourceFile(
        "rsa_pss_2048_sha256_mgf1_0_test.json",
        "b22a8d9a2e7e47f681d0ff1d1a55455daaf325d989c64ad005f848bd50b9b4c0",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/rsa_pss_2048_sha256_mgf1_0_test.json",
    ),
    "rsa_pss_2048_sha256_mgf1_0_params_test.json": SourceFile(
        "rsa_pss_2048_sha256_mgf1_0_params_test.json",
        "a240b2267e9e411fdba4d7fe688b0c57c7dc6971818b370519a9142beec05743",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/rsa_pss_2048_sha256_mgf1_0_params_test.json",
    ),
    "rsa_pss_2048_sha256_mgf1_32_params_test.json": SourceFile(
        "rsa_pss_2048_sha256_mgf1_32_params_test.json",
        "8376c34232d5fe0e91ac1363a481c438540bc3982b0492628fa2d825d2c3c5a3",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/rsa_pss_2048_sha256_mgf1_32_params_test.json",
    ),
    "rsa_pss_2048_sha384_mgf1_48_test.json": SourceFile(
        "rsa_pss_2048_sha384_mgf1_48_test.json",
        "66d464778b0b2f683a1d1a20e94f77e9472b8cc393b45f423fa08a48e677063c",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/rsa_pss_2048_sha384_mgf1_48_test.json",
    ),
    "rsa_signature_2048_sha256_test.json": SourceFile(
        "rsa_signature_2048_sha256_test.json",
        "94a917b01ff50fb874cfc05bf29b4af44868d944a6558201cf18380da93fb393",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/rsa_signature_2048_sha256_test.json",
    ),
    "rsa_signature_2048_sha384_test.json": SourceFile(
        "rsa_signature_2048_sha384_test.json",
        "c571c105d261c0ff588a2888a529f152563fb3b77894b7620d4e4f8f934f2c1d",
        "https://raw.githubusercontent.com/C2SP/wycheproof/"
        "5722833ca004983abd1a91bcb6c24596d50ac0f9/testvectors_v1/rsa_signature_2048_sha384_test.json",
    ),
    "limbo.json": SourceFile(
        "limbo.json",
        "b25eab8e2eb3aa4e256a2bafbd01ae011627ac085ea7b7c72e0f51bce44c8f5e",
        "https://raw.githubusercontent.com/C2SP/x509-limbo/" "c6040f178a947b3fa4d4a5c118d5594f0e0ca6e2/limbo.json",
    ),
}

AES_FILES = {
    "KAT_AES.zip": [
        "CBCGFSbox256.rsp",
        "CBCKeySbox256.rsp",
        "CBCVarKey256.rsp",
        "CBCVarTxt256.rsp",
        "ECBGFSbox128.rsp",
        "ECBGFSbox256.rsp",
        "ECBKeySbox128.rsp",
        "ECBKeySbox256.rsp",
        "ECBVarKey128.rsp",
        "ECBVarKey256.rsp",
        "ECBVarTxt128.rsp",
        "ECBVarTxt256.rsp",
    ],
    "aesmct.zip": ["CBCMCT256.rsp", "ECBMCT128.rsp", "ECBMCT256.rsp"],
    "aesmmt.zip": ["CBCMMT256.rsp", "ECBMMT128.rsp", "ECBMMT256.rsp"],
}

SHA_FILES = [
    "SHA1LongMsg.rsp",
    "SHA1Monte.rsp",
    "SHA1ShortMsg.rsp",
    "SHA256LongMsg.rsp",
    "SHA256Monte.rsp",
    "SHA256ShortMsg.rsp",
    "SHA384LongMsg.rsp",
    "SHA384Monte.rsp",
    "SHA384ShortMsg.rsp",
    "SHA512LongMsg.rsp",
    "SHA512Monte.rsp",
    "SHA512ShortMsg.rsp",
]

SHA3_FILES = [
    "SHA3_256LongMsg.rsp",
    "SHA3_256Monte.rsp",
    "SHA3_256ShortMsg.rsp",
    "SHA3_384LongMsg.rsp",
    "SHA3_384Monte.rsp",
    "SHA3_384ShortMsg.rsp",
    "SHA3_512LongMsg.rsp",
    "SHA3_512Monte.rsp",
    "SHA3_512ShortMsg.rsp",
]

WYCHEPROOF_COMMIT = "5722833ca004983abd1a91bcb6c24596d50ac0f9"
X509_LIMBO_COMMIT = "c6040f178a947b3fa4d4a5c118d5594f0e0ca6e2"
X509_LIMBO_CASES = (
    "pathlen::intermediate-violates-pathlen-0",
    "pathlen::self-issued-certs-pathlen",
    "rfc5280::eku::ee-eku-empty",
    "rfc5280::eku::ee-wrong-eku",
    "rfc5280::eku::ee-without-eku",
    "rfc5280::validity::notbefore-exact",
    "rfc5280::validity::notafter-exact",
    "webpki::san::exact-dns-san",
    "webpki::san::mismatch-domain-san",
    "webpki::san::mismatch-subdomain-san",
    "webpki::san::leftmost-wildcard-san",
    "webpki::san::wildcard-embedded-leftmost-san",
    "webpki::san::no-san",
)


class ImportCryptologyVectorsApp(UtilityApp):
    """Import pinned official cryptology vector files."""

    description = "Import pinned NIST, BLAKE2, C2SP Wycheproof, and C2SP x509-limbo cryptology vector files."

    def __init__(self) -> None:
        super().__init__()
        self.source_directory = Path()
        self.output_directory = Path()
        self.file_update = FileUpdate(self.print_verbose)
        self.ed25519_only = False
        self.ecdsa_only = False
        self.rsa_only = False
        self.x509_limbo_only = False

    def add_command_line_args(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("source_directory", type=Path, help="Directory containing the pinned source files.")
        parser.add_argument(
            "--ed25519-only",
            action="store_true",
            help="Import only the Ed25519 signature source, then refresh shared metadata.",
        )
        parser.add_argument(
            "--ecdsa-only",
            action="store_true",
            help="Import only the three ECDSA signature sources, then refresh shared metadata.",
        )
        parser.add_argument(
            "--rsa-only",
            action="store_true",
            help="Import only the seven RSA signature sources, then refresh shared metadata.",
        )
        parser.add_argument(
            "--x509-limbo-only",
            action="store_true",
            help="Import only the selected x509-limbo cases, then refresh shared metadata.",
        )

    def handle_command_line_args(self, args: argparse.Namespace) -> None:
        self.source_directory = args.source_directory.resolve()
        self.output_directory = self.project_directory / "test" / "unittest" / "data" / "cryptology"
        self.ed25519_only = args.ed25519_only
        self.ecdsa_only = args.ecdsa_only
        self.rsa_only = args.rsa_only
        self.x509_limbo_only = args.x509_limbo_only
        if sum((self.ed25519_only, self.ecdsa_only, self.rsa_only, self.x509_limbo_only)) > 1:
            raise UtilityError("Select at most one partial import mode.")

    @staticmethod
    def digest(data: bytes) -> str:
        """Calculate the lowercase SHA-256 digest of bytes."""
        return hashlib.sha256(data).hexdigest()

    def source_data(self, source: SourceFile) -> bytes:
        """Read and verify one pinned source file."""
        path = self.source_directory / source.name
        if not path.is_file():
            raise UtilityError(f"Missing cryptology vector source: {path}")
        data = path.read_bytes()
        actual = self.digest(data)
        if actual != source.sha256:
            raise UtilityError(f"Unexpected SHA-256 for {source.name}: {actual}")
        return data

    def write(self, relative_path: str, data: bytes) -> None:
        """Write one deterministic UTF-8 fixture."""
        path = self.output_directory / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        text = data.decode("utf-8").replace("\r\n", "\n").replace("\r", "\n")
        text = "\n".join(line.rstrip(" \t") for line in text.split("\n"))
        if self.file_update.write_if_changed(path, text):
            self.print_verbose(f"Imported {relative_path}.")

    @staticmethod
    def normalized_rsp(data: bytes) -> bytes:
        """Normalize an official response file to UTF-8 with LF endings."""
        return data.decode("ascii").replace("\r\n", "\n").encode("ascii")

    def import_aes_files(self) -> None:
        """Import complete AES files whose configurations are all supported."""
        for archive_name, file_names in AES_FILES.items():
            source = SOURCES[archive_name]
            archive_path = self.source_directory / archive_name
            self.source_data(source)
            with zipfile.ZipFile(archive_path) as archive:
                for file_name in file_names:
                    self.write(f"aes/{file_name}", self.normalized_rsp(archive.read(file_name)))

    def import_sha_files(self) -> None:
        """Import all byte-oriented SHA variants supported by HashAlgorithm."""
        source = SOURCES["shabytetestvectors.zip"]
        archive_path = self.source_directory / source.name
        self.source_data(source)
        with zipfile.ZipFile(archive_path) as archive:
            for file_name in SHA_FILES:
                source_name = f"shabytetestvectors/{file_name}"
                self.write(f"sha/{file_name}", self.normalized_rsp(archive.read(source_name)))

    def import_sha3_files(self) -> None:
        """Import all byte-oriented SHA-3 variants supported by HashAlgorithm."""
        source = SOURCES["sha-3bytetestvectors.zip"]
        archive_path = self.source_directory / source.name
        self.source_data(source)
        with zipfile.ZipFile(archive_path) as archive:
            for file_name in SHA3_FILES:
                self.write(f"sha3/{file_name}", self.normalized_rsp(archive.read(file_name)))

    @staticmethod
    def filter_gcm(data: bytes) -> tuple[bytes, int, int]:
        """Keep only 96-bit-IV and 128-bit-tag groups from one GCM response file."""
        lines = data.decode("ascii").replace("\r\n", "\n").splitlines()
        comments = [line for line in lines if line.startswith("#")]
        key_length = next(line for line in lines if line.startswith("[Keylen = "))
        output = [*comments, "", key_length, ""]
        groups = 0
        records = 0
        index = 0
        while index < len(lines):
            if not lines[index].startswith("[IVlen = "):
                index += 1
                continue
            end = index + 1
            while end < len(lines) and not lines[end].startswith("[IVlen = "):
                end += 1
            section = lines[index:end]
            settings = {
                match.group(1): int(match.group(2))
                for line in section
                if (match := re.fullmatch(r"\[(\w+) = (\d+)\]", line))
            }
            if settings.get("IVlen") == 96 and settings.get("Taglen") == 128:
                output.extend(section)
                output.append("")
                groups += 1
                records += sum(line.startswith("Count = ") for line in section)
            index = end
        if groups != 25 or records != 375:
            raise UtilityError(f"Unexpected applicable GCM dimensions: {groups} groups, {records} records.")
        return ("\n".join(output).rstrip() + "\n").encode("ascii"), groups, records

    def import_gcm_files(self) -> None:
        """Import the four supported GCM key/direction files."""
        source = SOURCES["gcmtestvectors.zip"]
        archive_path = self.source_directory / source.name
        self.source_data(source)
        with zipfile.ZipFile(archive_path) as archive:
            for name in ["gcmDecrypt128.rsp", "gcmDecrypt256.rsp", "gcmEncryptExtIV128.rsp", "gcmEncryptExtIV256.rsp"]:
                filtered, _, _ = self.filter_gcm(archive.read(name))
                self.write(f"gcm/{name}", filtered)

    @staticmethod
    def filter_hmac(data: bytes, digest_length: int) -> tuple[bytes, int]:
        """Keep one complete digest-length section from the NIST HMAC response file."""
        lines = data.decode("ascii").replace("\r\n", "\n").splitlines()
        comments = [line for line in lines if line.startswith("#")]
        start = lines.index(f"[L={digest_length}]")
        end = next((index for index in range(start + 1, len(lines)) if lines[index].startswith("[L=")), len(lines))
        section = lines[start:end]
        records = sum(line.startswith("Count = ") for line in section)
        expected_records = {32: 225, 48: 300}.get(digest_length)
        if records != expected_records:
            raise UtilityError(f"Unexpected {digest_length}-byte HMAC record count: {records}.")
        output = [*comments, "", *section]
        return ("\n".join(output).rstrip() + "\n").encode("ascii"), records

    def import_hmac(self) -> None:
        """Import all SHA-256 and SHA-384 HMAC response records."""
        source = SOURCES["hmactestvectors.zip"]
        archive_path = self.source_directory / source.name
        self.source_data(source)
        with zipfile.ZipFile(archive_path) as archive:
            source_data = archive.read("HMAC.rsp")
        for digest_length, name in [(32, "HMAC_SHA256.rsp"), (48, "HMAC_SHA384.rsp")]:
            filtered, _ = self.filter_hmac(source_data, digest_length)
            self.write(f"hash/{name}", filtered)

    @staticmethod
    def filter_blake2b(data: bytes) -> tuple[bytes, int]:
        """Convert all unkeyed BLAKE2b-512 KATs into response-file form."""
        values = json.loads(data)
        records = [value for value in values if value["hash"] == "blake2b" and not value["key"]]
        if len(records) != 256:
            raise UtilityError(f"Unexpected unkeyed BLAKE2b record count: {len(records)}.")
        output = [
            "# Official BLAKE2 unkeyed BLAKE2b-512 known-answer vectors",
            "# Converted deterministically from blake2-kat.json",
            "",
            "[Hash = BLAKE2b]",
            "[Keylen = 0]",
            "[Outlen = 512]",
            "",
        ]
        for count, value in enumerate(records):
            message = value["in"]
            output.extend(
                [
                    f"Count = {count}",
                    f"Len = {len(message) * 4}",
                    f"Msg = {message}",
                    f"MD = {value['out']}",
                    "",
                ]
            )
        return ("\n".join(output).rstrip() + "\n").encode("ascii"), len(records)

    def import_blake2b(self) -> None:
        """Import the supported unkeyed BLAKE2b vectors."""
        source = SOURCES["blake2-kat.json"]
        filtered, _ = self.filter_blake2b(self.source_data(source))
        self.write("hash/BLAKE2b.rsp", filtered)

    @staticmethod
    def wycheproof_header(source: dict, title: str, source_name: str) -> list[str]:
        """Create licensing, provenance, and modification notices for a converted Wycheproof fixture."""
        vector_sources = set()
        for group in source.get("testGroups", []):
            group_source = group.get("source")
            if group_source is None:
                continue
            if not isinstance(group_source, dict):
                raise UtilityError("Unexpected Wycheproof vector source attribution.")
            name = group_source.get("name")
            version = group_source.get("version")
            if not isinstance(name, str) or not name or not isinstance(version, str) or not version:
                raise UtilityError("Unexpected Wycheproof vector source attribution.")
            vector_sources.add((name, version))
        result = [
            f"# C2SP Wycheproof {title} vectors",
            f"# Upstream file: {source_name}",
            f"# Upstream commit: {WYCHEPROOF_COMMIT}",
        ]
        if vector_sources:
            for name, version in sorted(vector_sources):
                result.append(f"# Upstream vector source: {name} {version}")
        else:
            generator_version = source.get("generatorVersion")
            if not isinstance(generator_version, str) or not generator_version:
                raise UtilityError("Wycheproof source contains no vector-source or generator attribution.")
            result.append(f"# Upstream Wycheproof generator version: {generator_version}")
        result.extend(
            [
                "# Modified by Erbsland Core: converted from JSON into deterministic response-file form",
                "# License: Apache-2.0; see the repository LICENSE file",
                "# SPDX-License-Identifier: Apache-2.0",
                "",
            ]
        )
        return result

    @staticmethod
    def filter_wycheproof_chacha20_poly1305(data: bytes) -> tuple[bytes, int, int, int, int]:
        """Convert every pinned Wycheproof ChaCha20-Poly1305 case into response-file form."""
        source = json.loads(data)
        if source.get("algorithm") != "CHACHA20-POLY1305" or source.get("numberOfTests") != 325:
            raise UtilityError("Unexpected Wycheproof ChaCha20-Poly1305 source header.")
        output = ImportCryptologyVectorsApp.wycheproof_header(
            source, "ChaCha20-Poly1305", "chacha20_poly1305_test.json"
        )
        groups = 0
        valid = 0
        authentication_failures = 0
        invalid_nonces = 0
        for group in source["testGroups"]:
            if group.get("keySize") != 256 or group.get("tagSize") != 128 or group.get("type") != "AeadTest":
                raise UtilityError("Unexpected Wycheproof ChaCha20-Poly1305 group parameters.")
            iv_size = group["ivSize"]
            output.extend([f"[IVlen = {iv_size}]", "[Keylen = 256]", "[Taglen = 128]", ""])
            groups += 1
            for test in group["tests"]:
                result = test["result"]
                flags = ",".join(test["flags"])
                if result == "valid":
                    valid += 1
                elif iv_size == 96:
                    authentication_failures += 1
                else:
                    invalid_nonces += 1
                output.extend(
                    [
                        f"Count = {test['tcId']}",
                        f"Result = {result}",
                        f"Flags = {flags}",
                        f"Key = {test['key']}",
                        f"Nonce = {test['iv']}",
                        f"AAD = {test['aad']}",
                        f"Msg = {test['msg']}",
                        f"CT = {test['ct']}",
                        f"Tag = {test['tag']}",
                        "",
                    ]
                )
        records = valid + authentication_failures + invalid_nonces
        if (groups, records, valid, authentication_failures, invalid_nonces) != (10, 325, 256, 60, 9):
            raise UtilityError(
                "Unexpected Wycheproof ChaCha20-Poly1305 dimensions: "
                f"{groups} groups, {records} records, {valid} valid, "
                f"{authentication_failures} authentication failures, {invalid_nonces} invalid nonces."
            )
        return (
            ("\n".join(output).rstrip() + "\n").encode("ascii"),
            groups,
            valid,
            authentication_failures,
            invalid_nonces,
        )

    def import_wycheproof_chacha20_poly1305(self) -> None:
        """Import all ChaCha20-Poly1305 cases from the pinned C2SP Wycheproof revision."""
        source = SOURCES["chacha20_poly1305_test.json"]
        filtered, _, _, _, _ = self.filter_wycheproof_chacha20_poly1305(self.source_data(source))
        self.write("chacha20_poly1305/wycheproof.rsp", filtered)

    @staticmethod
    def filter_wycheproof_x25519(data: bytes) -> tuple[bytes, int, int, int]:
        """Convert every pinned Wycheproof X25519 case into response-file form."""
        source = json.loads(data)
        if source.get("algorithm") != "XDH" or source.get("numberOfTests") != 518:
            raise UtilityError("Unexpected Wycheproof X25519 source header.")
        output = ImportCryptologyVectorsApp.wycheproof_header(source, "X25519", "x25519_test.json")
        valid = 0
        acceptable = 0
        zero_shared_secret = 0
        groups = 0
        for group in source["testGroups"]:
            if group.get("type") != "XdhComp" or group.get("curve") != "curve25519":
                raise UtilityError("Unexpected Wycheproof X25519 group parameters.")
            output.extend(["[Curve = curve25519]", ""])
            groups += 1
            for test in group["tests"]:
                result = test["result"]
                flags = ",".join(test["flags"])
                if result == "valid":
                    valid += 1
                elif result == "acceptable":
                    acceptable += 1
                else:
                    raise UtilityError(f"Unexpected Wycheproof X25519 result: {result}.")
                if "ZeroSharedSecret" in test["flags"]:
                    zero_shared_secret += 1
                output.extend(
                    [
                        f"Count = {test['tcId']}",
                        f"Result = {result}",
                        f"Flags = {flags}",
                        f"Public = {test['public']}",
                        f"Private = {test['private']}",
                        f"Shared = {test['shared']}",
                        "",
                    ]
                )
        if (groups, valid, acceptable, zero_shared_secret) != (1, 264, 254, 31):
            raise UtilityError(
                "Unexpected Wycheproof X25519 dimensions: "
                f"{groups} groups, {valid} valid, {acceptable} acceptable, "
                f"{zero_shared_secret} zero shared secrets."
            )
        return ("\n".join(output).rstrip() + "\n").encode("ascii"), valid, acceptable, zero_shared_secret

    def import_wycheproof_x25519(self) -> None:
        """Import all X25519 cases from the pinned C2SP Wycheproof revision."""
        source = SOURCES["x25519_test.json"]
        filtered, _, _, _ = self.filter_wycheproof_x25519(self.source_data(source))
        self.write("x25519/wycheproof.rsp", filtered)

    @staticmethod
    def filter_wycheproof_ed25519(
        data: bytes,
        expected_count: int = 151,
        expected_groups: int = 78,
        expected_valid: int = 88,
        expected_invalid: int = 63,
    ) -> tuple[bytes, int, int]:
        """Convert every pinned Wycheproof Ed25519 verification case into response-file form."""
        source = json.loads(data)
        if source.get("algorithm") != "EDDSA" or source.get("numberOfTests") != expected_count:
            raise UtilityError("Unexpected Wycheproof Ed25519 source header.")
        output = ImportCryptologyVectorsApp.wycheproof_header(source, "Ed25519", "ed25519_test.json")
        records = 0
        valid = 0
        invalid = 0
        groups = 0
        for group in source["testGroups"]:
            public_key = group.get("publicKey", {})
            if (
                group.get("type") != "EddsaVerify"
                or public_key.get("type") != "EDDSAPublicKey"
                or public_key.get("curve") != "edwards25519"
                or public_key.get("keySize") != 255
            ):
                raise UtilityError("Unexpected Wycheproof Ed25519 group parameters.")
            public_key_der = group.get("publicKeyDer")
            if public_key_der != "302a300506032b6570032100" + public_key.get("pk", ""):
                raise UtilityError("Unexpected Wycheproof Ed25519 SubjectPublicKeyInfo encoding.")
            output.extend([f"[PublicKeyDer = {public_key_der}]", "[AlgorithmDer = 300506032b6570]", ""])
            groups += 1
            for test in group["tests"]:
                result = test["result"]
                if result == "valid":
                    valid += 1
                elif result == "invalid":
                    invalid += 1
                else:
                    raise UtilityError(f"Unexpected Wycheproof Ed25519 result: {result}.")
                output.extend(
                    [
                        f"Count = {test['tcId']}",
                        f"Result = {result}",
                        f"Flags = {','.join(test['flags'])}",
                        f"Msg = {test['msg']}",
                        f"Sig = {test['sig']}",
                        "",
                    ]
                )
                records += 1
        if (records, groups, valid, invalid) != (
            expected_count,
            expected_groups,
            expected_valid,
            expected_invalid,
        ):
            raise UtilityError(
                "Unexpected Wycheproof Ed25519 dimensions: "
                f"{records} records, {groups} groups, {valid} valid, {invalid} invalid."
            )
        return ("\n".join(output).rstrip() + "\n").encode("ascii"), valid, invalid

    def import_wycheproof_ed25519(self) -> None:
        """Import all Ed25519 cases from the pinned C2SP Wycheproof revision."""
        source = SOURCES["ed25519_test.json"]
        filtered, _, _ = self.filter_wycheproof_ed25519(self.source_data(source))
        self.write("signature/ED25519.rsp", filtered)

    @staticmethod
    def ecdsa_algorithm_der(sha: str) -> str:
        """Build a supported ECDSA X.509 signature AlgorithmIdentifier."""
        if sha == "SHA-256":
            return "300a06082a8648ce3d040302"
        if sha == "SHA-384":
            return "300a06082a8648ce3d040303"
        raise UtilityError("Unexpected ECDSA signature hash.")

    @staticmethod
    def ecdsa_public_key_der(curve: str, x: str, y: str) -> str:
        """Build an uncompressed id-ecPublicKey SubjectPublicKeyInfo."""
        widths = {"P-256": 64, "P-384": 96}
        width = widths.get(curve)
        if width is None or len(x) != width or len(y) != width:
            raise UtilityError("Unexpected NIST ECDSA public-key coordinate width.")
        if curve == "P-256":
            return "3059301306072a8648ce3d020106082a8648ce3d03010703420004" + x + y
        return "3076301006072a8648ce3d020106052b8104002203620004" + x + y

    @staticmethod
    def der_integer(value: str) -> str:
        """Encode one nonnegative hexadecimal integer as canonical DER."""
        value = value.lstrip("0") or "0"
        if len(value) % 2 != 0:
            value = "0" + value
        if int(value[:2], 16) >= 0x80:
            value = "00" + value
        length = len(value) // 2
        if length >= 128:
            raise UtilityError("ECDSA DER INTEGER exceeds the one-octet length bound.")
        return f"02{length:02x}{value}"

    @staticmethod
    def ecdsa_signature_der(r: str, s: str) -> str:
        """Encode an ECDSA `(r,s)` pair as canonical ECDSA-Sig-Value DER."""
        contents = ImportCryptologyVectorsApp.der_integer(r) + ImportCryptologyVectorsApp.der_integer(s)
        length = len(contents) // 2
        if length >= 128:
            raise UtilityError("ECDSA-Sig-Value exceeds the one-octet length bound.")
        return f"30{length:02x}{contents}"

    @staticmethod
    def filter_wycheproof_ecdsa(
        data: bytes, source_name: str, expected_curve: str, expected_sha: str, expected_count: int, expected_groups: int
    ) -> bytes:
        """Convert every selected pinned Wycheproof ECDSA case into response-file form."""
        source = json.loads(data)
        if source.get("algorithm") != "ECDSA" or source.get("numberOfTests") != expected_count:
            raise UtilityError("Unexpected Wycheproof ECDSA source header.")
        output = ImportCryptologyVectorsApp.wycheproof_header(
            source, f"ECDSA {expected_curve} {expected_sha}", source_name
        )
        records = 0
        groups = 0
        for group in source["testGroups"]:
            key = group.get("publicKey")
            expected_size = 256 if expected_curve == "secp256r1" else 384
            if (
                group.get("type") != "EcdsaVerify"
                or group.get("sha") != expected_sha
                or not isinstance(key, dict)
                or key.get("curve") != expected_curve
                or key.get("keySize") != expected_size
            ):
                raise UtilityError("Unexpected Wycheproof ECDSA group parameters.")
            output.extend(
                [
                    f"[Curve = {expected_curve}]",
                    f"[Hash = {expected_sha}]",
                    f"[PublicKeyDer = {group['publicKeyDer']}]",
                    f"[AlgorithmDer = {ImportCryptologyVectorsApp.ecdsa_algorithm_der(expected_sha)}]",
                    "",
                ]
            )
            groups += 1
            for test in group["tests"]:
                if test["result"] not in ("valid", "invalid"):
                    raise UtilityError(f"Unexpected Wycheproof ECDSA result: {test['result']}.")
                output.extend(
                    [
                        f"Count = {test['tcId']}",
                        f"Result = {test['result']}",
                        f"Flags = {','.join(test['flags'])}",
                        f"Msg = {test['msg']}",
                        f"Sig = {test['sig']}",
                        "",
                    ]
                )
                records += 1
        if (records, groups) != (expected_count, expected_groups):
            raise UtilityError(f"Unexpected Wycheproof ECDSA dimensions: {records} records, {groups} groups.")
        return ("\n".join(output).rstrip() + "\n").encode("ascii")

    def import_wycheproof_ecdsa(self) -> None:
        """Import both supported ECDSA corpora from the pinned C2SP Wycheproof revision."""
        files = [
            ("ecdsa_secp256r1_sha256_test.json", "secp256r1", "SHA-256", 484, 113, "ECDSA_P256_SHA256.rsp"),
            ("ecdsa_secp384r1_sha384_test.json", "secp384r1", "SHA-384", 504, 105, "ECDSA_P384_SHA384.rsp"),
        ]
        for source_name, curve, sha, count, groups, output_name in files:
            filtered = self.filter_wycheproof_ecdsa(
                self.source_data(SOURCES[source_name]), source_name, curve, sha, count, groups
            )
            self.write(f"signature/{output_name}", filtered)

    @staticmethod
    def filter_nist_ecdsa(data: bytes) -> tuple[bytes, int]:
        """Select the matched P-256/SHA-256 and P-384/SHA-384 NIST SigVer cases."""
        output = [
            "# NIST CAVP FIPS 186-4 ECDSA SigVer vectors",
            "# Upstream archive: 186-4ecdsatestvectors.zip, member 186-4ecdsatestvectors/SigVer.rsp",
            "# Modified by Erbsland Core: selected matched curves and encoded SPKI/signatures as canonical DER",
            "",
        ]
        selected = {"P-256,SHA-256", "P-384,SHA-384"}
        section = ""
        record: dict[str, str] = {}
        count = 0

        def finish_record() -> None:
            nonlocal record, count
            if not record or section not in selected:
                record = {}
                return
            required = {"Msg", "Qx", "Qy", "R", "S", "Result"}
            if set(record) != required:
                raise UtilityError("Unexpected NIST ECDSA SigVer record fields.")
            curve, sha = section.split(",")
            result = "valid" if record["Result"].startswith("P ") else "invalid"
            output.extend(
                [
                    f"[Curve = {curve}]",
                    f"[Hash = {sha}]",
                    f"[PublicKeyDer = {ImportCryptologyVectorsApp.ecdsa_public_key_der(curve, record['Qx'], record['Qy'])}]",
                    f"[AlgorithmDer = {ImportCryptologyVectorsApp.ecdsa_algorithm_der(sha)}]",
                    "",
                    f"Count = {count}",
                    f"Result = {result}",
                    f"Flags = {record['Result']}",
                    f"Msg = {record['Msg']}",
                    f"Sig = {ImportCryptologyVectorsApp.ecdsa_signature_der(record['R'], record['S'])}",
                    "",
                ]
            )
            count += 1
            record = {}

        text = data.decode("ascii").replace("\r\n", "\n").replace("\r", "\n")
        for source_line in text.split("\n"):
            line = source_line.strip()
            if not line or line.startswith("#"):
                finish_record()
                continue
            if line.startswith("[") and line.endswith("]"):
                finish_record()
                section = line[1:-1]
                continue
            if section in selected:
                name, separator, value = line.partition("=")
                if not separator:
                    raise UtilityError("Malformed NIST ECDSA SigVer line.")
                record[name.strip()] = value.strip()
        finish_record()
        if count != 30:
            raise UtilityError(f"Unexpected selected NIST ECDSA SigVer record count: {count}.")
        return ("\n".join(output).rstrip() + "\n").encode("ascii"), count

    def import_nist_ecdsa(self) -> None:
        """Import the matched NIST ECDSA signature-verification cases."""
        source = SOURCES["186-4ecdsatestvectors.zip"]
        with zipfile.ZipFile(self.source_directory / source.name) as archive:
            self.source_data(source)
            data = archive.read("186-4ecdsatestvectors/SigVer.rsp")
        filtered, _ = self.filter_nist_ecdsa(data)
        self.write("signature/ECDSA_NIST_SIGVER.rsp", filtered)

    def import_ecdsa(self) -> None:
        """Import every supported ECDSA signature source."""
        self.import_nist_ecdsa()
        self.import_wycheproof_ecdsa()

    @staticmethod
    def rsa_algorithm_der(padding: str, sha: str, salt_length: int) -> str:
        """Build the one supported X.509 signature AlgorithmIdentifier encoding."""
        if padding == "PKCS1" and sha == "SHA-256" and salt_length == 0:
            return "300d06092a864886f70d01010b0500"
        if padding == "PKCS1" and sha == "SHA-384" and salt_length == 0:
            return "300d06092a864886f70d01010c0500"
        hash_oid = {"SHA-256": "01", "SHA-384": "02"}.get(sha)
        maximum_salt_length = {"SHA-256": 32, "SHA-384": 48}.get(sha)
        if (
            padding != "PSS"
            or hash_oid is None
            or maximum_salt_length is None
            or not 0 <= salt_length <= maximum_salt_length
        ):
            raise UtilityError("Unexpected supported RSA signature AlgorithmIdentifier parameters.")
        # RFC 4055: id-RSASSA-PSS with explicit hash, matching MGF1 hash, and the exact salt length.
        return (
            "304106092a864886f70d01010a3034"
            f"a00f300d06096086480165030402{hash_oid}0500"
            f"a11c301a06092a864886f70d010108300d06096086480165030402{hash_oid}0500"
            f"a2030201{salt_length:02x}"
        )

    @staticmethod
    def filter_wycheproof_rsa(
        data: bytes,
        source_name: str,
        expected_algorithm: str,
        expected_sha: str,
        expected_count: int,
        expected_group_type: str | None = None,
    ) -> bytes:
        """Convert every supported pinned Wycheproof RSA signature case into response-file form."""
        source = json.loads(data)
        if source.get("algorithm") != expected_algorithm or source.get("numberOfTests") != expected_count:
            raise UtilityError("Unexpected Wycheproof RSA signature source header.")
        if expected_algorithm == "RSASSA-PSS":
            padding = "PSS"
        elif expected_algorithm == "RSASSA-PKCS1-v1_5":
            padding = "PKCS1"
        else:
            raise UtilityError("Unexpected supported Wycheproof RSA signature algorithm.")
        if expected_group_type is None:
            expected_group_type = "RsassaPssVerify" if padding == "PSS" else "RsassaPkcs1Verify"
        output = ImportCryptologyVectorsApp.wycheproof_header(
            source, f"{expected_algorithm} {expected_sha}", source_name
        )
        records = 0
        for group in source["testGroups"]:
            salt_length = group.get("sLen", 0)
            if (
                group.get("type") != expected_group_type
                or group.get("keySize") != 2048
                or group.get("sha") != expected_sha
                or (padding == "PSS" and (group.get("mgf") != "MGF1" or group.get("mgfSha") != expected_sha))
            ):
                raise UtilityError("Unexpected Wycheproof RSA signature group parameters.")
            output.extend(
                [
                    f"[Padding = {padding}]",
                    f"[Hash = {expected_sha}]",
                    f"[SaltLength = {salt_length}]",
                    f"[PublicKeyDer = {group['publicKeyDer']}]",
                    f"[AlgorithmDer = {ImportCryptologyVectorsApp.rsa_algorithm_der(padding, expected_sha, salt_length)}]",
                    "",
                ]
            )
            for test in group["tests"]:
                if test["result"] not in ("valid", "invalid", "acceptable"):
                    raise UtilityError(f"Unexpected Wycheproof RSA signature result: {test['result']}.")
                output.extend(
                    [
                        f"Count = {test['tcId']}",
                        f"Result = {test['result']}",
                        f"Flags = {','.join(test['flags'])}",
                        f"Msg = {test['msg']}",
                        f"Sig = {test['sig']}",
                        "",
                    ]
                )
                records += 1
        if records != expected_count:
            raise UtilityError(f"Unexpected Wycheproof RSA signature record count: {records}.")
        return ("\n".join(output).rstrip() + "\n").encode("ascii")

    def import_wycheproof_rsa(self) -> None:
        """Import all supported RSA signature cases from the pinned C2SP Wycheproof revision."""
        files = [
            ("rsa_pss_2048_sha256_mgf1_0_test.json", "RSASSA-PSS", "SHA-256", 103, "PSS_SHA256_SALT0.rsp", None),
            ("rsa_pss_2048_sha256_mgf1_32_test.json", "RSASSA-PSS", "SHA-256", 108, "PSS_SHA256.rsp", None),
            (
                "rsa_pss_2048_sha256_mgf1_0_params_test.json",
                "RSASSA-PSS",
                "SHA-256",
                103,
                "PSS_SHA256_PSS_KEY_SALT0.rsp",
                "RsassaPssWithParametersVerify",
            ),
            (
                "rsa_pss_2048_sha256_mgf1_32_params_test.json",
                "RSASSA-PSS",
                "SHA-256",
                108,
                "PSS_SHA256_PSS_KEY_SALT32.rsp",
                "RsassaPssWithParametersVerify",
            ),
            ("rsa_pss_2048_sha384_mgf1_48_test.json", "RSASSA-PSS", "SHA-384", 141, "PSS_SHA384.rsp", None),
            (
                "rsa_signature_2048_sha256_test.json",
                "RSASSA-PKCS1-v1_5",
                "SHA-256",
                259,
                "PKCS1_SHA256.rsp",
                None,
            ),
            (
                "rsa_signature_2048_sha384_test.json",
                "RSASSA-PKCS1-v1_5",
                "SHA-384",
                258,
                "PKCS1_SHA384.rsp",
                None,
            ),
        ]
        for source_name, algorithm, sha, count, output_name, group_type in files:
            filtered = self.filter_wycheproof_rsa(
                self.source_data(SOURCES[source_name]), source_name, algorithm, sha, count, group_type
            )
            self.write(f"signature/{output_name}", filtered)

    @staticmethod
    def certificate_pem_to_der_hex(pem: str) -> str:
        """Convert one strict CERTIFICATE PEM value into lowercase DER hex."""
        lines = pem.strip().splitlines()
        if lines[0] != "-----BEGIN CERTIFICATE-----" or lines[-1] != "-----END CERTIFICATE-----":
            raise UtilityError("Unexpected x509-limbo certificate PEM boundary.")
        try:
            return base64.b64decode("".join(lines[1:-1]), validate=True).hex()
        except ValueError as exception:
            raise UtilityError("Unexpected x509-limbo certificate Base64.") from exception

    @staticmethod
    def filter_x509_limbo(data: bytes) -> bytes:
        """Select policy-compatible server-authentication cases from pinned C2SP x509-limbo JSON."""
        source = json.loads(data)
        testcases = source.get("testcases")
        if not isinstance(testcases, list) or len(testcases) != 9786:
            raise UtilityError("Unexpected x509-limbo source dimensions.")
        by_id = {testcase.get("id"): testcase for testcase in testcases}
        selected = []
        for testcase_id in X509_LIMBO_CASES:
            testcase = by_id.get(testcase_id)
            if testcase is None:
                raise UtilityError(f"Missing selected x509-limbo testcase: {testcase_id}")
            if testcase.get("validation_kind") != "SERVER":
                raise UtilityError(f"Selected x509-limbo testcase is not a server validation: {testcase_id}")
            trusted = testcase.get("trusted_certs")
            intermediates = testcase.get("untrusted_intermediates")
            peer_name = testcase.get("expected_peer_name")
            if not isinstance(trusted, list) or len(trusted) != 1 or not isinstance(intermediates, list):
                raise UtilityError(f"Unexpected selected x509-limbo path inputs: {testcase_id}")
            if not isinstance(peer_name, dict) or peer_name.get("kind") != "DNS":
                raise UtilityError(f"Unexpected selected x509-limbo identity: {testcase_id}")
            selected.append(testcase)

        lines = [
            "# C2SP x509-limbo server-certificate path-validation subset",
            "# SPDX-License-Identifier: Apache-2.0",
            f"# Upstream commit: {X509_LIMBO_COMMIT}",
            "# Modified by Erbsland Core: selected compatible cases and converted certificates to DER hex",
            "# Excluded features: RFC 4518 name equivalence, IDNA conversion, name constraints, policies, revocation",
            "",
            f"[SourceCommit = {X509_LIMBO_COMMIT}]",
            "",
        ]
        for index, testcase in enumerate(selected):
            peer_name = testcase["expected_peer_name"]
            intermediates = testcase["untrusted_intermediates"]
            lines.extend(
                [
                    f"Count = {index}",
                    f"Id = {testcase['id']}",
                    f"Expected = {testcase['expected_result']}",
                    f"ValidationTime = {testcase.get('validation_time') or ''}",
                    f"IdentityKind = {peer_name['kind']}",
                    f"Identity = {peer_name['value']}",
                    f"PeerDer = {ImportCryptologyVectorsApp.certificate_pem_to_der_hex(testcase['peer_certificate'])}",
                    f"AnchorDer = {ImportCryptologyVectorsApp.certificate_pem_to_der_hex(testcase['trusted_certs'][0])}",
                    f"IntermediateCount = {len(intermediates)}",
                ]
            )
            lines.extend(
                f"Intermediate{intermediate_index}Der = "
                f"{ImportCryptologyVectorsApp.certificate_pem_to_der_hex(certificate)}"
                for intermediate_index, certificate in enumerate(intermediates)
            )
            lines.append("")
        return ("\n".join(lines) + "\n").encode("ascii")

    def import_x509_limbo(self) -> None:
        """Import the selected scope-compatible subset from pinned C2SP x509-limbo."""
        source = SOURCES["limbo.json"]
        self.write("x509/x509_limbo.rsp", self.filter_x509_limbo(self.source_data(source)))

    @staticmethod
    def record_count(path: Path, text: str) -> int:
        """Count complete records in a generated or existing response file."""
        if "Monte" in path.name:
            return len(re.findall(r"^MD = ", text, re.MULTILINE))
        if path.name.startswith(("ECB", "CBC")):
            return len(re.findall(r"^COUNT = ", text, re.MULTILINE))
        if path.name.startswith("SHA"):
            return len(re.findall(r"^MD = ", text, re.MULTILINE))
        return len(re.findall(r"^Count = ", text, re.MULTILINE))

    @staticmethod
    def group_count(path: Path, text: str) -> int:
        """Count semantic response groups in a generated fixture."""
        if path.parent.name == "aes":
            return len(re.findall(r"^\[(?:ENCRYPT|DECRYPT)\]$", text, re.MULTILINE))
        if path.parent.name == "gcm":
            return len(re.findall(r"^\[IVlen = ", text, re.MULTILINE))
        if path.parent.name == "chacha20_poly1305":
            return len(re.findall(r"^\[IVlen = ", text, re.MULTILINE))
        if path.parent.name == "x25519":
            return len(re.findall(r"^\[Curve = ", text, re.MULTILINE))
        if path.parent.name == "signature":
            return len(re.findall(r"^\[PublicKeyDer = ", text, re.MULTILINE))
        if path.parent.name == "x509":
            return len(re.findall(r"^\[SourceCommit = ", text, re.MULTILINE))
        if path.name == "BLAKE2b.rsp":
            return len(re.findall(r"^\[Hash = ", text, re.MULTILINE))
        return len(re.findall(r"^\[L ?=", text, re.MULTILINE))

    def write_manifest(self) -> None:
        """Write fixture digests and expected record/group counts."""
        root = self.project_directory / "test" / "unittest" / "data" / "cryptology"
        paths = sorted(path for path in root.rglob("*.rsp") if path.name != "manifest.rsp")
        lines = [
            "# path sha256 records groups",
        ]
        for path in paths:
            data = path.read_bytes()
            text = data.decode("ascii")
            relative = path.relative_to(root).as_posix()
            records = self.record_count(path, text)
            groups = self.group_count(path, text)
            lines.append(f"{relative} {self.digest(data)} {records} {groups}")
        self.write("manifest.txt", ("\n".join(lines) + "\n").encode("ascii"))

    def write_readme(self) -> None:
        """Write human-readable fixture provenance."""
        lines = [
            "****************************",
            "Cryptology Vector Provenance",
            "****************************",
            "",
            "The files in this directory are generated by ``import_cryptology_vectors`` from pinned upstream files.",
            "The unit tests never download data.",
            "Running the complete importer requires all twenty-two source files in one directory.",
            "",
            "Licensing",
            "=========",
            "",
            (
                "The C2SP Wycheproof and x509-limbo sources and the response files derived from them are licensed under the "
                "Apache License, Version 2.0."
            ),
            "The complete license text is in the repository's ``LICENSE`` file.",
            (
                "Each derived response file retains the upstream file, pinned commit, available vector-source or "
                "generator-version"
            ),
            "attribution, and a prominent conversion notice.",
            "The pinned Wycheproof revision contains no ``NOTICE`` file.",
            (
                "Distributions that contain these fixtures independently of the source tree must include the "
                "repository ``LICENSE`` file"
            ),
            "and this provenance document.",
            "",
            "Sources",
            "=======",
            "",
        ]
        for source in SOURCES.values():
            lines.extend([f"* ``{source.name}``: {source.url}", f"  SHA-256: ``{source.sha256}``"])
        lines.extend(
            [
                "",
                "Selection",
                "=========",
                "",
                "* AES files contain AES-128/AES-256 ECB and AES-256 CBC configurations supported by the library.",
                "* GCM files contain all records with a 96-bit IV and 128-bit tag: 25 groups and 375 records per file.",
                "* HMAC contains all 225 SHA-256 records and all 300 SHA-384 records.",
                "* BLAKE2b contains all 256 unkeyed BLAKE2b-512 records.",
                "* ChaCha20-Poly1305 contains all 325 cases from the pinned C2SP Wycheproof revision:",
                "  256 valid cases, 60 authentication failures, and 9 invalid nonce sizes.",
                "* X25519 contains all 518 cases from the pinned C2SP Wycheproof revision:",
                "  264 valid cases and 254 acceptable cases, including 31 forbidden all-zero shared secrets.",
                "* Ed25519 contains all 151 cases from the pinned C2SP Wycheproof revision:",
                "  88 valid cases and 63 invalid cases covering encoding, malleability, and overflow boundaries.",
                "* ECDSA contains all 988 P-256/SHA-256 and P-384/SHA-384 cases from the pinned C2SP",
                "  Wycheproof revision, plus all 30 matching NIST CAVP FIPS 186-4 SigVer cases.",
                "* RSA signatures contain all 1080 selected 2048-bit SHA-256/SHA-384 PKCS#1 v1.5 and PSS cases",
                "  including zero-length salts and PSS-restricted keys, from the pinned C2SP Wycheproof revision;",
                "  MissingNull cases are rejected by the strict DER policy.",
                "* X.509 validation contains 13 selected server-authentication cases from the pinned C2SP",
                "  x509-limbo revision, covering path length, self-issued CAs, EKU, validity boundaries, SAN",
                "  wildcards, absence, and mismatches. RFC 4518 name equivalence, IDNA conversion,",
                "  name constraints, policies, and revocation cases are excluded from this initial policy scope.",
                "* SHA and SHA-3 contain all byte-oriented short-message, long-message, and Monte Carlo files for",
                "  the variants supported by ``HashAlgorithm``.",
                "",
                "These vectors provide implementation validation; they are not evidence of CAVP certification.",
                "",
            ]
        )
        self.write("README.rst", "\n".join(lines).encode("utf-8"))

    def run(self, argv=None) -> None:
        super().run(argv)
        require_directory(self.source_directory, "Cryptology vector source directory")
        self.output_directory.mkdir(parents=True, exist_ok=True)
        if self.ed25519_only:
            self.import_wycheproof_ed25519()
            self.write_manifest()
            self.write_readme()
            return
        if self.ecdsa_only:
            self.import_ecdsa()
            self.write_manifest()
            self.write_readme()
            return
        if self.rsa_only:
            self.import_wycheproof_rsa()
            self.write_manifest()
            self.write_readme()
            return
        if self.x509_limbo_only:
            self.import_x509_limbo()
            self.write_manifest()
            self.write_readme()
            return
        self.import_aes_files()
        self.import_sha_files()
        self.import_sha3_files()
        self.import_gcm_files()
        self.import_hmac()
        self.import_blake2b()
        self.import_wycheproof_chacha20_poly1305()
        self.import_wycheproof_x25519()
        self.import_wycheproof_ed25519()
        self.import_ecdsa()
        self.import_wycheproof_rsa()
        self.import_x509_limbo()
        self.write_manifest()
        self.write_readme()


def main() -> None:
    raise SystemExit(ImportCryptologyVectorsApp().main())


if __name__ == "__main__":
    main()
