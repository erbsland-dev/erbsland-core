#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
"""Regenerate the PUBLIC demonstration keys; OpenSSL is needed only for this step."""

import shutil
import subprocess
import tempfile
from pathlib import Path


def generate() -> None:
    root = Path(__file__).resolve().parent.parent
    with tempfile.TemporaryDirectory(prefix="mail-certificates-") as temporary:
        work = Path(temporary)

        def openssl(*arguments: str) -> None:
            subprocess.run(["openssl", *arguments], cwd=work, check=True, capture_output=True)

        openssl("genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048", "-out", "ca-key.pem")
        (work / "ca.cnf").write_text(
            "[req]\ndistinguished_name=dn\nx509_extensions=ca\n[dn]\n[ca]\n"
            "basicConstraints=critical,CA:TRUE\nkeyUsage=critical,keyCertSign,cRLSign\n"
            "subjectKeyIdentifier=hash\nauthorityKeyIdentifier=keyid:always\n"
        )
        openssl(
            "req",
            "-config",
            "ca.cnf",
            "-new",
            "-x509",
            "-sha256",
            "-key",
            "ca-key.pem",
            "-days",
            "3650",
            "-subj",
            "/CN=Erbsland Mail Demo CA",
            "-out",
            "ca.pem",
        )
        for role in ("server", "client"):
            openssl("genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048", "-out", f"{role}-key.pem")
            openssl(
                "req",
                "-new",
                "-sha256",
                "-key",
                f"{role}-key.pem",
                "-subj",
                f"/CN=Mail Demo {role}",
                "-out",
                f"{role}.csr",
            )
            extensions = "basicConstraints=critical,CA:FALSE\nkeyUsage=critical,digitalSignature\n"
            extensions += "subjectKeyIdentifier=hash\nauthorityKeyIdentifier=keyid,issuer\n"
            extensions += f"extendedKeyUsage={role}Auth\n"
            if role == "server":
                extensions += "subjectAltName=DNS:localhost,IP:127.0.0.1,IP:::1\n"
            (work / "extensions.cnf").write_text(extensions)
            openssl(
                "x509",
                "-req",
                "-sha256",
                "-in",
                f"{role}.csr",
                "-CA",
                "ca.pem",
                "-CAkey",
                "ca-key.pem",
                "-set_serial",
                "2" if role == "server" else "3",
                "-days",
                "3650",
                "-extfile",
                "extensions.cnf",
                "-out",
                f"{role}.pem",
            )
        for app, role in (("MailSender", "client"), ("MailReceiver", "server")):
            destination = root / app / "conf" / f"{role}-certificates"
            destination.mkdir(parents=True, exist_ok=True)
            for name in ("ca.pem", f"{role}.pem", f"{role}-key.pem"):
                shutil.copyfile(work / name, destination / name)
        # The disposable CA signing key is deliberately not distributed.


if __name__ == "__main__":
    generate()
