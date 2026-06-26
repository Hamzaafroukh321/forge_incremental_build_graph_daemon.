# Security

Forge treats protocol frames, state files, file paths, worker results, and fuzz inputs as untrusted.

Supported versions: pre-1.0 development only.

Security posture:

- Bounds are checked before allocation or slicing.
- Protocol and state records use CRC32C integrity checks.
- Paths are normalized and must remain workspace-relative.
- Fuzz harnesses and deterministic mock execution do not run arbitrary shell commands.
- Diagnostics should include validated ids, offsets, and categories, not secrets.

Report issues through the repository owner. Do not include private credentials or proprietary corpora in bug reports.
