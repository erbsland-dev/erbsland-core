# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
"""Regenerate public client-authentication fixtures using OpenSSL; no private keys are retained."""

from pathlib import Path
import subprocess
import tempfile


def run(*args: str) -> None:
    subprocess.run(["openssl", *args], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)


def main() -> None:
    destination = Path(__file__).parent / "client_authentication.pem"
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        key = str(root / "key.pem")
        run("ecparam", "-name", "prime256v1", "-genkey", "-noout", "-out", key)
        ca = str(root / "root.pem")
        config = root / "root.cnf"
        config.write_text(
            "[req]\ndistinguished_name=dn\nx509_extensions=ca\n[dn]\n[ca]\nbasicConstraints=critical,CA:true\nkeyUsage=critical,keyCertSign,cRLSign\n"
        )
        run(
            "req",
            "-new",
            "-x509",
            "-key",
            key,
            "-subj",
            "/CN=Client Authentication Test Root",
            "-days",
            "3650",
            "-config",
            str(config),
            "-out",
            ca,
        )
        certificates = [Path(ca).read_text()]
        cases = [
            ("Client", "extendedKeyUsage=clientAuth\n", False),
            ("Server", "extendedKeyUsage=serverAuth\n", False),
            ("Absent EKU", "", False),
            ("Empty EKU", "2.5.29.37=DER:30:00\n", False),
            ("Wrong KU", "extendedKeyUsage=clientAuth\n", True),
            ("Unknown Critical", "extendedKeyUsage=clientAuth\n1.2.3.4=critical,DER:05:00\n", False),
        ]

        def issue(name: str, extensions: str, issuer: str, serial: int) -> str:
            csr = str(root / "request.pem")
            result = str(root / "result.pem")
            ext = root / "extension.cnf"
            ext.write_text(extensions)
            run("req", "-new", "-key", key, "-subj", f"/CN={name}", "-out", csr)
            run(
                "x509",
                "-req",
                "-in",
                csr,
                "-CA",
                issuer,
                "-CAkey",
                key,
                "-set_serial",
                str(serial),
                "-days",
                "3650",
                "-sha256",
                "-extfile",
                str(ext),
                "-out",
                result,
            )
            return Path(result).read_text()

        for index, (name, eku, wrong_ku) in enumerate(cases, 2):
            usage = "keyEncipherment" if wrong_ku else "digitalSignature"
            certificates.append(
                issue(name, f"basicConstraints=critical,CA:false\nkeyUsage=critical,{usage}\n{eku}", ca, index)
            )
        for index, purpose in enumerate(["serverAuth", "clientAuth"], 20):
            intermediate = root / "intermediate.pem"
            intermediate.write_text(
                issue(
                    f"{purpose} Intermediate",
                    f"basicConstraints=critical,CA:true\nkeyUsage=critical,keyCertSign\nextendedKeyUsage={purpose}\n",
                    ca,
                    index,
                )
            )
            certificates.append(intermediate.read_text())
            certificates.append(
                issue(
                    "Client through Intermediate",
                    "basicConstraints=critical,CA:false\nkeyUsage=critical,digitalSignature\nextendedKeyUsage=clientAuth\n",
                    str(intermediate),
                    index + 10,
                )
            )
        destination.write_text("".join(certificates))


if __name__ == "__main__":
    main()
