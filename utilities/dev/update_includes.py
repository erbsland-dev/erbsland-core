# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from collections import defaultdict
from pathlib import Path

from lib.config import read_elcl_file, validate_local_names, validate_source_relative_path
from lib.copyright import HeaderConfig
from lib.error import UtilityError
from lib.file_update import FileUpdate
from lib.path_safety import require_directory, require_safe_existing_file
from lib.utility import UtilityApp


class UpdateIncludesApp(UtilityApp):
    """
    The working set for this script.
    """

    description = "Generate public include wrappers and `all.hpp` header files."

    def __init__(self):
        """
        Create a new empty working set.
        """
        super().__init__()
        self.project_dir = Path()  # The project directory
        self.generated_header = ""  # The generated header text.
        self.generated_include_header = ""  # The compact generated include wrapper header text.
        self.exclude_dirs: list[str] = []  # Parent directories to include.
        self.create_global_includes: bool = False  # If includes of submodules shall also copy into global space.
        self.fold_into_parent: set[str] = set()  # Directory basenames to fold.
        self.create_all_base_dir = ""  # The base dir from where `all.hpp` header shall be created.
        self.src_dir = Path()  # The directory with the sources.
        self.include_dir = Path()  # The directory with the includes.
        self.dir_map: dict[str, list[str]] = defaultdict(list)  # A map with all project subdirectories.
        self.file_map: dict[str, Path] = {}  # A map with all header file names and their paths
        self.folded_include_map: dict[str, list[tuple[str, Path]]] = defaultdict(
            list
        )  # Parent dir to folded include entries `(relative_include, source_path)`.
        self.generated_include_paths: set[Path] = set()  # All public include wrappers generated in this run.
        self.file_update = FileUpdate(self.print_verbose)

    def write_all_header(self, header_dir, header_files):
        """
        Write a single 'in source' header file.

        :param header_dir: The directory of the header file.
        :param header_files: A list of header files to include.
        """
        header_path = self.src_dir / header_dir / "all.hpp"
        self.print_verbose(f"- writing: {header_dir}/all.hpp")
        self.file_update.write_if_changed(header_path, self.create_all_header_text(header_files))

    def create_all_header_text(self, header_files: list[str]) -> str:
        """
        Create the text for a generated source `all.hpp` file.

        :param header_files: A list of header files to include.
        :return: The generated header text.
        """
        text = f"{self.generated_header}\n\n"
        for header_file in header_files:
            text += f'#include "{header_file}"\n'
        text += "\n\n"
        return text

    @staticmethod
    def is_public_header_file_name(file_name: str) -> bool:
        """
        Test if a header file name is a public include target.

        :param file_name: The header file name to test.
        :return: If the file name shall be published or added to `all.hpp`.
        """
        file_path = Path(file_name)
        return file_path.suffix == ".hpp" and "_" not in file_path.stem

    def source_relative_target_path(self, source_path: Path, target_path: Path) -> Path:
        """
        Calculate the relative target path used by an include wrapper.

        :param source_path: The wrapper path below the `include` directory.
        :param target_path: The source header path below the `src` directory.
        :return: The relative include path from the wrapper to the source header.
        """
        up_count = len(source_path.relative_to(self.project_dir).parts) - 1
        return Path(("../" * up_count) + "src/erbsland") / target_path.relative_to(self.src_dir)

    def write_include_header(self, target_path: Path, source_path: Path | None = None):
        """
        Write an include header that points to a header in the `src` directory.

        :param target_path: The path to the header file in the `src` directory.
        :param source_path: The optional relative path in the `include` directory.
        """
        if not target_path.is_file():
            raise UtilityError(f"Header file {target_path} not found.")
        if not source_path:
            source_path = Path(target_path.name)
        source_path = self.include_dir / source_path
        rel_target_path = self.source_relative_target_path(source_path, target_path)
        self.print_verbose(f'  writing "{source_path.relative_to(self.project_dir)}" -> "{rel_target_path}"')
        text = f'{self.generated_include_header}\n#include "{rel_target_path}"\n'
        self.generated_include_paths.add(source_path)
        self.file_update.write_if_changed(source_path, text)

    def should_skip_header_file(self, path: Path) -> bool:
        """
        Test if a scanned header should be skipped.

        :param path: The header path below the source directory.
        :return: If this file shall be skipped.
        """
        if path.name in {"all.hpp", "fwd.hpp"}:
            self.print_verbose(f'  Skipping "{path.name}": {path.relative_to(self.src_dir)}')
            return True
        rel_path = path.relative_to(self.src_dir)
        if not self.is_public_header_file_name(path.name):
            self.print_verbose(f"  Skipping private header: {rel_path}")
            return True
        if any((exclude_dir in rel_path.parts[:-1]) for exclude_dir in self.exclude_dirs):
            self.print_verbose(f"  Skipping excluded: {path.relative_to(self.src_dir)}")
            return True
        return len(rel_path.parts) < 2

    def record_header_file(self, path: Path) -> None:
        """
        Record one scanned header file for later generated outputs.

        :param path: The header path below the source directory.
        """
        rel_path = path.relative_to(self.src_dir)
        header_dir = rel_path.parent.as_posix()
        header_file_name = rel_path.name
        self.dir_map[str(header_dir)].append(header_file_name)
        self.print_verbose(f'Adding header "{header_file_name}" from dir "{header_dir}"')
        if header_file_name in self.file_map:
            raise UtilityError(f"Header file name conflict with `{header_file_name}`!")
        self.file_map[header_file_name] = path

    def collect_header_files(self):
        """
        Scan all subdirectories and generate leaf to root.
        """
        self.print_verbose("Scanning source dir")
        for path in self.src_dir.rglob("*.hpp"):
            require_safe_existing_file(path, "Header file", FileUpdate.MAX_COMPARE_FILE_SIZE)
            if self.should_skip_header_file(path):
                continue
            self.record_header_file(path)

    def collect_folded_parent_candidates(self) -> dict[tuple[str, str], list[Path]]:
        """
        Collect headers that are candidates to be additionally exposed in their parent directory.

        :return: A map of `(parent_dir, header_name)` to candidate source-relative paths.
        """
        parent_candidates: dict[tuple[str, str], list[Path]] = defaultdict(list)
        for path in self.file_map.values():
            rel_path = path.relative_to(self.src_dir)
            if rel_path.parent.name not in self.fold_into_parent:
                continue
            parent_dir = rel_path.parent.parent.as_posix()
            parent_candidates[(parent_dir, rel_path.name)].append(rel_path)
        return parent_candidates

    def add_effective_folded_parent_candidate(
        self, parent_dir: str, header_name: str, candidate_paths: list[Path]
    ) -> None:
        """
        Add one folded parent include candidate if it is unambiguous.

        :param parent_dir: The source-relative parent directory.
        :param header_name: The header file name.
        :param candidate_paths: All candidates found for this parent/header pair.
        """
        if header_name in self.dir_map.get(parent_dir, []):
            self.print_verbose(f'  skipping folded "{header_name}" in "{parent_dir}" due to local conflict')
            return
        if len(candidate_paths) > 1:
            self.print_verbose(f'  skipping folded "{header_name}" in "{parent_dir}" due to multiple folded candidates')
            return
        source_rel_path = candidate_paths[0]
        relative_include = source_rel_path.relative_to(Path(parent_dir)).as_posix()
        self.folded_include_map[parent_dir].append((relative_include, self.src_dir / source_rel_path))
        self.print_verbose(f'  folding "{source_rel_path.as_posix()}" into parent "{parent_dir}"')

    def _create_effective_parent_include_map(self):
        """
        Build a map of folded headers that shall be additionally exposed in the parent.
        """
        self.print_verbose("Building folded parent include map")
        parent_candidates = self.collect_folded_parent_candidates()
        for (parent_dir, header_name), candidate_paths in sorted(parent_candidates.items()):
            self.add_effective_folded_parent_candidate(parent_dir, header_name, candidate_paths)

    def _create_all_header_for_dir(self, header_dir: str):
        # Only create `all.hpp` in subdirectories of `create_all_base_dir`
        if self.create_all_base_dir:
            if not header_dir.startswith(self.create_all_base_dir + "/"):
                return False
        else:
            if not header_dir:
                return False
        return True

    @staticmethod
    def _is_root_header_dir(header_dir: str) -> bool:
        return header_dir in {"", "."}

    def _create_all_path_for_base_dir(self, header_dir: str) -> Path:
        if self._is_root_header_dir(header_dir):
            return Path("all.hpp")
        if self.create_all_base_dir:
            header_dir = header_dir.replace(self.create_all_base_dir + "/", "")
            return Path(f'{self.create_all_base_dir}/all_{header_dir.replace("/", "_")}.hpp')
        return Path(f'all_{header_dir.replace("/", "_")}.hpp')

    def generate_all_headers(self):
        """
        Write the `all.hpp` files.
        """
        self.print_verbose('Writing the "all.hpp" files to the `src` directory.')
        all_header_dirs = sorted(set(self.dir_map.keys()) | set(self.folded_include_map.keys()))
        for header_dir in all_header_dirs:
            if not self._create_all_header_for_dir(header_dir):
                continue
            header_files = list(self.dir_map.get(header_dir, []))
            header_files.extend(relative_include for relative_include, _ in self.folded_include_map.get(header_dir, []))
            header_files.sort()
            self.write_all_header(header_dir, header_files)

    def generate_includes(self):
        """
        Write all files in the `include` directory.
        """
        self.print_verbose("Writing the individual header files in the `include` directory.")
        self.generated_include_paths.clear()
        self.generate_all_include_wrappers()
        self.generate_header_include_wrappers()
        self.generate_folded_include_wrappers()

    def generate_all_include_wrappers(self):
        """
        Write public wrappers for generated source `all.hpp` files.
        """
        include_dirs = sorted(set(self.dir_map.keys()) | set(self.folded_include_map.keys()))
        for header_dir in include_dirs:
            if not self._create_all_header_for_dir(header_dir):
                continue
            target_path = self.src_dir / header_dir / "all.hpp"
            flat_all_path = self._create_all_path_for_base_dir(header_dir)
            nested_all_path = Path("all.hpp") if self._is_root_header_dir(header_dir) else Path(f"{header_dir}/all.hpp")
            self.write_include_header(target_path, flat_all_path)
            if flat_all_path != nested_all_path:
                self.write_include_header(target_path, nested_all_path)

    def generate_header_include_wrappers(self):
        """
        Write public wrappers for each scanned source header.
        """
        for header_file, path in self.file_map.items():
            self.write_include_header(path, path.relative_to(self.src_dir))
            if self.create_global_includes:
                self.write_include_header(path, Path(header_file))

    def generate_folded_include_wrappers(self):
        """
        Write additional public wrappers for headers folded into their parent directory.
        """
        for parent_dir, folded_entries in self.folded_include_map.items():
            for _, source_path in folded_entries:
                self.write_include_header(source_path, Path(parent_dir) / source_path.name)

    def remove_obsolete_includes(self) -> None:
        """
        Remove public include wrappers that are no longer generated.
        """
        if not self.include_dir.is_dir():
            return
        existing_paths = {path for path in self.include_dir.rglob("*") if path.is_file()}
        for path in sorted(existing_paths - self.generated_include_paths, key=lambda path: path.as_posix().casefold()):
            self.print_verbose(f"  removing obsolete include wrapper: {path.relative_to(self.project_dir)}")
            path.unlink()
        self.remove_empty_include_dirs()

    def remove_empty_include_dirs(self) -> None:
        """Remove empty directories left behind after stale wrappers were deleted."""
        if not self.include_dir.is_dir():
            return
        directories = [path for path in self.include_dir.rglob("*") if path.is_dir()]
        directories.sort(key=lambda path: len(path.parts), reverse=True)
        for path in directories:
            try:
                path.rmdir()
            except OSError:
                pass

    def read_config(self):
        self.print_verbose("Reading the configuration")
        config = read_elcl_file(self.config_file_path())
        main_config = config["main"]
        header_config = HeaderConfig.read(self.config_file_path())
        self.generated_header = header_config.source_header("hpp", tool="update_includes.py", pragma_once=True)
        self.generated_include_header = header_config.source_header("include", tool="update_includes.py")
        self.exclude_dirs = main_config.get_list("excluded_directories", str, default=[])
        self.create_global_includes = main_config.get_bool("create_global_includes", default=False)
        self.fold_into_parent = set(main_config.get_list("fold_into_parent", str, default=[]))
        self.create_all_base_dir = main_config.get_text("create_all_base_dir", default="")
        self.validate_config()

    def validate_config(self) -> None:
        """Validate the update-includes configuration."""
        validate_local_names(self.exclude_dirs, "Excluded Directories")
        validate_local_names(self.fold_into_parent, "Fold into Parent")
        validate_source_relative_path(self.create_all_base_dir, "Create All Base Dir")

    def handle_command_line_args(self, args) -> None:
        """
        Handle the command line arguments.
        """
        self.project_dir = self.project_directory
        self.src_dir = self.project_dir / "src"
        require_directory(self.src_dir, "Source Directory")
        self.src_dir = self.src_dir / "erbsland"
        require_directory(self.src_dir, "Erbsland Source Directory")
        self.include_dir = self.project_dir / "include"
        require_directory(self.include_dir, "Include Directory")
        self.include_dir = self.include_dir / "erbsland"
        require_directory(self.include_dir, "Erbsland Include Directory")

    def run(self, argv=None):
        """
        Run this script.
        """
        super().run(argv)
        self.read_config()
        self.collect_header_files()
        self._create_effective_parent_include_map()
        self.generate_all_headers()
        self.generate_includes()
        self.remove_obsolete_includes()


def main():
    raise SystemExit(UpdateIncludesApp().main())


if __name__ == "__main__":
    main()
