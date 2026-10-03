#!/usr/bin/env python3
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
"""Local-only end-to-end tests for the two consumer applications."""

import re
import shutil
import socket
import ssl
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path

BIN = Path(sys.argv.pop(1)).resolve()


class Receiver:
    def __init__(self, config="mail_receiver.elcl", once=True, timeout=8):
        self.directory = tempfile.TemporaryDirectory(prefix="mail receiver ")
        self.output = Path(self.directory.name) / "console.log"
        self.stream = self.output.open("w")
        args = [
            str(BIN / "mail_receiver"),
            "--config",
            str(BIN / config),
            "--port",
            "0",
            "--verbose",
            "--timeout",
            str(timeout),
        ]
        if once:
            args.append("--once")
        self.process = subprocess.Popen(args, stdout=self.stream, stderr=subprocess.STDOUT)
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            match = re.search(r"Listening on 127\.0\.0\.1:(\d+)", self.text())
            if match:
                self.port = int(match[1])
                return
            if self.process.poll() is not None:
                break
            time.sleep(0.01)
        message = self.text()
        self.close()
        raise AssertionError(f"Receiver did not start: {message}")

    def text(self):
        return self.output.read_text()

    def wait(self):
        return self.process.wait(timeout=5)

    def close(self):
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=3)
        self.stream.close()
        self.directory.cleanup()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


def send(receiver, *arguments, config="mail_sender.elcl", cwd=None):
    return subprocess.run(
        [
            str(BIN / "mail_sender"),
            "--config",
            str(BIN / config),
            "--smtp-port",
            str(receiver.port),
            "--verbose",
            *arguments,
        ],
        cwd=cwd,
        capture_output=True,
        text=True,
        timeout=6,
    )


def context(identity=False):
    result = ssl.create_default_context(cafile=str(BIN / "client-certificates/ca.pem"))
    result.minimum_version = ssl.TLSVersion.TLSv1_3
    if identity:
        result.load_cert_chain(BIN / "client-certificates/client.pem", BIN / "client-certificates/client-key.pem")
    return result


class MailIntegrationTest(unittest.TestCase):
    def assertSent(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("Mail exchange completed", result.stdout)
        self.assertIn("close_notify exchange completed", result.stdout)

    def test_basic_and_explicit_overrides(self):
        with Receiver() as receiver, tempfile.TemporaryDirectory(prefix="mail cwd ") as cwd:
            result = send(
                receiver,
                "--sender-name",
                "",
                "--receiver-email",
                "override@example.test",
                "--message",
                "First line\n.dot\n.\nlast",
                cwd=cwd,
            )
            self.assertSent(result)
            self.assertEqual(receiver.wait(), 0)
            log = receiver.text()
            self.assertIn("override@example.test", log)
            self.assertIn(".dot", log)
            self.assertNotIn("..dot", log)
            self.assertIn('From: ""', log)

    def test_sni_and_numeric_default(self):
        with Receiver("mail_receiver_sni.elcl") as receiver:
            self.assertSent(send(receiver, config="mail_sender_identity.elcl"))
            self.assertEqual(receiver.wait(), 0)
            self.assertIn("Selected server profile tls/mail/identified", receiver.text())
            self.assertIn("Client certificate authenticated", receiver.text())
        with Receiver("mail_receiver_sni.elcl") as receiver:
            self.assertSent(send(receiver, "--smtp-host", "127.0.0.1"))
            self.assertEqual(receiver.wait(), 0)
            self.assertNotIn("tls/mail/identified", receiver.text())

    def test_required_client_identity_and_retry(self):
        with Receiver("mail_receiver_required.elcl") as receiver:
            rejected = send(receiver)
            self.assertNotEqual(rejected.returncode, 0, rejected.stdout)
            self.assertIsNone(receiver.process.poll())
            self.assertSent(send(receiver, config="mail_sender_identity.elcl"))
            self.assertEqual(receiver.wait(), 0)

    def test_optional_identity_and_repeated_connections(self):
        with Receiver("mail_receiver_optional.elcl", once=False) as receiver:
            self.assertSent(send(receiver))
            self.assertSent(send(receiver, config="mail_sender_identity.elcl"))
            self.assertIsNone(receiver.process.poll())
            self.assertEqual(receiver.text().count("Received mail from"), 2)

    def test_optional_identity_rejects_wrong_certificate_purpose(self):
        with Receiver("mail_receiver_optional.elcl") as receiver:
            invalid = context()
            invalid.load_cert_chain(BIN / "server-certificates/server.pem", BIN / "server-certificates/server-key.pem")
            with self.assertRaises((ssl.SSLError, ConnectionError)):
                with socket.create_connection(("127.0.0.1", receiver.port), timeout=3) as transport:
                    with invalid.wrap_socket(transport, server_hostname="localhost") as connection:
                        # TLS 1.3 can report the peer's certificate rejection on the first read.
                        connection.recv(1024)
            self.assertIsNone(receiver.process.poll())
            self.assertNotIn("Received mail", receiver.text())
            self.assertSent(send(receiver))
            self.assertEqual(receiver.wait(), 0)

    def test_untrusted_server_and_input_rejection(self):
        with Receiver() as receiver, tempfile.TemporaryDirectory() as temporary:
            config = Path(temporary) / "untrusted.elcl"
            original = (BIN / "mail_sender.elcl").read_text()
            config.write_text(
                original.split("*[TLS.Client]")[0] + '*[TLS.Client]\nLabel: "tls/mail"\nSystem Trust: true\n'
            )
            result = send(receiver, config=str(config))
            self.assertNotEqual(result.returncode, 0)
            self.assertNotIn("Server certificate authenticated", result.stdout)
            self.assertNotIn("Received mail", receiver.text())
            for arguments in (
                ("--message", "Grüezi"),
                ("--sender-email", "x@example.test\nInjected: yes"),
                ("--message", "x" * 999),
                ("--smtp-port", "0"),
            ):
                result = send(receiver, *arguments)
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn("Connecting to", result.stdout)
            self.assertSent(send(receiver))
            self.assertEqual(receiver.wait(), 0)

    def test_multiblock_message(self):
        with Receiver() as receiver:
            body = "\n".join(f"Line {index:03d}: " + "x" * 400 for index in range(100))
            self.assertSent(send(receiver, "--message", body))
            self.assertEqual(receiver.wait(), 0)
            self.assertEqual(receiver.text().count("Line "), 100)
            self.assertIn("Line 099:", receiver.text())

    def test_fragmented_commands_and_orderly_close(self):
        with Receiver() as receiver:
            with socket.create_connection(("127.0.0.1", receiver.port), timeout=3) as transport:
                with context().wrap_socket(transport, server_hostname="localhost") as connection:
                    reader = connection.makefile("rb")
                    self.assertTrue(reader.readline().startswith(b"220"))
                    for command, code in (
                        (b"HELO localhost", b"250"),
                        (b"MAIL FROM:<a@example.test>", b"250"),
                        (b"RCPT TO:<b@example.test>", b"250"),
                        (b"DATA", b"354"),
                    ):
                        for byte in command + b"\r\n":
                            connection.sendall(bytes([byte]))
                        self.assertTrue(reader.readline().startswith(code))
                    connection.sendall(b"From: a@example.test\r\n\r\n..dot\r\n.\r\n")
                    self.assertTrue(reader.readline().startswith(b"250"))
                    connection.sendall(b"QUIT\r\n")
                    self.assertTrue(reader.readline().startswith(b"221"))
                    reader.close()
                    connection.unwrap().close()
            self.assertEqual(receiver.wait(), 0)
            self.assertIn(".dot", receiver.text())

    def test_abrupt_disconnect_then_valid_delivery(self):
        with Receiver() as receiver:
            transport = socket.create_connection(("127.0.0.1", receiver.port), timeout=3)
            connection = context().wrap_socket(transport, server_hostname="localhost")
            connection.recv(1024)
            connection.close()  # No TLS close_notify.
            deadline = time.monotonic() + 2
            while "without close_notify" not in receiver.text() and time.monotonic() < deadline:
                time.sleep(0.01)
            self.assertIn("without close_notify", receiver.text())
            self.assertSent(send(receiver))
            self.assertEqual(receiver.wait(), 0)

    def test_configuration_paths_and_file_logging(self):
        with tempfile.TemporaryDirectory(prefix="mail configuration ") as temporary:
            directory = Path(temporary)
            shutil.copytree(BIN / "server-certificates", directory / "server-certificates")
            config = directory / "receiver settings.elcl"
            config.write_text(
                (BIN / "mail_receiver.elcl")
                .read_text()
                .replace('# Log File: "received-mail.log"', 'Log File: "received-mail.log"')
            )
            with Receiver(str(config)) as receiver:
                self.assertSent(send(receiver, cwd=directory))
                self.assertEqual(receiver.wait(), 0)
                file_log = (directory / "received-mail.log").read_text()
                self.assertIn("Received mail from", file_log)
                self.assertIn("observation notes", file_log)
                self.assertNotIn("\x1b[", file_log)
        result = subprocess.run(
            [str(BIN / "mail_receiver"), "--tls-profile", "missing"], capture_output=True, text=True, timeout=3
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("Listening", result.stdout)

    def test_malformed_session_does_not_stop_service(self):
        with Receiver() as receiver:
            with socket.create_connection(("127.0.0.1", receiver.port), timeout=3) as transport:
                with context().wrap_socket(transport, server_hostname="localhost") as connection:
                    connection.recv(1024)
                    connection.sendall(b"x" * 1000)
                    self.assertEqual(connection.recv(1024), b"")
            self.assertSent(send(receiver))
            self.assertEqual(receiver.wait(), 0)

    def test_timeout_and_help(self):
        with Receiver(once=False, timeout=1) as receiver:
            with socket.create_connection(("127.0.0.1", receiver.port), timeout=2):
                self.assertEqual(receiver.wait(), 0)
            self.assertIn("Stopping", receiver.text())
        with tempfile.TemporaryDirectory(prefix="mail help ") as cwd:
            for app in ("mail_sender", "mail_receiver"):
                result = subprocess.run([str(BIN / app), "--help"], cwd=cwd, capture_output=True, text=True, timeout=3)
                self.assertEqual(result.returncode, 0)
                self.assertIn("--config", result.stdout)
                self.assertNotIn("\x1b[", result.stdout)


if __name__ == "__main__":
    unittest.main()
