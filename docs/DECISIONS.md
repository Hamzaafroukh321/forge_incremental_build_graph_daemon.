# Architecture Decisions

## ADR-0001: Use Explicit Result Values Instead Of Exceptions Across Public Core APIs

Context: Forge handles untrusted protocol bytes, persisted state, and fault-injection tests. The spec requires stable error categories and cleanup after partial initialization.

Decision: Public core APIs return `forge::Result<T>` or `forge::Status`. Constructors keep work minimal; parsing, graph mutation, state replay, and publication return typed errors.

Alternatives: Exceptions throughout the core would reduce call-site checks but make fuzz/recovery control flow harder to audit. Error-code-only APIs would lose context.

Consequences: Callers must check results explicitly. Tests can assert exact categories and offsets.

Validation: Named tests cover malformed frames, graph conflicts, stale leases, and recovery errors.

## ADR-0002: Keep Platform-Specific Daemon I/O Behind Adapters

Context: The specification targets Linux Unix-domain sockets and inotify-compatible watchers, while this development environment is Windows and lacks a C++ toolchain.

Decision: Implement the production core, FIPC codec, session state, workspace model, and CLI state workflows portably. Linux socket/watcher bindings remain adapter entry points and must be validated on Linux; the first Unix-domain socket status adapter is now covered in Docker.

Alternatives: Write Linux-only code that cannot be compiled or exercised here; or use a third-party portability layer before license review.

Consequences: Core behavior is testable without kernel watchers. Docker now provides Linux compiler validation and a Unix-domain socket status smoke, but inotify and long-running daemon runtime validation remain pending until those adapters are implemented and exercised.

Validation: Portable tests drive the same core entry points that Linux adapters will feed.

## ADR-0003: Use A Small Internal Deterministic Digest Primitive Initially

Context: Forge needs stable content/action/state keys and CRC checks. The dependency policy prefers no production dependency unless necessary and license-reviewed.

Decision: Implement CRC32C and a deterministic 256-bit content key combiner internally for action/CAS keys. Document that cryptographic strength is not claimed.

Alternatives: Add OpenSSL/BLAKE3/xxHash. Those are reasonable future choices but require dependency/license review.

Consequences: Keys are stable and testable, but not suitable as security boundaries. A future schema version may migrate digest algorithms and force rebuild.

Validation: Tests cover canonical re-encoding, digest mismatch rejection, and map-order-independent action keys.

## ADR-0004: Treat Fuzz Harnesses As Production Entry-Point Drivers

Context: The spec prohibits duplicate parser/scheduler implementations in fuzz targets.

Decision: Harnesses decode only bounded operation streams and call production FIPC/session/workspace APIs for all semantics.

Alternatives: Build standalone fuzz parsers that only approximate behavior.

Consequences: Harnesses are more coupled to production APIs but provide meaningful sanitizer coverage.

Validation: Fuzz targets link `forge_core` and no test-only parser library exists.
