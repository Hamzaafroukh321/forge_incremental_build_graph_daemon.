cmake_minimum_required(VERSION 3.24)
project(forge_incremental_build_graph_daemon VERSION 0.1.0 LANGUAGES CXX)

option(FORGE_WARNINGS_AS_ERRORS "Treat first-party warnings as errors" ON)
option(FORGE_ENABLE_SANITIZERS "Enable sanitizer flags" OFF)
option(FORGE_ENABLE_FUZZING "Build fuzz harnesses" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Warnings.cmake)
include(cmake/Sanitizers.cmake)

add_library(forge_core
  src/base/checked.cpp
  src/base/id.cpp
  src/base/virtual_clock.cpp
  src/ipc/fipc_codec.cpp
  src/ipc/session.cpp
  src/ipc/client.cpp
  src/ipc/server.cpp
  src/workspace/workspace.cpp
  src/workspace/path_policy.cpp
  src/workspace/file_coalescer.cpp
  src/workspace/fingerprint.cpp
  src/workspace/manifest.cpp
  src/graph/build_graph.cpp
  src/graph/invalidation.cpp
  src/graph/cycle_analyzer.cpp
  src/graph/snapshot.cpp
  src/schedule/scheduler.cpp
  src/schedule/request.cpp
  src/schedule/job.cpp
  src/schedule/resource_pool.cpp
  src/execute/executor_manager.cpp
  src/execute/mock_executor.cpp
  src/execute/subprocess_executor.cpp
  src/execute/discovery.cpp
  src/artifact/cas.cpp
  src/artifact/artifact_writer.cpp
  src/artifact/gc.cpp
  src/state/state_codec.cpp
  src/state/append_log.cpp
  src/state/checkpoint.cpp
  src/state/recovery.cpp
  src/state/compact.cpp
  src/event/event_hub.cpp
  src/event/explanation.cpp
)
target_include_directories(forge_core PUBLIC include)
forge_apply_warnings(forge_core)
forge_apply_sanitizers(forge_core)

add_executable(forge src/cli/forge_main.cpp src/cli/cmd_build.cpp src/cli/cmd_graph.cpp src/cli/cmd_status.cpp)
target_link_libraries(forge PRIVATE forge_core)
forge_apply_warnings(forge)
forge_apply_sanitizers(forge)

add_executable(forged src/cli/forged_main.cpp)
target_link_libraries(forged PRIVATE forge_core)
forge_apply_warnings(forged)
forge_apply_sanitizers(forged)

enable_testing()
add_executable(forge_tests
  tests/unit/forge_tests.cpp
  tests/integration/build_flow.cpp
)
target_link_libraries(forge_tests PRIVATE forge_core)
forge_apply_warnings(forge_tests)
forge_apply_sanitizers(forge_tests)
add_test(NAME forge_tests COMMAND forge_tests)

if(FORGE_ENABLE_FUZZING)
  include(cmake/FuzzTargets.cmake)
  forge_add_fuzz_target(forge_fipc_decoder_fuzz fuzz/fuzz_fipc_decoder.cpp)
  forge_add_fuzz_target(forge_daemon_event_sequence_fuzz fuzz/fuzz_daemon_events.cpp)
  forge_add_fuzz_target(forge_build_pipeline_fuzz fuzz/fuzz_build_pipeline.cpp)
endif()

install(TARGETS forge_core forge forged EXPORT forgeTargets)
install(DIRECTORY include/forge DESTINATION include)
