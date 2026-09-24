// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

use std::env;
use std::fs::{self, File};
use std::io::{Read, Seek, Write};
use std::path::{Path, PathBuf};

use zip::write::SimpleFileOptions;
use zip::{CompressionMethod, DateTime, ZipArchive, ZipWriter};

const CORE_METHODS: [CompressionMethod; 5] = [
    CompressionMethod::Stored,
    CompressionMethod::Deflated,
    CompressionMethod::Bzip2,
    CompressionMethod::Lzma,
    CompressionMethod::Zstd,
];
const RUST_METHODS: [CompressionMethod; 4] = [
    CompressionMethod::Stored,
    CompressionMethod::Deflated,
    CompressionMethod::Bzip2,
    CompressionMethod::Zstd,
];
const FIXED_COMMENT: &str = "Erbsland ZIP interoperability – UTF-8";

fn main() {
    if let Err(error) = run() {
        eprintln!("ZIP interop counterpart failed: {error}");
        std::process::exit(1);
    }
}

fn run() -> Result<(), String> {
    let arguments = env::args_os().skip(1).collect::<Vec<_>>();
    match arguments.as_slice() {
        [command, workspace, case_count] if command == "verify-batch" => {
            let workspace = PathBuf::from(workspace);
            let case_count = case_count
                .to_str()
                .ok_or_else(|| "case count is not valid UTF-8".to_owned())?
                .parse::<usize>()
                .map_err(|error| format!("invalid case count: {error}"))?;
            verify_batch(&workspace, case_count)
        }
        [command, output, source] if command == "generate-vectors" => {
            generate_vectors(&PathBuf::from(output), &PathBuf::from(source))
        }
        _ => Err("usage: counterpart verify-batch <workspace> <case-count>\n       counterpart generate-vectors <output> <source>".to_owned()),
    }
}

fn verify_batch(workspace: &Path, case_count: usize) -> Result<(), String> {
    verify_core_archive(
        &workspace.join("core-classic.zip"),
        workspace,
        case_count,
        false,
    )?;
    verify_core_archive(
        &workspace.join("core-zip64.zip"),
        workspace,
        case_count,
        true,
    )?;
    write_seekable_archive(
        &workspace.join("rust-classic.zip"),
        workspace,
        case_count,
        false,
    )?;
    write_seekable_archive(
        &workspace.join("rust-zip64.zip"),
        workspace,
        case_count,
        true,
    )?;
    write_stream_archive(&workspace.join("rust-stream.zip"), workspace, case_count)?;
    Ok(())
}

fn verify_core_archive(
    archive_path: &Path,
    workspace: &Path,
    case_count: usize,
    expect_zip64: bool,
) -> Result<(), String> {
    let archive_bytes = fs::read(archive_path)
        .map_err(|error| format!("failed to read {}: {error}", archive_path.display()))?;
    if contains_zip64_end(&archive_bytes) != expect_zip64 {
        return Err(format!(
            "unexpected ZIP64 state in {}",
            archive_path.display()
        ));
    }
    let mut archive = ZipArchive::new(std::io::Cursor::new(archive_bytes))
        .map_err(|error| format!("failed to open {}: {error}", archive_path.display()))?;
    if archive.comment() != FIXED_COMMENT.as_bytes() {
        return Err(format!(
            "archive comment mismatch in {}",
            archive_path.display()
        ));
    }
    if archive.len() != case_count {
        return Err(format!(
            "entry count mismatch in {}: {} instead of {case_count}",
            archive_path.display(),
            archive.len()
        ));
    }
    for index in 0..case_count {
        let expected_name = case_name(index);
        let expected = read_case(workspace, index)?;
        let mut item = archive
            .by_index(index)
            .map_err(|error| format!("failed to read entry {index}: {error}"))?;
        if item.name() != expected_name {
            return Err(format!(
                "entry {index} name mismatch: {} instead of {expected_name}",
                item.name()
            ));
        }
        if item.compression() != CORE_METHODS[index % CORE_METHODS.len()] {
            return Err(format!("entry {index} compression method mismatch"));
        }
        let mut actual = Vec::new();
        item.read_to_end(&mut actual)
            .map_err(|error| format!("failed to extract entry {index}: {error}"))?;
        if actual != expected {
            return Err(format!("entry {index} payload mismatch"));
        }
    }
    Ok(())
}

fn write_seekable_archive(
    archive_path: &Path,
    workspace: &Path,
    case_count: usize,
    zip64: bool,
) -> Result<(), String> {
    let destination = File::create(archive_path)
        .map_err(|error| format!("failed to create {}: {error}", archive_path.display()))?;
    let mut writer = ZipWriter::new(destination);
    writer
        .set_comment(FIXED_COMMENT)
        .map_err(|error| format!("failed to set archive comment: {error}"))?;
    write_cases(&mut writer, workspace, case_count, zip64)?;
    writer
        .finish()
        .map_err(|error| format!("failed to finish {}: {error}", archive_path.display()))?;
    Ok(())
}

fn write_stream_archive(
    archive_path: &Path,
    workspace: &Path,
    case_count: usize,
) -> Result<(), String> {
    let destination = File::create(archive_path)
        .map_err(|error| format!("failed to create {}: {error}", archive_path.display()))?;
    let mut writer = ZipWriter::new_stream(destination);
    writer
        .set_comment(FIXED_COMMENT)
        .map_err(|error| format!("failed to set archive comment: {error}"))?;
    write_cases(&mut writer, workspace, case_count, false)?;
    writer
        .finish()
        .map_err(|error| format!("failed to finish {}: {error}", archive_path.display()))?;
    Ok(())
}

fn write_cases<W: Write + Seek>(
    writer: &mut ZipWriter<W>,
    workspace: &Path,
    case_count: usize,
    zip64: bool,
) -> Result<(), String> {
    for index in 0..case_count {
        let options = file_options(RUST_METHODS[index % RUST_METHODS.len()], zip64)?;
        writer
            .start_file(case_name(index), options)
            .map_err(|error| format!("failed to start entry {index}: {error}"))?;
        writer
            .write_all(&read_case(workspace, index)?)
            .map_err(|error| format!("failed to write entry {index}: {error}"))?;
    }
    Ok(())
}

fn generate_vectors(output: &Path, source: &Path) -> Result<(), String> {
    fs::create_dir_all(output)
        .map_err(|error| format!("failed to create {}: {error}", output.display()))?;
    let entries = vector_entries(source)?;
    for (name, zip64) in [("rust-classic.zip", false), ("rust-zip64.zip", true)] {
        let path = output.join(name);
        let destination = File::create(&path)
            .map_err(|error| format!("failed to create {}: {error}", path.display()))?;
        let mut writer = ZipWriter::new(destination);
        writer
            .set_comment(FIXED_COMMENT)
            .map_err(|error| format!("failed to set archive comment: {error}"))?;
        write_vector_entries(&mut writer, &entries, zip64)?;
        writer
            .finish()
            .map_err(|error| format!("failed to finish {}: {error}", path.display()))?;
    }
    let path = output.join("rust-stream.zip");
    let destination = File::create(&path)
        .map_err(|error| format!("failed to create {}: {error}", path.display()))?;
    let mut writer = ZipWriter::new_stream(destination);
    writer
        .set_comment(FIXED_COMMENT)
        .map_err(|error| format!("failed to set archive comment: {error}"))?;
    write_vector_entries(&mut writer, &entries, false)?;
    writer
        .finish()
        .map_err(|error| format!("failed to finish {}: {error}", path.display()))?;
    Ok(())
}

fn write_vector_entries<W: Write + Seek>(
    writer: &mut ZipWriter<W>,
    entries: &[(String, Option<Vec<u8>>)],
    zip64: bool,
) -> Result<(), String> {
    let mut file_index = 0usize;
    for (name, data) in entries {
        if let Some(data) = data {
            let method = RUST_METHODS[file_index % RUST_METHODS.len()];
            writer
                .start_file(name, file_options(method, zip64)?)
                .map_err(|error| format!("failed to start {name}: {error}"))?;
            writer
                .write_all(data)
                .map_err(|error| format!("failed to write {name}: {error}"))?;
            file_index += 1;
        } else {
            writer
                .add_directory(name, file_options(CompressionMethod::Stored, zip64)?)
                .map_err(|error| format!("failed to add directory {name}: {error}"))?;
        }
    }
    Ok(())
}

fn vector_entries(source: &Path) -> Result<Vec<(String, Option<Vec<u8>>)>, String> {
    let names = [
        "empty-dir/",
        "empty.txt",
        "ascii.txt",
        "all-bytes.bin",
        "nested/repetitive.bin",
        "utf8-ä/日本語.txt",
    ];
    let mut result = Vec::new();
    for name in names {
        if name.ends_with('/') {
            result.push((name.to_owned(), None));
        } else {
            let path = source.join(name);
            let data = fs::read(&path)
                .map_err(|error| format!("failed to read {}: {error}", path.display()))?;
            result.push((name.to_owned(), Some(data)));
        }
    }
    Ok(result)
}

fn file_options(method: CompressionMethod, zip64: bool) -> Result<SimpleFileOptions, String> {
    let time = DateTime::from_date_and_time(2024, 2, 3, 4, 5, 6)
        .map_err(|error| format!("invalid fixed ZIP time: {error}"))?;
    Ok(SimpleFileOptions::default()
        .compression_method(method)
        .last_modified_time(time)
        .large_file(zip64)
        .unix_permissions(0o644))
}

fn case_name(index: usize) -> String {
    if index % 7 == 0 {
        format!("cases/ümlaut-{index:03}.bin")
    } else {
        format!("cases/case-{index:03}.bin")
    }
}

fn read_case(workspace: &Path, index: usize) -> Result<Vec<u8>, String> {
    let path = workspace.join(format!("case-{index:03}.bin"));
    fs::read(&path).map_err(|error| format!("failed to read {}: {error}", path.display()))
}

fn contains_zip64_end(data: &[u8]) -> bool {
    data.windows(4)
        .any(|window| window == [0x50, 0x4b, 0x06, 0x06])
}
