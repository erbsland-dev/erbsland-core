#  Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
#  SPDX-License-Identifier: Apache-2.0

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
release = "1.0"

# -- General configuration ---------------------------------------------------
extensions = [
    "erbsland_mermaid",
    "erbsland_badges",
    "erbsland_index",
    "sphinx_rtd_theme",
    "sphinx_design",
    "sphinx_copybutton",
    "breathe",
    "erbsland.sphinx.ansi",
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
breathe_projects_source = {"erbsland-core": (_processed_src_dir, ["erbsland"])}
breathe_doxygen_config_options = {
    "STRIP_FROM_PATH": _processed_src_dir,
    "STRIP_FROM_INC_PATH": _processed_src_dir,
    "JAVADOC_AUTOBRIEF": "yes",
    "ALIASES": '"tested{1}=@verbatim embed:rst^^:el-tested:`\\1`^^@endverbatim", '
    '"notest{1}=@verbatim embed:rst^^:el-notest:`\\1`^^@endverbatim", '
    '"needtest{1}=@verbatim embed:rst^^:el-notest:`\\1`^^@endverbatim", '
    '"usesunidb{1}=@verbatim embed:rst^^:el-unicode-db:`\\1`^^@endverbatim", '
    '"wip=@par Work in Progress:^^", '
    '"seedoc{1}=@verbatim embed:rst^^See: :doc:`\\1`^^@endverbatim", '
    '"seeref{1}=@verbatim embed:rst^^See: :ref:`\\1`^^@endverbatim"',
    "ENABLE_PREPROCESSING": "yes",
    "MACRO_EXPANSION": "no",
    "RECURSIVE": "yes",
}
