// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

use rustls::pki_types::{CertificateDer, PrivateKeyDer, ServerName};
use rustls::{
    version, ClientConfig, ClientConnection, RootCertStore, ServerConfig, ServerConnection,
    StreamOwned, SupportedCipherSuite,
};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs::File;
use std::io::{self, BufRead, BufReader, Read, Write};
use std::net::{Shutdown, TcpListener, TcpStream};
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};
use std::thread::{self, JoinHandle};
use std::time::Duration;

const PROTOCOL_VERSION: u32 = 1;
const MAXIMUM_CONTROL_LINE_LENGTH: usize = 64 * 1024;
const IO_TIMEOUT: Duration = Duration::from_secs(10);

#[derive(Debug)]
struct Arguments {
    control_port: u16,
    token: String,
    certificate: PathBuf,
    private_key: PathBuf,
    trust_certificates: Vec<PathBuf>,
}

#[derive(Debug, Deserialize)]
struct Request {
    protocol: u32,
    id: u64,
    command: String,
    #[serde(default)]
    scenario: String,
    #[serde(default)]
    scenario_id: u64,
    #[serde(default)]
    cipher: String,
    #[serde(default)]
    payload_length: usize,
    #[serde(default)]
    port: u16,
    #[serde(default)]
    server_name: String,
    #[serde(default)]
    wire_hex: String,
}

#[derive(Debug, Serialize)]
struct Response<'a> {
    protocol: u32,
    id: u64,
    status: &'a str,
    #[serde(skip_serializing_if = "Option::is_none")]
    scenario_id: Option<u64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    port: Option<u16>,
    #[serde(skip_serializing_if = "Option::is_none")]
    bytes_received: Option<usize>,
    #[serde(skip_serializing_if = "Option::is_none")]
    error_code: Option<&'a str>,
    #[serde(skip_serializing_if = "Option::is_none")]
    error_message: Option<String>,
}

struct Scenario {
    port: u16,
    active_stream: Arc<Mutex<Option<TcpStream>>>,
    thread: JoinHandle<Result<usize, String>>,
}

impl Scenario {
    fn cancel(self) {
        let mut interrupted = false;
        if let Ok(active_stream) = self.active_stream.lock() {
            if let Some(stream) = active_stream.as_ref() {
                let _ = stream.shutdown(Shutdown::Both);
                interrupted = true;
            }
        }
        if !interrupted {
            let _ = TcpStream::connect(("127.0.0.1", self.port));
        }
        let _ = self.thread.join();
    }
}

fn main() {
    if let Err(error) = run() {
        eprintln!("network interop counterpart failed: {error}");
        std::process::exit(1);
    }
}

fn run() -> Result<(), String> {
    rustls::crypto::ring::default_provider()
        .install_default()
        .map_err(|_| "failed to install the rustls ring provider".to_owned())?;
    let arguments = parse_arguments()?;
    let mut control = TcpStream::connect(("127.0.0.1", arguments.control_port))
        .map_err(|error| format!("failed to connect control channel: {error}"))?;
    control
        .set_nodelay(true)
        .map_err(|error| format!("failed to configure control channel: {error}"))?;
    let mut reader = BufReader::new(
        control
            .try_clone()
            .map_err(|error| format!("failed to clone control channel: {error}"))?,
    );
    write_json(
        &mut control,
        &serde_json::json!({
            "protocol": PROTOCOL_VERSION,
            "type": "hello",
            "token": arguments.token,
            "implementation": "rustls",
            "implementation_version": "0.23.43"
        }),
    )?;

    let certificates = Arc::new(load_certificates(&arguments.certificate)?);
    let mut trust_certificates = Vec::<CertificateDer<'static>>::new();
    for path in &arguments.trust_certificates {
        trust_certificates.extend(load_certificates(path)?);
    }
    let trust_certificates = Arc::new(trust_certificates);
    let private_key_path = Arc::new(arguments.private_key);
    let mut scenarios = HashMap::<u64, Scenario>::new();
    loop {
        let Some(line) = read_line(&mut reader)? else {
            break;
        };
        let request: Request = serde_json::from_str(&line)
            .map_err(|error| format!("invalid control request: {error}"))?;
        if request.protocol != PROTOCOL_VERSION {
            write_error(
                &mut control,
                request.id,
                "protocol",
                "unsupported protocol version",
            )?;
            continue;
        }
        match request.command.as_str() {
            "http-request" | "http-response" | "http-chunk-size" => {
                let result = compare_http(&request.command, &request.wire_hex);
                match result {
                    Ok((accepted, parsed)) => write_json(
                        &mut control,
                        &serde_json::json!({
                            "protocol": PROTOCOL_VERSION,
                            "id": request.id,
                            "status": "ok",
                            "accepted": if accepted { 1 } else { 0 },
                            "parsed": parsed
                        }),
                    )?,
                    Err(error) => write_error(&mut control, request.id, "http", &error)?,
                }
            }
            "start" => {
                if scenarios.contains_key(&request.id) {
                    write_error(
                        &mut control,
                        request.id,
                        "duplicate",
                        "duplicate scenario id",
                    )?;
                    continue;
                }
                match start_scenario(
                    &request,
                    Arc::clone(&certificates),
                    Arc::clone(&private_key_path),
                ) {
                    Ok((port, scenario)) => {
                        scenarios.insert(request.id, scenario);
                        write_json(
                            &mut control,
                            &Response {
                                protocol: PROTOCOL_VERSION,
                                id: request.id,
                                status: "ok",
                                scenario_id: Some(request.id),
                                port: Some(port),
                                bytes_received: None,
                                error_code: None,
                                error_message: None,
                            },
                        )?;
                    }
                    Err(error) => write_error(&mut control, request.id, "start", &error)?,
                }
            }
            "connect" => {
                if scenarios.contains_key(&request.id) {
                    write_error(
                        &mut control,
                        request.id,
                        "duplicate",
                        "duplicate scenario id",
                    )?;
                    continue;
                }
                match start_client_scenario(&request, Arc::clone(&trust_certificates)) {
                    Ok(scenario) => {
                        scenarios.insert(request.id, scenario);
                        write_json(
                            &mut control,
                            &Response {
                                protocol: PROTOCOL_VERSION,
                                id: request.id,
                                status: "ok",
                                scenario_id: Some(request.id),
                                port: Some(request.port),
                                bytes_received: None,
                                error_code: None,
                                error_message: None,
                            },
                        )?;
                    }
                    Err(error) => write_error(&mut control, request.id, "connect", &error)?,
                }
            }
            "finish" => {
                let Some(scenario) = scenarios.remove(&request.scenario_id) else {
                    write_error(&mut control, request.id, "unknown", "unknown scenario id")?;
                    continue;
                };
                let result = scenario
                    .thread
                    .join()
                    .map_err(|_| "scenario thread panicked".to_owned())?;
                match result {
                    Ok(bytes_received) => write_json(
                        &mut control,
                        &Response {
                            protocol: PROTOCOL_VERSION,
                            id: request.id,
                            status: "ok",
                            scenario_id: Some(request.scenario_id),
                            port: None,
                            bytes_received: Some(bytes_received),
                            error_code: None,
                            error_message: None,
                        },
                    )?,
                    Err(error) => write_error(&mut control, request.id, "scenario", &error)?,
                }
            }
            "cancel" => {
                if let Some(scenario) = scenarios.remove(&request.scenario_id) {
                    scenario.cancel();
                }
                write_ok(&mut control, request.id)?;
            }
            "shutdown" => {
                for (_, scenario) in scenarios.drain() {
                    scenario.cancel();
                }
                write_ok(&mut control, request.id)?;
                break;
            }
            _ => write_error(&mut control, request.id, "command", "unknown command")?,
        }
    }
    for (_, scenario) in scenarios.drain() {
        scenario.cancel();
    }
    Ok(())
}

fn compare_http(command: &str, wire_hex: &str) -> Result<(bool, String), String> {
    let wire = decode_hex(wire_hex)?;
    match command {
        "http-request" => {
            let mut headers = [httparse::EMPTY_HEADER; 100];
            let mut request = httparse::Request::new(&mut headers);
            match request.parse(&wire) {
                Ok(httparse::Status::Complete(consumed)) => {
                    let mut parsed = format!(
                        "{}:{}:{}:{}",
                        consumed,
                        request.version.unwrap_or(255),
                        hex(request.method.unwrap_or("").as_bytes()),
                        hex(request.path.unwrap_or("").as_bytes())
                    );
                    for header in request.headers {
                        parsed.push(':');
                        parsed.push_str(&hex(header.name.as_bytes()));
                        parsed.push('=');
                        parsed.push_str(&hex(header.value));
                    }
                    Ok((true, parsed))
                }
                Ok(httparse::Status::Partial) | Err(_) => Ok((false, String::new())),
            }
        }
        "http-response" => {
            let mut headers = [httparse::EMPTY_HEADER; 100];
            let mut response = httparse::Response::new(&mut headers);
            match response.parse(&wire) {
                Ok(httparse::Status::Complete(consumed)) => {
                    let mut parsed = format!(
                        "{}:{}:{}:{}",
                        consumed,
                        response.version.unwrap_or(255),
                        response.code.unwrap_or(0),
                        hex(response.reason.unwrap_or("").as_bytes())
                    );
                    for header in response.headers {
                        parsed.push(':');
                        parsed.push_str(&hex(header.name.as_bytes()));
                        parsed.push('=');
                        parsed.push_str(&hex(header.value));
                    }
                    Ok((true, parsed))
                }
                Ok(httparse::Status::Partial) | Err(_) => Ok((false, String::new())),
            }
        }
        "http-chunk-size" => match httparse::parse_chunk_size(&wire) {
            Ok(httparse::Status::Complete((consumed, size))) => {
                Ok((true, format!("{consumed}:{size}")))
            }
            Ok(httparse::Status::Partial) | Err(_) => Ok((false, String::new())),
        },
        _ => Err("unknown HTTP comparison kind".to_owned()),
    }
}

fn decode_hex(text: &str) -> Result<Vec<u8>, String> {
    if text.len() % 2 != 0 || text.len() > 128 * 1024 {
        return Err("invalid bounded hexadecimal HTTP input".to_owned());
    }
    let mut result = Vec::with_capacity(text.len() / 2);
    let bytes = text.as_bytes();
    for index in (0..bytes.len()).step_by(2) {
        let high = hex_digit(bytes[index]).ok_or_else(|| "invalid hexadecimal input".to_owned())?;
        let low =
            hex_digit(bytes[index + 1]).ok_or_else(|| "invalid hexadecimal input".to_owned())?;
        result.push((high << 4) | low);
    }
    Ok(result)
}

fn hex_digit(value: u8) -> Option<u8> {
    match value {
        b'0'..=b'9' => Some(value - b'0'),
        b'a'..=b'f' => Some(value - b'a' + 10),
        b'A'..=b'F' => Some(value - b'A' + 10),
        _ => None,
    }
}

fn hex(bytes: &[u8]) -> String {
    const DIGITS: &[u8; 16] = b"0123456789abcdef";
    let mut result = String::with_capacity(bytes.len() * 2);
    for value in bytes {
        result.push(DIGITS[(value >> 4) as usize] as char);
        result.push(DIGITS[(value & 0x0f) as usize] as char);
    }
    result
}

fn start_client_scenario(
    request: &Request,
    ca_certificates: Arc<Vec<CertificateDer<'static>>>,
) -> Result<Scenario, String> {
    if request.port == 0 {
        return Err("client scenario requires a nonzero server port".to_owned());
    }
    let port = request.port;
    let scenario = request.scenario.clone();
    let cipher = request.cipher.clone();
    let payload_length = request.payload_length;
    let server_name = if request.server_name.is_empty() {
        "localhost".to_owned()
    } else {
        request.server_name.clone()
    };
    let active_stream = Arc::new(Mutex::new(None::<TcpStream>));
    let thread_active_stream = Arc::clone(&active_stream);
    let thread = thread::spawn(move || {
        let stream = TcpStream::connect(("127.0.0.1", port))
            .map_err(|error| format!("failed to connect TLS server: {error}"))?;
        configure_stream(&stream)?;
        *thread_active_stream
            .lock()
            .map_err(|_| "scenario stream lock was poisoned".to_owned())? = Some(
            stream
                .try_clone()
                .map_err(|error| format!("failed to clone scenario stream: {error}"))?,
        );
        match scenario.as_str() {
            "tls_client_echo" => run_tls_client_echo(
                stream,
                build_client_config(&ca_certificates, &cipher)?,
                &server_name,
                payload_length,
                false,
            ),
            "tls_client_truncate" => run_tls_client_echo(
                stream,
                build_client_config(&ca_certificates, &cipher)?,
                &server_name,
                payload_length,
                true,
            ),
            _ => Err(format!("unknown client scenario: {scenario}")),
        }
    });
    Ok(Scenario {
        port,
        active_stream,
        thread,
    })
}

fn start_scenario(
    request: &Request,
    certificates: Arc<Vec<CertificateDer<'static>>>,
    private_key_path: Arc<PathBuf>,
) -> Result<(u16, Scenario), String> {
    let listener = TcpListener::bind(("127.0.0.1", 0))
        .map_err(|error| format!("failed to bind scenario listener: {error}"))?;
    let port = listener
        .local_addr()
        .map_err(|error| format!("failed to query scenario port: {error}"))?
        .port();
    let scenario = request.scenario.clone();
    let cipher = request.cipher.clone();
    let payload_length = request.payload_length;
    let active_stream = Arc::new(Mutex::new(None::<TcpStream>));
    let thread_active_stream = Arc::clone(&active_stream);
    let thread = thread::spawn(move || {
        let (stream, _) = listener
            .accept()
            .map_err(|error| format!("failed to accept TLS client: {error}"))?;
        configure_stream(&stream)?;
        *thread_active_stream
            .lock()
            .map_err(|_| "scenario stream lock was poisoned".to_owned())? = Some(
            stream
                .try_clone()
                .map_err(|error| format!("failed to clone scenario stream: {error}"))?,
        );
        match scenario.as_str() {
            "http_plain_server" => run_http_exchange(stream),
            "http_tls_server" => run_tls_http_server(
                stream,
                build_http_server_config(&certificates, &private_key_path)?,
            ),
            "tls_echo" => run_tls_echo(
                stream,
                build_server_config(&certificates, &private_key_path, &cipher, false)?,
                payload_length,
                false,
            ),
            "truncate_after_data" => run_tls_echo(
                stream,
                build_server_config(&certificates, &private_key_path, &cipher, false)?,
                payload_length,
                true,
            ),
            "tls12_only" => run_tls12_only(
                stream,
                build_server_config(&certificates, &private_key_path, "", true)?,
            ),
            "disconnect_during_handshake" => {
                let mut stream = stream;
                let mut header = [0_u8; 5];
                stream
                    .read_exact(&mut header)
                    .map_err(io_error("read interrupted handshake header"))?;
                let length = u16::from_be_bytes([header[3], header[4]]) as usize;
                let mut record = vec![0_u8; length];
                stream
                    .read_exact(&mut record)
                    .map_err(io_error("read interrupted handshake record"))?;
                stream.shutdown(Shutdown::Write).ok();
                Ok(0)
            }
            "unsupported_server_cipher" => run_unsupported_cipher(stream),
            _ => Err(format!("unknown scenario: {scenario}")),
        }
    });
    Ok((
        port,
        Scenario {
            port,
            active_stream,
            thread,
        },
    ))
}

fn build_server_config(
    certificates: &[CertificateDer<'static>],
    private_key_path: &Path,
    cipher_name: &str,
    tls12_only: bool,
) -> Result<Arc<ServerConfig>, String> {
    let mut provider = rustls::crypto::ring::default_provider();
    if !cipher_name.is_empty() {
        provider
            .cipher_suites
            .retain(|suite| cipher_suite_name(*suite) == cipher_name);
        if provider.cipher_suites.is_empty() {
            return Err(format!(
                "rustls does not support requested cipher: {cipher_name}"
            ));
        }
    }
    let tls12_versions = [&version::TLS12];
    let tls13_versions = [&version::TLS13];
    let versions: &[&'static rustls::SupportedProtocolVersion] = if tls12_only {
        &tls12_versions
    } else {
        &tls13_versions
    };
    let key = load_private_key(private_key_path)?;
    let mut config = ServerConfig::builder_with_provider(Arc::new(provider))
        .with_protocol_versions(versions)
        .map_err(|error| format!("failed to select TLS versions: {error}"))?
        .with_no_client_auth()
        .with_single_cert(certificates.to_vec(), key)
        .map_err(|error| format!("failed to configure certificate: {error}"))?;
    config.alpn_protocols = vec![b"erbsland-test".to_vec()];
    Ok(Arc::new(config))
}

fn build_http_server_config(
    certificates: &[CertificateDer<'static>],
    private_key_path: &Path,
) -> Result<Arc<ServerConfig>, String> {
    let key = load_private_key(private_key_path)?;
    let mut config =
        ServerConfig::builder_with_provider(Arc::new(rustls::crypto::ring::default_provider()))
            .with_protocol_versions(&[&version::TLS13])
            .map_err(|error| format!("failed to select TLS 1.3: {error}"))?
            .with_no_client_auth()
            .with_single_cert(certificates.to_vec(), key)
            .map_err(|error| format!("failed to configure HTTP certificate: {error}"))?;
    config.alpn_protocols = vec![b"http/1.1".to_vec()];
    Ok(Arc::new(config))
}

fn build_client_config(
    ca_certificates: &[CertificateDer<'static>],
    cipher_name: &str,
) -> Result<Arc<ClientConfig>, String> {
    let mut provider = rustls::crypto::ring::default_provider();
    if !cipher_name.is_empty() {
        provider
            .cipher_suites
            .retain(|suite| cipher_suite_name(*suite) == cipher_name);
        if provider.cipher_suites.is_empty() {
            return Err(format!(
                "rustls does not support requested cipher: {cipher_name}"
            ));
        }
    }
    let mut roots = RootCertStore::empty();
    for certificate in ca_certificates {
        roots
            .add(certificate.clone())
            .map_err(|error| format!("failed to add root certificate: {error}"))?;
    }
    let mut config = ClientConfig::builder_with_provider(Arc::new(provider))
        .with_protocol_versions(&[&version::TLS13])
        .map_err(|error| format!("failed to select TLS 1.3: {error}"))?
        .with_root_certificates(roots)
        .with_no_client_auth();
    config.alpn_protocols = vec![b"erbsland-test".to_vec()];
    Ok(Arc::new(config))
}

fn cipher_suite_name(suite: SupportedCipherSuite) -> &'static str {
    match suite.suite() {
        rustls::CipherSuite::TLS13_AES_128_GCM_SHA256 => "TLS_AES_128_GCM_SHA256",
        rustls::CipherSuite::TLS13_AES_256_GCM_SHA384 => "TLS_AES_256_GCM_SHA384",
        rustls::CipherSuite::TLS13_CHACHA20_POLY1305_SHA256 => "TLS_CHACHA20_POLY1305_SHA256",
        _ => "",
    }
}

fn run_tls_echo(
    stream: TcpStream,
    config: Arc<ServerConfig>,
    expected_length: usize,
    truncate: bool,
) -> Result<usize, String> {
    let connection = ServerConnection::new(config)
        .map_err(|error| format!("failed to create TLS server: {error}"))?;
    let mut stream = StreamOwned::new(connection, stream);
    let mut total = 0_usize;
    let mut buffer = [0_u8; 16 * 1024];
    while total < expected_length {
        let read_length = buffer.len().min(expected_length - total);
        let count = stream
            .read(&mut buffer[..read_length])
            .map_err(|error| format!("failed to read TLS payload: {error}"))?;
        if count == 0 {
            return Err(format!(
                "TLS client closed after {total} of {expected_length} bytes"
            ));
        }
        total += count;
        stream
            .write_all(&buffer[..count])
            .map_err(|error| format!("failed to echo TLS payload: {error}"))?;
        stream
            .flush()
            .map_err(|error| format!("failed to flush TLS payload: {error}"))?;
    }
    if truncate {
        stream.sock.shutdown(Shutdown::Both).ok();
        return Ok(total);
    }
    stream.conn.send_close_notify();
    stream
        .flush()
        .map_err(|error| format!("failed to send close notification: {error}"))?;
    loop {
        match stream.read(&mut buffer) {
            Ok(0) => break,
            Ok(_) => {}
            Err(error) => {
                return Err(format!(
                    "failed while waiting for peer close notification: {error}"
                ));
            }
        }
    }
    Ok(total)
}

fn run_tls_http_server(stream: TcpStream, config: Arc<ServerConfig>) -> Result<usize, String> {
    let connection = ServerConnection::new(config)
        .map_err(|error| format!("failed to create TLS HTTP server: {error}"))?;
    let mut stream = StreamOwned::new(connection, stream);
    let result = run_http_exchange(&mut stream)?;
    stream.conn.send_close_notify();
    stream
        .flush()
        .map_err(|error| format!("failed to flush HTTP close notification: {error}"))?;
    Ok(result)
}

fn run_http_exchange<T: Read + Write>(stream: T) -> Result<usize, String> {
    let mut stream = BufReader::new(stream);
    let mut request_wire = Vec::<u8>::new();
    let mut header_length = None::<usize>;
    while header_length.is_none() {
        if request_wire.len() >= MAXIMUM_CONTROL_LINE_LENGTH {
            return Err("HTTP request head exceeds 64 KiB".to_owned());
        }
        let mut byte = [0_u8; 1];
        stream
            .read_exact(&mut byte)
            .map_err(io_error("read HTTP request head"))?;
        request_wire.push(byte[0]);
        if request_wire.ends_with(b"\r\n\r\n") {
            header_length = Some(request_wire.len());
        }
    }
    let mut headers = [httparse::EMPTY_HEADER; 100];
    let mut request = httparse::Request::new(&mut headers);
    let parsed_length = match request
        .parse(&request_wire)
        .map_err(|error| format!("failed to parse HTTP request: {error}"))?
    {
        httparse::Status::Complete(length) => length,
        httparse::Status::Partial => return Err("HTTP request head remained partial".to_owned()),
    };
    if parsed_length != header_length.unwrap_or_default()
        || request.method != Some("POST")
        || request.path != Some("/interop?x=1")
        || request.version != Some(1)
    {
        return Err("HTTP request line does not match the deterministic scenario".to_owned());
    }
    let mut content_length = None::<usize>;
    let mut host_seen = false;
    for header in request.headers.iter() {
        if header.name.eq_ignore_ascii_case("host") {
            host_seen = !header.value.is_empty();
        } else if header.name.eq_ignore_ascii_case("content-length") {
            let text = std::str::from_utf8(header.value)
                .map_err(|_| "HTTP Content-Length is not ASCII".to_owned())?;
            content_length = Some(
                text.parse::<usize>()
                    .map_err(|_| "HTTP Content-Length is not numeric".to_owned())?,
            );
        } else if header.name.eq_ignore_ascii_case("transfer-encoding") {
            return Err("fixed interop request unexpectedly used Transfer-Encoding".to_owned());
        }
    }
    if !host_seen {
        return Err("HTTP interop request has no Host field".to_owned());
    }
    let body_length =
        content_length.ok_or_else(|| "HTTP interop request has no Content-Length".to_owned())?;
    let mut body = vec![0_u8; body_length];
    stream
        .read_exact(&mut body)
        .map_err(io_error("read HTTP request body"))?;
    if body != b"payload" {
        return Err("HTTP interop request body mismatch".to_owned());
    }
    stream
        .get_mut()
        .write_all(
            b"HTTP/1.1 103 Early Hints\r\nLink: </style.css>; rel=preload\r\n\r\n\
HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\n\
Transfer-Encoding: chunked\r\nTrailer: X-Interop\r\nConnection: close\r\n\r\n\
6\r\n{\"ok\":\r\n5\r\ntrue}\r\n0\r\nX-Interop: rust\r\n\r\n",
        )
        .map_err(io_error("write HTTP response"))?;
    stream
        .get_mut()
        .flush()
        .map_err(io_error("flush HTTP response"))?;
    Ok(body_length)
}

fn run_tls_client_echo(
    stream: TcpStream,
    config: Arc<ClientConfig>,
    server_name: &str,
    payload_length: usize,
    truncate: bool,
) -> Result<usize, String> {
    let name = ServerName::try_from(server_name.to_owned())
        .map_err(|error| format!("invalid TLS server name: {error}"))?;
    let connection = ClientConnection::new(config, name)
        .map_err(|error| format!("failed to create TLS client: {error}"))?;
    let mut stream = StreamOwned::new(connection, stream);
    let mut total = 0_usize;
    let mut buffer = [0_u8; 16 * 1024];
    while total < payload_length {
        let length = buffer.len().min(payload_length - total);
        buffer[..length].fill(0x5a);
        stream
            .write_all(&buffer[..length])
            .map_err(|error| format!("failed to write TLS payload: {error}"))?;
        stream
            .flush()
            .map_err(|error| format!("failed to flush TLS payload: {error}"))?;
        let mut received = 0_usize;
        while received < length {
            let count = stream
                .read(&mut buffer[received..length])
                .map_err(|error| format!("failed to read TLS echo: {error}"))?;
            if count == 0 {
                return Err(format!("TLS server closed after {total} bytes"));
            }
            received += count;
        }
        if buffer[..length].iter().any(|value| *value != 0x5a) {
            return Err("TLS echo payload mismatch".to_owned());
        }
        total += length;
        if total >= payload_length / 2 && total - length < payload_length / 2 {
            stream
                .conn
                .refresh_traffic_keys()
                .map_err(|error| format!("failed to request KeyUpdate: {error}"))?;
        }
    }
    if truncate {
        stream.sock.shutdown(Shutdown::Both).ok();
        return Ok(total);
    }
    stream.conn.send_close_notify();
    stream
        .flush()
        .map_err(|error| format!("failed to send close notification: {error}"))?;
    loop {
        match stream.read(&mut buffer) {
            Ok(0) => break,
            Ok(_) => {}
            Err(error) => {
                return Err(format!(
                    "failed while waiting for server close notification: {error}"
                ));
            }
        }
    }
    Ok(total)
}

fn run_tls12_only(stream: TcpStream, config: Arc<ServerConfig>) -> Result<usize, String> {
    let mut connection = ServerConnection::new(config)
        .map_err(|error| format!("failed to create TLS 1.2 server: {error}"))?;
    match connection.complete_io(&mut &stream) {
        Ok(_) => Err("TLS 1.3 client unexpectedly negotiated TLS 1.2".to_owned()),
        Err(_) => Ok(0),
    }
}

fn run_unsupported_cipher(mut stream: TcpStream) -> Result<usize, String> {
    let mut record_header = [0_u8; 5];
    stream
        .read_exact(&mut record_header)
        .map_err(io_error("read ClientHello header"))?;
    if record_header[0] != 22 {
        return Err("first client record is not a handshake".to_owned());
    }
    let length = u16::from_be_bytes([record_header[3], record_header[4]]) as usize;
    let mut client_hello = vec![0_u8; length];
    stream
        .read_exact(&mut client_hello)
        .map_err(io_error("read ClientHello"))?;
    if client_hello.len() < 39 || client_hello[0] != 1 {
        return Err("malformed ClientHello".to_owned());
    }
    let session_id_length = client_hello[38] as usize;
    if client_hello.len() < 39 + session_id_length {
        return Err("truncated ClientHello session id".to_owned());
    }
    let session_id = &client_hello[39..39 + session_id_length];
    let mut body = Vec::<u8>::new();
    body.extend_from_slice(&[0x03, 0x03]);
    body.extend_from_slice(&[0x5a; 32]);
    body.push(session_id_length as u8);
    body.extend_from_slice(session_id);
    body.extend_from_slice(&[0x13, 0x04, 0x00]);
    body.extend_from_slice(&[0x00, 0x06, 0x00, 0x2b, 0x00, 0x02, 0x03, 0x04]);
    let mut handshake = Vec::<u8>::new();
    handshake.push(2);
    let body_length = body.len() as u32;
    handshake.extend_from_slice(&body_length.to_be_bytes()[1..]);
    handshake.extend_from_slice(&body);
    let mut record = vec![22, 0x03, 0x03];
    record.extend_from_slice(&(handshake.len() as u16).to_be_bytes());
    record.extend_from_slice(&handshake);
    stream
        .write_all(&record)
        .map_err(io_error("write malformed ServerHello"))?;
    stream
        .flush()
        .map_err(io_error("flush malformed ServerHello"))?;
    let mut alert = [0_u8; 64];
    let _ = stream.read(&mut alert);
    Ok(0)
}

fn configure_stream(stream: &TcpStream) -> Result<(), String> {
    stream
        .set_nodelay(true)
        .map_err(|error| format!("failed to set TCP_NODELAY: {error}"))?;
    stream
        .set_read_timeout(Some(IO_TIMEOUT))
        .map_err(|error| format!("failed to set read timeout: {error}"))?;
    stream
        .set_write_timeout(Some(IO_TIMEOUT))
        .map_err(|error| format!("failed to set write timeout: {error}"))?;
    Ok(())
}

fn load_certificates(path: &Path) -> Result<Vec<CertificateDer<'static>>, String> {
    let file = File::open(path)
        .map_err(|error| format!("failed to open certificate {}: {error}", path.display()))?;
    rustls_pemfile::certs(&mut BufReader::new(file))
        .collect::<Result<Vec<_>, _>>()
        .map_err(|error| format!("failed to parse certificate {}: {error}", path.display()))
}

fn load_private_key(path: &Path) -> Result<PrivateKeyDer<'static>, String> {
    let file = File::open(path)
        .map_err(|error| format!("failed to open private key {}: {error}", path.display()))?;
    rustls_pemfile::private_key(&mut BufReader::new(file))
        .map_err(|error| format!("failed to parse private key {}: {error}", path.display()))?
        .ok_or_else(|| format!("no private key found in {}", path.display()))
}

fn parse_arguments() -> Result<Arguments, String> {
    let mut values = std::env::args().skip(1);
    let mut result = Arguments {
        control_port: 0,
        token: String::new(),
        certificate: PathBuf::new(),
        private_key: PathBuf::new(),
        trust_certificates: Vec::new(),
    };
    while let Some(name) = values.next() {
        let value = values
            .next()
            .ok_or_else(|| format!("missing value for {name}"))?;
        match name.as_str() {
            "--control-port" => {
                result.control_port = value
                    .parse()
                    .map_err(|_| "invalid control port".to_owned())?
            }
            "--token" => result.token = value,
            "--certificate" => result.certificate = value.into(),
            "--private-key" => result.private_key = value.into(),
            "--trust-certificate" => result.trust_certificates.push(value.into()),
            _ => return Err(format!("unknown argument: {name}")),
        }
    }
    if result.control_port == 0
        || result.token.is_empty()
        || result.certificate.as_os_str().is_empty()
        || result.private_key.as_os_str().is_empty()
        || result.trust_certificates.is_empty()
    {
        return Err("required counterpart arguments are missing".to_owned());
    }
    Ok(result)
}

fn read_line(reader: &mut BufReader<TcpStream>) -> Result<Option<String>, String> {
    let mut bytes = Vec::<u8>::new();
    let count = reader
        .read_until(b'\n', &mut bytes)
        .map_err(|error| format!("failed to read control channel: {error}"))?;
    if count == 0 {
        return Ok(None);
    }
    if bytes.len() > MAXIMUM_CONTROL_LINE_LENGTH {
        return Err("control line exceeds 64 KiB".to_owned());
    }
    if bytes.last() == Some(&b'\n') {
        bytes.pop();
    }
    if bytes.last() == Some(&b'\r') {
        bytes.pop();
    }
    String::from_utf8(bytes)
        .map(Some)
        .map_err(|_| "control line is not UTF-8".to_owned())
}

fn write_ok(stream: &mut TcpStream, id: u64) -> Result<(), String> {
    write_json(
        stream,
        &Response {
            protocol: PROTOCOL_VERSION,
            id,
            status: "ok",
            scenario_id: None,
            port: None,
            bytes_received: None,
            error_code: None,
            error_message: None,
        },
    )
}

fn write_error(
    stream: &mut TcpStream,
    id: u64,
    code: &'static str,
    message: &str,
) -> Result<(), String> {
    write_json(
        stream,
        &Response {
            protocol: PROTOCOL_VERSION,
            id,
            status: "error",
            scenario_id: None,
            port: None,
            bytes_received: None,
            error_code: Some(code),
            error_message: Some(message.to_owned()),
        },
    )
}

fn write_json(stream: &mut TcpStream, value: &impl Serialize) -> Result<(), String> {
    serde_json::to_writer(&mut *stream, value)
        .map_err(|error| format!("failed to encode control message: {error}"))?;
    stream
        .write_all(b"\n")
        .map_err(|error| format!("failed to write control channel: {error}"))?;
    stream
        .flush()
        .map_err(|error| format!("failed to flush control channel: {error}"))
}

fn io_error(action: &'static str) -> impl FnOnce(io::Error) -> String {
    move |error| format!("failed to {action}: {error}")
}
