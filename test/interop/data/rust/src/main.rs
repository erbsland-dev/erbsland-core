// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

use bson::{doc, Bson, Document};
use ciborium::value::Value;
use std::env;
use std::fs;
use std::io::Cursor;
use std::path::Path;

fn main() {
    if let Err(error) = run() {
        eprintln!("data interop counterpart failed: {error}");
        std::process::exit(1);
    }
}

fn run() -> Result<(), String> {
    let args = env::args().collect::<Vec<_>>();
    if args.len() != 3 {
        return Err("usage: counterpart <emit|verify> <directory>".into());
    }
    let directory = Path::new(&args[2]);
    match args[1].as_str() {
        "emit" => emit(directory),
        "verify" => verify(directory),
        _ => Err("unknown command".into()),
    }
}

fn emit(directory: &Path) -> Result<(), String> {
    fs::create_dir_all(directory).map_err(|error| error.to_string())?;
    let document = doc! {
        "ritim": "aksak",
        "bpm": 96_i32,
        "etkin": true,
        "vurgu": Bson::Binary(bson::Binary { subtype: bson::spec::BinarySubtype::Generic, bytes: vec![0, 2, 4] }),
    };
    let bson = bson::to_vec(&document).map_err(|error| error.to_string())?;
    fs::write(directory.join("rust-bson.bin"), bson).map_err(|error| error.to_string())?;

    let cbor = Value::Map(vec![
        (Value::Text("bpm".into()), Value::Integer(96.into())),
        (Value::Text("ritim".into()), Value::Text("aksak".into())),
        (Value::Text("vurgu".into()), Value::Bytes(vec![0, 2, 4])),
    ]);
    let mut encoded = Vec::new();
    ciborium::ser::into_writer(&cbor, &mut encoded).map_err(|error| error.to_string())?;
    fs::write(directory.join("rust-cbor.bin"), encoded).map_err(|error| error.to_string())?;

    fs::write(directory.join("rust-xml.xml"),
        "<?xml version=\"1.0\"?><ritimler><ritim bpm=\"96\">aksak<vurgu>2</vurgu></ritim></ritimler>")
        .map_err(|error| error.to_string())?;
    Ok(())
}

fn verify(directory: &Path) -> Result<(), String> {
    let bson_bytes = fs::read(directory.join("core-bson.bin")).map_err(|error| error.to_string())?;
    let bson = Document::from_reader(&mut Cursor::new(bson_bytes)).map_err(|error| error.to_string())?;
    if bson.get_str("ritim").map_err(|error| error.to_string())? != "aksak" ||
        bson.get_i32("bpm").map_err(|error| error.to_string())? != 96 ||
        !bson.get_bool("etkin").map_err(|error| error.to_string())? {
        return Err("BSON field mismatch".into());
    }
    let binary = bson.get_binary_generic("vurgu").map_err(|error| error.to_string())?;
    if binary.as_slice() != [0, 2, 4] { return Err("BSON binary mismatch".into()); }

    let cbor_bytes = fs::read(directory.join("core-cbor.bin")).map_err(|error| error.to_string())?;
    let cbor: Value = ciborium::de::from_reader(Cursor::new(cbor_bytes)).map_err(|error| error.to_string())?;
    let Value::Map(entries) = cbor else { return Err("CBOR root is not a map".into()); };
    let mut found = 0;
    for (key, value) in entries {
        match (key, value) {
            (Value::Text(key), Value::Text(value)) if key == "ritim" && value == "aksak" => found += 1,
            (Value::Text(key), Value::Integer(value)) if key == "bpm" && value == 96.into() => found += 1,
            (Value::Text(key), Value::Bytes(value)) if key == "vurgu" && value == [0, 2, 4] => found += 1,
            _ => return Err("CBOR field mismatch".into()),
        }
    }
    if found != 3 { return Err("CBOR field count mismatch".into()); }

    let xml = fs::read_to_string(directory.join("core-xml.xml")).map_err(|error| error.to_string())?;
    let document = roxmltree::Document::parse(&xml).map_err(|error| error.to_string())?;
    let root = document.root_element();
    if root.tag_name().name() != "ritimler" { return Err("XML root mismatch".into()); }
    let ritim = root.children().find(|node| node.is_element()).ok_or("XML child missing")?;
    if ritim.tag_name().name() != "ritim" || ritim.attribute("bpm") != Some("96") {
        return Err("XML element or attribute mismatch".into());
    }
    let first_text = ritim.children().find_map(|node| node.text()).unwrap_or("");
    if first_text != "aksak" { return Err("XML text mismatch".into()); }
    Ok(())
}
