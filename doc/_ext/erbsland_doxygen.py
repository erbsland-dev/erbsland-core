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


def default_config_options(source_dir: Path) -> dict:
    """Return the shared Erbsland API documentation conventions."""
    return {
        "STRIP_FROM_PATH": source_dir,
        "STRIP_FROM_INC_PATH": source_dir,
        "JAVADOC_AUTOBRIEF": "yes",
        # Reference pages explicitly select the public API they render. Missing
        # comments on every parsed implementation detail are therefore not an
        # actionable diagnostic; malformed documentation remains reported.
        "WARN_IF_UNDOCUMENTED": "no",
        "ALIASES": '"tested{1}=@verbatim embed:rst^^:el-tested:`\\1`^^@endverbatim", '
        '"notest{1}=@verbatim embed:rst^^:el-notest:`\\1`^^@endverbatim", '
        '"needtest{1}=@verbatim embed:rst^^:el-needtest:`\\1`^^@endverbatim", '
        '"usesunidb{1}=@verbatim embed:rst^^:el-unicode-db:`\\1`^^@endverbatim", '
        '"wip=@par Work in Progress:^^", '
        '"seedoc{1}=@verbatim embed:rst^^See: :doc:`\\1`^^@endverbatim", '
        '"seeref{1}=@verbatim embed:rst^^See: :ref:`\\1`^^@endverbatim"',
        "ENABLE_PREPROCESSING": "yes",
        "MACRO_EXPANSION": "no",
        "RECURSIVE": "yes",
        # Reference directives render declarations and comments, not source listings.
        "XML_PROGRAMLISTING": "no",
    }


def doxygen_configuration(app: Sphinx, source_dir: Path, output_dir: Path) -> str:
    """Render the same XML-only input and aliases previously supplied by Breathe."""
    lines = [
        f'PROJECT_NAME = "{app.config.breathe_default_project}"',
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


def preprocess_sources(project: Path, tool_name: str) -> Path:
    """Invoke the selected C++ preprocessor and return its generated input directory."""
    variable = tool_name.upper().replace("-", "_")
    executable = Path(
        os.environ.get(
            variable,
            project / "tool" / "bin" / (f"{tool_name}.exe" if os.name == "nt" else tool_name),
        )
    )
    if not executable.is_file():
        raise ExtensionError(
            f"The maintenance tool is missing. Run setup.py or set {variable} to a built executable."
        )
    try:
        subprocess.run([str(executable), "doc-preprocess-sources", "--project", str(project)], cwd=project, check=True)
    except (OSError, subprocess.CalledProcessError) as error:
        raise ExtensionError(f"Doxygen source preprocessing failed: {error}") from error
    return project / "_doxygen_input"


def prepare_doxygen(app: Sphinx) -> None:
    """Refresh XML only when preprocessed sources or Doxygen settings changed."""
    project_name = app.config.breathe_default_project
    source_dir = preprocess_sources(Path(app.confdir).parent, app.config.erbsland_doc_tool)
    output_dir = Path(app.config.breathe_projects[project_name]).parent
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
    config_path = output_dir.parent / f"{project_name}.cfg"
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
    app.add_config_value("erbsland_doc_tool", "erbsland-core-tool", "env")
    app.connect("builder-inited", prepare_doxygen)
    return {"version": "1", "parallel_read_safe": True, "parallel_write_safe": True}
