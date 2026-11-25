# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Bootstrap repository development using only Python's standard library."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


class SetupError(RuntimeError):
    """A bootstrap step could not finish safely."""


def run(command: list[str], root: Path, label: str) -> None:
    """Run a bootstrap command and retain its output and failure status."""
    print(f"{label}...", flush=True)
    allowed = {
        "HOME",
        "LANG",
        "LC_ALL",
        "LC_CTYPE",
        "PATH",
        "TMPDIR",
        "TEMP",
        "TMP",
        "SYSTEMROOT",
        "WINDIR",
        "INCLUDE",
        "LIB",
        "LIBPATH",
        "COMSPEC",
        "PATHEXT",
        "VCINSTALLDIR",
        "VCTOOLSINSTALLDIR",
        "WINDOWSSDKDIR",
        "WINDOWSSDKVERSION",
        "UNIVERSALCRTSDKDIR",
        "UCRTVERSION",
    }
    environment = {name: value for name, value in os.environ.items() if name.upper() in allowed}
    try:
        subprocess.run(command, cwd=root, env=environment, check=True)
    except FileNotFoundError as error:
        raise SetupError(f"{label}: could not find {command[0]!r}.") from error
    except subprocess.CalledProcessError as error:
        raise SetupError(f"{label}: command exited with status {error.returncode}.") from error


def install_binary(root: Path, build: Path, tool: str) -> None:
    """Install a successfully built executable and the optional POSIX shortcut."""
    name = f"{tool}.exe" if os.name == "nt" else tool
    source = build / "tool" / name
    if not source.is_file():
        raise SetupError(f"The build did not produce {source}.")
    destination = root / "tool" / "bin" / name
    shortcut = root / tool
    if os.name != "nt" and shortcut.exists() and not shortcut.is_symlink():
        raise SetupError(f"Refusing to replace unrelated file {shortcut}.")
    if os.name != "nt" and shortcut.is_symlink() and os.readlink(shortcut) != f"tool/bin/{tool}":
        raise SetupError(f"Refusing to replace unrelated symbolic link {shortcut}.")
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(f".{name}.new")
    if temporary.is_symlink() or temporary.exists():
        raise SetupError(f"Refusing to overwrite existing installation temporary file {temporary}.")
    try:
        shutil.copy2(source, temporary)
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)
    if os.name != "nt" and not shortcut.is_symlink():
        shortcut.symlink_to(f"tool/bin/{tool}")
    print(f"Installed {destination}", flush=True)


def prepare_development(
    root: Path, tool: str, requirements: list[str], submodules: list[str], release_options: list[str]
) -> None:
    """Prepare a repository's development environment and install its Release maintenance command."""
    venv = root / ".venv"
    python = venv / ("Scripts/python.exe" if os.name == "nt" else "bin/python")
    if not venv.exists():
        run([sys.executable, "-m", "venv", str(venv)], root, "Creating .venv")
    elif not python.is_file():
        raise SetupError(f"Existing virtual environment is incomplete: {python}.")
    run(
        [str(python), "-m", "pip", "install", *(arg for path in requirements for arg in ("-r", path))],
        root,
        "Installing Python requirements",
    )
    run(
        [
            "git",
            "submodule",
            "update",
            "--init",
            "--recursive",
            *submodules,
        ],
        root,
        "Initializing test submodules",
    )
    run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            "cmake-build-debug",
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DCMAKE_UNITY_BUILD=ON",
        ],
        root,
        "Configuring Debug development build",
    )
    build = root / f"cmake-build-{tool}-release"
    run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_UNITY_BUILD=ON",
            *release_options,
        ],
        root,
        f"Configuring Release {tool} build",
    )
    run(
        ["cmake", "--build", str(build), "--target", tool, "--parallel"],
        root,
        f"Building Release {tool}",
    )
    install_binary(root, build, tool)
