# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
"""Check real configuration files and the CLI's reporting and error-limit behavior."""

from pathlib import Path
import subprocess
import sys
import tempfile


def main() -> None:
    executable = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="config-check-") as temporary:
        root = Path(temporary)
        fixtures = {
            "server.elcl": '[Server]\nHost: "127.0.0.1"\nPort: 8080\n',
            "worker config-é.elcl": '[Worker]\nName: "Index builder"\n',
            "broken.elcl": "[Server]\nPort: ???\n",
            "damaged.elcl": "[Worker\nParallel Jobs: 4\n",
            "includes.elcl": '@include: "server.elcl"\n',
            "missing-include.elcl": '@include: "missing.elcl"\n',
        }
        for name, content in fixtures.items():
            (root / name).write_text(content, encoding="utf-8")

        def run(*arguments: str, success: bool = True) -> subprocess.CompletedProcess[str]:
            result = subprocess.run([executable, *arguments], cwd=root, capture_output=True, text=True, timeout=5)
            assert (result.returncode == 0) == success, (arguments, result.returncode, result.stdout, result.stderr)
            return result

        def summary(result: subprocess.CompletedProcess[str], text: str) -> None:
            assert result.stdout.splitlines()[-1] == text, result.stdout

        help_output = run("--help")
        assert "--max-errors" in help_output.stdout
        good = run("server.elcl", "worker config-é.elcl")
        summary(good, "Checked 2 of 2 files: 2 passed, 0 failed, 0 skipped.")
        assert "worker config-é.elcl" in good.stdout and not good.stderr

        all_files = ("broken.elcl", "server.elcl", "damaged.elcl", "worker config-é.elcl")
        complete = run(*all_files, success=False)
        summary(complete, "Checked 4 of 4 files: 2 passed, 2 failed, 0 skipped.")
        assert complete.stderr.index("broken.elcl") < complete.stderr.index("damaged.elcl"), complete.stderr
        assert "OK  server.elcl" in complete.stdout and "OK  worker config-é.elcl" in complete.stdout

        limited = run("--max-errors", "1", *all_files, success=False)
        summary(limited, "Checked 1 of 4 files: 0 passed, 1 failed, 3 skipped.")
        assert "broken.elcl" in limited.stderr and "damaged.elcl" not in limited.stderr
        assert "OK  " not in limited.stdout

        limited = run("--max-errors", "2", *all_files, success=False)
        summary(limited, "Checked 3 of 4 files: 1 passed, 2 failed, 1 skipped.")
        assert "OK  server.elcl" in limited.stdout and "worker config-é.elcl" not in limited.stdout

        missing = run("missing-é.elcl", "server.elcl", success=False)
        summary(missing, "Checked 2 of 2 files: 1 passed, 1 failed, 0 skipped.")
        assert "missing-é.elcl" in missing.stderr

        included = run("includes.elcl", "missing-include.elcl", "server.elcl", success=False)
        summary(included, "Checked 3 of 3 files: 2 passed, 1 failed, 0 skipped.")
        assert "missing.elcl" in included.stderr

        negative = run("--max-errors=-1", "server.elcl", success=False)
        assert "non-negative" in negative.stderr and "Checked" not in negative.stdout
        no_arguments = run()
        assert "Usage:" in no_arguments.stdout and "Checked" not in no_arguments.stdout
        run("--max-errors", "1", success=False)
        for name, content in fixtures.items():
            assert (root / name).read_text(encoding="utf-8") == content, name
    print("ConfigCheck: parsing, ordered diagnostics, error limits, includes, and CLI validation passed.")


if __name__ == "__main__":
    main()
