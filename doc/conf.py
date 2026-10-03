#  Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
#  SPDX-License-Identifier: Apache-2.0

import os
import re
import subprocess
import sys
from pathlib import Path
from datetime import date

_doc_dir = Path(__file__).parent
sys.path.insert(0, str(_doc_dir))
sys.path.insert(0, str(_doc_dir / "_ext"))

# -- Project information -----------------------------------------------------
project = "Erbsland Core"
copyright = f"{date.today().year}, Tobias Erbsland - Erbsland DEV"
author = "Tobias Erbsland - Erbsland DEV"
# Only published documentation needs a release version. GitHub checks out the complete tag history.
version = release = "development"
if os.environ.get("GITHUB_ACTIONS") == "true":
    _tag = subprocess.check_output(["git", "describe", "--tags", "--abbrev=0"], cwd=_doc_dir.parent, text=True).strip()
    _tag_version = re.fullmatch(r"v?(\d+)\.(\d+)\.(\d+)", _tag)
    if _tag_version is None:
        raise RuntimeError(f"The documentation release tag is not a version tag: {_tag!r}")
    version = ".".join(_tag_version.groups()[:2])
    release = ".".join(_tag_version.groups())

# -- General configuration ---------------------------------------------------
extensions = [
    "erbsland_mermaid",
    "erbsland_badges",
    "erbsland_index",
    "sphinx_rtd_theme",
    "sphinx_design",
    "sphinx_copybutton",
    "breathe",
    "erbsland_breathe",
    "erbsland_doxygen",
    "erbsland.sphinx.ansi",
    "erbsland_draft",
    "erbsland_demo",
    "erbsland_styles",
]
templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

# -- Options for HTML output -------------------------------------------------
html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]
html_css_files = [
    "custom.css",
]
html_js_files = [
    "https://erbsland.dev/ext/fa7/js/all.min.js",
    "https://erbsland.dev/ext/mermaid/11/mermaid.min.js",
]

# -- Options for Breathe -----------------------------------------------------
_project_dir = Path(__file__).parent.parent
_processed_src_dir = _project_dir / "_doxygen_input"
breathe_projects = {"erbsland-core": _project_dir / "_build/breathe/doxygen/erbsland-core/xml"}
breathe_default_project = "erbsland-core"
# erbsland_doxygen caches XML generation. Breathe's automatic source processing
# reruns Doxygen on every build and invalidates unchanged reference pages.
breathe_doxygen_config_options = {
    "STRIP_FROM_PATH": _processed_src_dir,
    "STRIP_FROM_INC_PATH": _processed_src_dir,
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
