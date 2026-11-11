# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Exercise the complete Git wrapper in disposable repositories, including worker failure shutdown."""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main() -> None:
    executable = str(Path(sys.argv[1]).resolve())
    git = shutil.which("git")
    if git is None:
        print("Git is not installed; skipping repository integration checks.")
        sys.exit(77)
    environment = os.environ | {"GIT_CONFIG_NOSYSTEM": "1", "GIT_CONFIG_GLOBAL": os.devnull}

    def git_run(root: Path, *arguments: str, expected: int = 0) -> None:
        result = subprocess.run(
            [
                git,
                "-c",
                "user.name=Demo",
                "-c",
                "user.email=demo@example.invalid",
                "-c",
                "commit.gpgsign=false",
                "-C",
                str(root),
                *arguments,
            ],
            env=environment,
            capture_output=True,
            text=True,
            timeout=10,
        )
        assert result.returncode == expected, (arguments, result.stdout, result.stderr)

    def run(root: Path, *, success: bool = True, env: dict[str, str] | None = None) -> str:
        result = subprocess.run(
            [executable, str(root)],
            env=env or environment,
            capture_output=True,
            text=True,
            timeout=10,
        )
        assert (result.returncode == 0) == success, (result.returncode, result.stdout, result.stderr)
        if not success:
            assert "Staged  Unstaged" not in result.stdout, result.stdout
            assert "Working tree is clean" not in result.stdout, result.stdout
        return result.stdout + result.stderr

    def write(root: Path, name: str, content: str) -> None:
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def initialize(root: Path) -> None:
        root.mkdir()
        git_run(root, "init", "--quiet", "--initial-branch=main")
        write(root, "README", "base\n")
        write(root, "src/file", "base\n")
        write(root, "old/move", "rename me\n")
        git_run(root, "add", ".")
        git_run(root, "commit", "--quiet", "-m", "Initial files")

    def rows(output: str) -> dict[str, tuple[int, ...]]:
        result = {}
        for line in output.splitlines()[1:]:
            staged, unstaged, untracked, conflicted, directory = line.split(maxsplit=4)
            result[directory] = tuple(map(int, (staged, unstaged, untracked, conflicted)))
        assert list(result) == sorted(result), output
        return result

    with tempfile.TemporaryDirectory(prefix="git-change-summary-") as temporary:
        base = Path(temporary)
        repository = base / "repository"
        initialize(repository)
        assert "Working tree is clean." in run(repository)
        default = subprocess.run(
            [executable],
            cwd=repository,
            env=environment,
            capture_output=True,
            text=True,
            timeout=10,
        )
        assert default.returncode == 0 and "Working tree is clean." in default.stdout, default

        write(repository, "src/file", "staged\n")
        git_run(repository, "add", "src/file")
        write(repository, "src/file", "staged and unstaged\n")
        write(repository, "README", "changed\n")
        write(repository, "docs/nested/a b-é.txt", "untracked\n")
        (repository / "new").mkdir()
        git_run(repository, "mv", "old/move", "new/move")
        expected = {"(root)": (0, 1, 0, 0), "docs/": (0, 0, 1, 0), "new/": (1, 0, 0, 0), "src/": (1, 1, 0, 0)}
        assert rows(run(repository)) == expected
        assert rows(run(repository / "src")) == expected

        conflicts = base / "conflicts"
        initialize(conflicts)
        git_run(conflicts, "checkout", "--quiet", "-b", "topic")
        write(conflicts, "src/file", "topic\n")
        git_run(conflicts, "commit", "--quiet", "-am", "Topic change")
        git_run(conflicts, "checkout", "--quiet", "main")
        write(conflicts, "src/file", "main\n")
        git_run(conflicts, "commit", "--quiet", "-am", "Main change")
        git_run(conflicts, "merge", "--no-edit", "topic", expected=1)
        assert rows(run(conflicts)) == {"src/": (0, 0, 0, 1)}

        run(base / "absent", success=False)
        assert "Git status failed" in run(base, success=False)
        assert "Git is unavailable" in run(repository, success=False, env=environment | {"PATH": str(base / "empty")})
    print("Passed clean, changed, renamed, conflicted, default-directory, and failure-shutdown scenarios.")


if __name__ == "__main__":
    main()
