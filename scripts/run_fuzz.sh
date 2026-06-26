#!/usr/bin/env sh
set -eu
./build/fuzz/forge_fipc_decoder_fuzz -runs=1000
./build/fuzz/forge_daemon_event_sequence_fuzz -runs=1000
./build/fuzz/forge_build_pipeline_fuzz -runs=1000
