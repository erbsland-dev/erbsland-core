# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

"""Cache Doxygen generation across Sphinx and sphinx-autobuild invocations."""

from __future__ import annotations

import hashlib
import os
import re
import subprocess
from pathlib import Path

from sphinx.application import Sphinx
from sphinx.errors import ExtensionError
from sphinx.util import logging

logger = logging.getLogger(__name__)
_INCLUDE_GRAPHS = re.compile(rb"    <(incdepgraph|invincdepgraph)>.*?</\1>\r?\n", re.DOTALL)


def remove_include_graphs(xml_dir: Path) -> None:
    """Remove unused include graphs that Doxygen emits even when their flags are off.

    These generated XML elements cannot contain nested elements of the same name;
    byte replacement preserves every other part of the XML, including verbatim RST.
    No project directive uses :allow-dot-graphs: on a file.
    """
    for path in xml_dir.glob("*.xml"):
        original = path.read_bytes()
        compact = _INCLUDE_GRAPHS.sub(b"", original)
        if compact != original:
            path.write_bytes(compact)


def doxygen_configuration(app: Sphinx, source_dir: Path, output_dir: Path) -> str:
    """Render the same XML-only input and aliases previously supplied by Breathe."""
    lines = [
        'PROJECT_NAME = "erbsland-core"',
        f'OUTPUT_DIRECTORY = "{output_dir}"',
        f'INPUT = "{source_dir / "erbsland"}"',
        "GENERATE_HTML = NO",
        "GENERATE_LATEX = NO",
        "GENERATE_MAN = NO",
        "GENERATE_RTF = NO",
        "GENERATE_XML = YES",
        "CASE_SENSE_NAMES = NO",
        "QUIET = YES",
        'ALIASES = rst="\\verbatim embed:rst"',
        'ALIASES += endrst="\\endverbatim"',
        'ALIASES += inlinerst="\\verbatim embed:rst:inline"',
    ]
    for name, value in app.config.breathe_doxygen_config_options.items():
        if isinstance(value, Path):
            value = f'"{value}"'
        lines.append(f"{name} = {value}")
    for name, value in app.config.breathe_doxygen_aliases.items():
        lines.append(f'ALIASES += {name}="{value}"')
    return "\n".join(lines) + "\n"


def preprocess_sources() -> Path:
    """Invoke the selected C++ preprocessor and return its generated input directory."""
    project = Path(__file__).resolve().parents[2]
    executable = Path(
        os.environ.get(
            "ERBSLAND_CORE_TOOL",
            project / "tool" / "bin" / ("erbsland-core-tool.exe" if os.name == "nt" else "erbsland-core-tool"),
        )
    )
    if not executable.is_file():
        raise ExtensionError(
            "The maintenance tool is missing. Run setup.py or set ERBSLAND_CORE_TOOL to a built executable."
        )
    try:
        subprocess.run([str(executable), "doc-preprocess-sources", "--project", str(project)], cwd=project, check=True)
    except (OSError, subprocess.CalledProcessError) as error:
        raise ExtensionError(f"Doxygen source preprocessing failed: {error}") from error
    return project / "_doxygen_input"


def prepare_doxygen(app: Sphinx) -> None:
    """Refresh XML only when preprocessed sources or Doxygen settings changed."""
    source_dir = preprocess_sources()
    output_dir = Path(app.config.breathe_projects["erbsland-core"]).parent
    configuration = doxygen_configuration(app, source_dir, output_dir)
    digest = hashlib.sha256(configuration.encode("utf-8"))
    digest.update((source_dir / ".preprocess-sources-run").read_bytes())
    digest.update(Path(__file__).read_bytes())
    fingerprint = digest.hexdigest()
    stamp = output_dir / ".doxygen-input-sha256"
    index = output_dir / "xml" / "index.xml"
    if index.is_file() and stamp.is_file() and stamp.read_text(encoding="utf-8") == fingerprint:
        logger.info("Doxygen XML is up to date.")
        return

    # A failed/interrupted run must never leave a valid cache stamp behind.
    stamp.unlink(missing_ok=True)
    output_dir.mkdir(parents=True, exist_ok=True)
    config_path = output_dir.parent / "erbsland-core.cfg"
    config_path.write_text(configuration, encoding="utf-8")
    logger.info("Generating Doxygen XML (source or configuration changed)...")
    try:
        subprocess.run(["doxygen", str(config_path)], cwd=output_dir.parent, check=True)
    except (OSError, subprocess.CalledProcessError) as error:
        raise ExtensionError(f"Doxygen generation failed: {error}") from error
    if not index.is_file():
        raise ExtensionError(f"Doxygen did not generate {index}")
    remove_include_graphs(index.parent)
    stamp.write_text(fingerprint, encoding="utf-8")


def setup(app: Sphinx) -> dict[str, bool | str]:
    """Prepare cached XML before Sphinx checks Breathe's file dependencies."""
    app.connect("builder-inited", prepare_doxygen)
    return {"version": "1", "parallel_read_safe": True, "parallel_write_safe": True}
