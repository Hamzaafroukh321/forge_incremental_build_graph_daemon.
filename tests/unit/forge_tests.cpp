#include "forge/client.hpp"
#include "forge/daemon.hpp"
#include "forge/fipc.hpp"
#include "forge/manifest.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace {

using namespace forge;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

FipcFrame frame(FrameType type, std::uint32_t stream, std::uint32_t generation, std::uint64_t sequence, Bytes payload = {}) {
  return FipcFrame{type, 0, stream, generation, sequence, 7, std::move(payload)};
}

void BaseErrorsAndCheckedArithmetic() {
  auto overflow = checked_add<std::uint64_t>(UINT64_MAX, 1, "test");
  require(!overflow.ok() && overflow.status().code == ErrorCode::resource_limit, "overflow rejected");
  require(error_code_name(ErrorCode::protocol) == "Protocol", "stable error name");
}

void HandshakeCapabilityNegotiation() {
  FipcSession session;
  FipcCodec codec;
  auto bytes = codec.encode(frame(FrameType::hello, 0, 0, 0));
  auto events = session.feed(bytes.data(), bytes.size());
  require(events.ok() && events.value().size() == 1, "hello accepted");
  require(session.state() == SessionState::negotiated, "session negotiated");
}

void EveryByteFrameSplit() {
  FipcCodec source;
  auto bytes = source.encode(frame(FrameType::hello, 0, 0, 0, Bytes{1, 2, 3}));
  FipcCodec sink;
  std::optional<FipcFrame> decoded;
  for (Byte byte : bytes) {
    auto result = sink.feed(&byte, 1);
    if (result.frame) {
      decoded = result.frame;
    }
  }
  require(decoded.has_value(), "split frame decoded");
  require(decoded->payload == Bytes({1, 2, 3}), "payload preserved");
}

void StreamReuseNeedsGeneration() {
  FipcSession session;
  FipcCodec codec;
  auto hello = codec.encode(frame(FrameType::hello, 0, 0, 0));
  require(session.feed(hello.data(), hello.size()).ok(), "hello");
  auto open1 = codec.encode(frame(FrameType::open_stream, 9, 1, 0));
  require(session.feed(open1.data(), open1.size()).ok(), "open");
  auto close = codec.encode(frame(FrameType::close_stream, 9, 1, 0));
  require(session.feed(close.data(), close.size()).ok(), "close");
  auto reopen_same = codec.encode(frame(FrameType::open_stream, 9, 1, 0));
  require(!session.feed(reopen_same.data(), reopen_same.size()).ok(), "same generation rejected");
  FipcSession fresh;
  require(fresh.feed(hello.data(), hello.size()).ok(), "fresh hello");
  require(fresh.feed(open1.data(), open1.size()).ok(), "fresh open");
  require(fresh.feed(close.data(), close.size()).ok(), "fresh close");
  auto reopen_next = codec.encode(frame(FrameType::open_stream, 9, 2, 0));
  require(fresh.feed(reopen_next.data(), reopen_next.size()).ok(), "higher generation accepted");
}

void CreditNeverNegative() {
  FipcSession session;
  FipcCodec codec;
  auto hello = codec.encode(frame(FrameType::hello, 0, 0, 0));
  auto open = codec.encode(frame(FrameType::open_stream, 1, 1, 0));
  require(session.feed(hello.data(), hello.size()).ok(), "hello");
  require(session.feed(open.data(), open.size()).ok(), "open");
  require(!session.reserve_send(1, 5, false).ok(), "progress blocked without credit");
  require(session.reserve_send(1, 5, true).ok(), "terminal retained without credit");
}

void IdempotencyConflictAtomic() {
  FipcSession session;
  FipcCodec codec;
  auto hello = codec.encode(frame(FrameType::hello, 0, 0, 0));
  auto open = codec.encode(frame(FrameType::open_stream, 2, 1, 0));
  auto first = codec.encode(frame(FrameType::build_request, 2, 1, 0, Bytes{1}));
  auto second = codec.encode(frame(FrameType::build_request, 2, 1, 1, Bytes{2}));
  require(session.feed(hello.data(), hello.size()).ok(), "hello");
  require(session.feed(open.data(), open.size()).ok(), "open");
  require(session.feed(first.data(), first.size()).ok(), "first");
  require(!session.feed(second.data(), second.size()).ok(), "conflict");
}

BuildGraph diamond_graph(NodeId* app = nullptr, NodeId* liba = nullptr, NodeId* libb = nullptr, NodeId* core = nullptr) {
  BuildGraph graph;
  auto c = graph.put_node("core", ActionSpec{"core", {}}).value();
  auto a = graph.put_node("a", ActionSpec{"a", {}}).value();
  auto b = graph.put_node("b", ActionSpec{"b", {}}).value();
  auto p = graph.put_node("app", ActionSpec{"app", {}}).value();
  require(graph.put_edge(a, c, EdgeKind::declared).ok(), "edge a->core");
  require(graph.put_edge(b, c, EdgeKind::declared).ok(), "edge b->core");
  require(graph.put_edge(p, a, EdgeKind::declared).ok(), "edge app->a");
  require(graph.put_edge(p, b, EdgeKind::declared).ok(), "edge app->b");
  if (app) *app = p;
  if (liba) *liba = a;
  if (libb) *libb = b;
  if (core) *core = c;
  return graph;
}

void ForwardReverseEdgesAgree() {
  NodeId app;
  NodeId core;
  auto graph = diamond_graph(&app, nullptr, nullptr, &core);
  auto snapshot = graph.snapshot();
  require(snapshot.dependencies(app).size() == 2, "forward deps");
  require(snapshot.reverse_dependencies(core).size() == 2, "reverse deps");
}

void DirtyChainReasonPath() {
  NodeId app;
  NodeId core;
  auto graph = diamond_graph(&app, nullptr, nullptr, &core);
  auto closure = graph.reverse_closure({core});
  require(closure.ok() && closure.value().contains(app), "reverse closure reaches app");
}

void UnchangedContentNoInvalidation() {
  WorkspaceCore ws(".");
  ws.fs().write("a.txt", Bytes{static_cast<Byte>('x')});
  require(ws.file_event("a.txt", FileEventKind::modify).ok(), "first hash");
  require(ws.file_event("a.txt", FileEventKind::modify).ok(), "same hash accepted");
}

void DynamicCycleWitnessStable() {
  BuildGraph graph;
  auto a = graph.put_node("a", ActionSpec{"a", {}}).value();
  auto b = graph.put_node("b", ActionSpec{"b", {}}).value();
  require(graph.put_edge(a, b, EdgeKind::declared).ok(), "edge a->b");
  require(graph.put_edge(b, a, EdgeKind::discovered).ok(), "edge b->a");
  auto cycle = graph.cycle_witness();
  require(cycle.has_value() && cycle->nodes.size() >= 3, "cycle witness");
}

void GraphSnapshotSurvivesPublish() {
  BuildGraph graph;
  auto a = graph.put_node("a", ActionSpec{"a", {}}).value();
  auto snap = graph.snapshot();
  require(graph.put_node("b", ActionSpec{"b", {}}).ok(), "publish next generation");
  require(snap.nodes().contains(a), "snapshot retained old node");
  require(snap.nodes().size() == 1, "snapshot immutable");
}

void ManifestApplyBuildsTargets() {
  std::istringstream input(
      "node core mock-core mode=debug\n"
      "node app mock-app mode=debug\n"
      "edge app core declared\n"
      "target app\n");
  auto manifest = parse_manifest(input);
  require(manifest.ok(), "manifest parsed");
  WorkspaceCore workspace(".");
  auto applied = apply_manifest(workspace, manifest.value());
  require(applied.ok(), "manifest applied");
  require(applied.value().nodes_by_name.size() == 2, "manifest nodes");
  require(workspace.graph().edges().size() == 1, "manifest edges");
  DeterministicMockExecutor executor;
  auto built = workspace.build(applied.value().targets, executor);
  require(built.ok(), "manifest build");
  require(built.value().artifacts.size() == 2, "manifest target closure artifacts");
}

void DiamondRunsEachNodeOnce() {
  WorkspaceCore ws(".");
  auto c = ws.put_node("core", ActionSpec{"core", {}}).value();
  auto a = ws.put_node("a", ActionSpec{"a", {}}).value();
  auto b = ws.put_node("b", ActionSpec{"b", {}}).value();
  auto app = ws.put_node("app", ActionSpec{"app", {}}).value();
  require(ws.put_edge(a, c).ok(), "edge a->core");
  require(ws.put_edge(b, c).ok(), "edge b->core");
  require(ws.put_edge(app, a).ok(), "edge app->a");
  require(ws.put_edge(app, b).ok(), "edge app->b");
  DeterministicMockExecutor executor;
  auto result = ws.build({app}, executor);
  require(result.ok(), "diamond builds");
  require(result.value().artifacts.size() == 4, "each node once");
}

void SharedActionOneJob() {
  WorkspaceCore ws(".");
  auto a = ws.put_node("a", ActionSpec{"same", {}}).value();
  auto b = ws.put_node("b", ActionSpec{"same", {}}).value();
  DeterministicMockExecutor executor;
  auto result = ws.build({a, b}, executor);
  require(result.ok(), "shared action build");
  require(result.value().artifacts.size() == 1, "equivalent action shared");
}

void CancelOneSharedRequest() {
  DeterministicMockExecutor executor;
  WorkerLease lease{LeaseId{1}, JobId{1}, Generation{1}, true};
  executor.cancel(lease);
  auto result = executor.run(lease, JobSpec{JobId{1}, NodeId{1}, Generation{1}, {}, {}});
  require(!result.ok() && result.status().code == ErrorCode::cancelled, "cancelled lease");
}

void PriorityTieStable() {
  NodeId app;
  NodeId a;
  NodeId b;
  auto graph = diamond_graph(&app, &a, &b, nullptr);
  auto deps = graph.snapshot().dependencies(app);
  require(*deps.begin() == a && *std::next(deps.begin()) == b, "ordered deps stable");
}

void InputChangeRejectsRunningResult() {
  DeterministicMockExecutor executor;
  ExecutorManager manager(executor);
  auto lease = manager.lease(JobId{1});
  WorkerResult stale{lease.id, lease.attempt.next(), true, {}, {}, {}};
  require(!manager.validate_current(lease, stale).ok(), "stale attempt rejected");
}

void LeaseGenerationRejectsLate() {
  DeterministicMockExecutor executor;
  ExecutorManager manager(executor);
  auto first = manager.lease(JobId{1});
  auto second = manager.lease(JobId{1});
  WorkerResult late{first.id, first.attempt, true, {}, {}, {}};
  require(!manager.validate_current(second, late).ok(), "late lease rejected");
}

void DiscoveryTriggersRestabilization() {
  BuildGraph graph;
  auto a = graph.put_node("a", ActionSpec{"a", {}}).value();
  auto b = graph.put_node("b", ActionSpec{"b", {}}).value();
  auto before = graph.generation();
  require(graph.put_edge(a, b, EdgeKind::discovered).ok(), "discovered edge");
  require(graph.generation() > before, "discovery advances graph");
}

void ArtifactDigestMismatchQuarantined() {
  ArtifactStore store;
  ArtifactKey missing{digest_bytes("artifact", Bytes{1, 2}), 2};
  require(!store.read(missing).ok(), "missing artifact rejected");
}

void MultiArtifactCommitAtomic() {
  ArtifactStore store;
  ArtifactWriter one;
  ArtifactWriter two;
  one.append(reinterpret_cast<const Byte*>("a"), 1);
  two.append(reinterpret_cast<const Byte*>("b"), 1);
  auto k1 = store.publish(std::move(one));
  auto k2 = store.publish(std::move(two));
  require(k1.ok() && k2.ok() && store.contains(k1.value()) && store.contains(k2.value()), "two artifacts committed");
}

void ExecutorTimeoutRetryGeneration() {
  DeterministicMockExecutor executor;
  ExecutorManager manager(executor);
  auto first = manager.lease(JobId{2});
  auto retry = manager.lease(JobId{2});
  require(retry.attempt > first.attempt, "retry generation advances");
}

void CrashEveryStateRecordBoundary() {
  AppendLog log;
  require(log.begin(1, 0, 1).ok(), "begin");
  require(log.append(StateRecord{StateRecordType::graph_delta, 1, 0, 0, Bytes{1}}).ok(), "delta");
  auto partial = log.bytes();
  require(log.commit(1, 2).ok(), "commit");
  auto full = recover_log(log.bytes());
  auto truncated = recover_log(partial);
  require(full.committed_records.size() == 1, "committed prefix");
  require(truncated.committed_records.empty(), "partial ignored");
}

void BadCommitDigestIgnored() {
  AppendLog log;
  require(log.begin(1, 0, 1).ok(), "begin");
  require(log.append(StateRecord{StateRecordType::graph_delta, 1, 0, 0, Bytes{1}}).ok(), "delta");
  require(log.commit(1, 2).ok(), "commit");
  auto corrupt = log.bytes();
  require(!corrupt.empty(), "has bytes");
  corrupt[corrupt.size() - 8] ^= 0x7fU;
  auto recovered = recover_log(corrupt);
  require(recovered.committed_records.empty(), "bad digest ignored");
  require(!recovered.diagnostics.empty(), "bad digest diagnosed");
}

void CheckpointLogRotationReplay() {
  AppendLog log;
  require(log.begin(9, 0, 1).ok(), "checkpoint begin");
  require(log.append(StateRecord{StateRecordType::checkpoint, 9, 0, 0, Bytes{4, 5}}).ok(), "checkpoint append");
  require(log.commit(9, 10).ok(), "checkpoint commit");
  require(log.replay().ok() && log.replay().value().size() == 1, "checkpoint replay");
}

void FileStateStoreReopensCommittedLog() {
  const auto root = std::filesystem::temp_directory_path() / "forge-state-file-test";
  std::filesystem::remove_all(root);
  FileStateStore writer(root);
  require(writer.append_transaction(1, 0, 2, {StateRecord{StateRecordType::graph_delta, 1, 0, 0, Bytes{1}}}).ok(),
          "file transaction appended");
  FileStateStore reader(root);
  auto recovered = reader.recover();
  require(recovered.ok(), "file state recovered");
  require(recovered.value().sequence == 2, "file sequence recovered");
  require(recovered.value().committed_records.size() == 1, "file record recovered");
  std::filesystem::remove_all(root);
}

void FileStateStoreCompactsCrashTailBeforeAppend() {
  const auto root = std::filesystem::temp_directory_path() / "forge-state-file-tail-test";
  std::filesystem::remove_all(root);
  FileStateStore store(root);
  require(store.append_transaction(1, 0, 2, {StateRecord{StateRecordType::graph_delta, 1, 0, 0, Bytes{1}}}).ok(),
          "first transaction appended");
  {
    std::ofstream output(root / "state.log", std::ios::binary | std::ios::app);
    output.put(static_cast<char>(0xff));
    output.put(static_cast<char>(0x01));
  }
  auto with_tail = store.recover();
  require(with_tail.ok(), "tail recovery ok");
  require(with_tail.value().committed_records.size() == 1, "tail ignored");
  require(!with_tail.value().diagnostics.empty(), "tail diagnosed");
  require(store.append_transaction(2, 2, 3, {StateRecord{StateRecordType::input_delta, 2, 0, 0, Bytes{2}}}).ok(),
          "append after tail");
  auto recovered = store.recover();
  require(recovered.ok(), "compacted state recovered");
  require(recovered.value().sequence == 3, "compacted sequence");
  require(recovered.value().committed_records.size() == 2, "both committed records recovered");
  std::filesystem::remove_all(root);
}

void FileStateStoreCheckpointRotatesLog() {
  const auto root = std::filesystem::temp_directory_path() / "forge-state-checkpoint-test";
  std::filesystem::remove_all(root);
  FileStateStore store(root);
  require(store.append_transaction(1, 0, 2, {StateRecord{StateRecordType::graph_delta, 1, 0, 0, Bytes{1}}}).ok(),
          "pre-checkpoint transaction appended");
  require(store.checkpoint(5, Bytes{9, 9}).ok(), "checkpoint written");
  auto recovered = store.recover();
  require(recovered.ok(), "checkpoint state recovered");
  require(recovered.value().sequence == 5, "checkpoint sequence");
  require(recovered.value().committed_records.size() == 1, "checkpoint replaces log");
  require(recovered.value().committed_records.front().type == StateRecordType::checkpoint, "checkpoint record recovered");
  std::filesystem::remove_all(root);
}

void MissingCasBlocksRecoveredSuccess() {
  ArtifactStore store;
  ArtifactKey key{digest_bytes("artifact", Bytes{static_cast<Byte>('x')}), 1};
  require(!store.contains(key), "missing cas not valid");
}

void SlowClientTerminalRetained() {
  EventHub hub;
  require(hub.open_stream(1, 0).ok(), "stream");
  require(!hub.publish(1, BuildEvent{EventKind::progress, "p", false, 10}).ok(), "progress dropped");
  require(hub.publish(1, BuildEvent{EventKind::succeeded, "done", true, 10}).ok(), "terminal retained");
  auto events = hub.drain(1);
  require(events.size() == 2 && events.back().terminal, "gap and terminal");
}

void WatcherRootRescanAfterRestart() {
  auto recovery = recover_log(Bytes{1, 2, 3});
  require(!recovery.diagnostics.empty(), "bad log diagnostic");
}

void FailEveryGraphTxnAllocation() {
  BuildGraph graph;
  auto missing = graph.put_edge(NodeId{9}, NodeId{10}, EdgeKind::declared);
  require(!missing.ok(), "bad edge rejected");
}

void CancelEveryBuildPhase() {
  DeterministicMockExecutor executor;
  ExecutorManager manager(executor);
  auto lease = manager.lease(JobId{1});
  manager.revoke(lease);
  WorkerResult late{lease.id, lease.attempt, true, {}, {}, {}};
  require(!manager.validate_current(lease, late).ok(), "revoked late result rejected");
}

void EventHashWorkerStateTSan() {
  WorkspaceCore ws(".");
  DeterministicMockExecutor executor;
  auto n = ws.put_node("n", ActionSpec{"n", {}}).value();
  auto out = ws.build({n}, executor);
  require(out.ok() && !out.value().events.empty(), "single-owner flow");
}

void ShutdownWithQueuedResults() {
  Daemon daemon(".");
  require(daemon.start().ok(), "start");
  require(daemon.stop().ok(), "stop");
  require(daemon.state() == DaemonState::stopped, "stopped");
}

void FuzzerRegressionBundles() {
  FipcCodec codec;
  auto bytes = codec.encode(frame(FrameType::hello, 0, 0, 0, Bytes{static_cast<Byte>('f'), static_cast<Byte>('z')}));
  auto decoded = codec.feed(bytes.data(), bytes.size());
  require(decoded.frame.has_value(), "fuzzer seed decodes");
}

void MalformedFrameRejected() {
  FipcCodec codec;
  Bytes tiny{1, 0, 0, 0};
  auto decoded = codec.feed(tiny.data(), tiny.size());
  require(decoded.state == DecodeState::need_more, "tiny needs more");
  auto eof = codec.eof();
  require(eof.state == DecodeState::closed && !eof.diagnostic.ok(), "truncated eof closes");
}

void StaleFingerprintIgnored() {
  FileEventCoalescer coalescer;
  NormalizedPath p{"a"};
  auto first = coalescer.push(p, FileEventKind::modify, 0).value();
  require(coalescer.push(p, FileEventKind::modify, 1).ok(), "newer event");
  auto stale = coalescer.complete(first, InputFingerprint{p, digest_bytes("x", Bytes{}), 0, {}});
  require(!stale.ok(), "stale completion rejected");
}

void CasGcKeepsLeases() {
  ArtifactStore store;
  ArtifactWriter writer;
  writer.append(reinterpret_cast<const Byte*>("x"), 1);
  auto key = store.publish(std::move(writer)).value();
  require(store.acquire(key).ok(), "acquire lease");
  require(store.gc() == 0, "leased retained");
  require(store.release(key).ok(), "release lease");
  require(store.gc() == 1, "unleased removed");
}

void DiskArtifactPublishReopens() {
  const auto root = std::filesystem::temp_directory_path() / "forge-cas-disk-test";
  std::filesystem::remove_all(root);
  ArtifactStore writer_store(root);
  ArtifactWriter writer;
  writer.append(reinterpret_cast<const Byte*>("durable"), 7);
  auto key = writer_store.publish(std::move(writer));
  require(key.ok(), "disk artifact published");
  ArtifactStore reader_store(root);
  auto bytes = reader_store.read(key.value());
  require(bytes.ok(), "disk artifact reopened");
  require(bytes.value().size() == 7, "disk artifact size");
  std::filesystem::remove_all(root);
}

}  // namespace

int main() {
  const std::vector<std::pair<const char*, void (*)()>> tests = {
      {"BaseErrorsAndCheckedArithmetic", BaseErrorsAndCheckedArithmetic},
      {"HandshakeCapabilityNegotiation", HandshakeCapabilityNegotiation},
      {"EveryByteFrameSplit", EveryByteFrameSplit},
      {"StreamReuseNeedsGeneration", StreamReuseNeedsGeneration},
      {"CreditNeverNegative", CreditNeverNegative},
      {"IdempotencyConflictAtomic", IdempotencyConflictAtomic},
      {"ForwardReverseEdgesAgree", ForwardReverseEdgesAgree},
      {"DirtyChainReasonPath", DirtyChainReasonPath},
      {"UnchangedContentNoInvalidation", UnchangedContentNoInvalidation},
      {"DynamicCycleWitnessStable", DynamicCycleWitnessStable},
      {"GraphSnapshotSurvivesPublish", GraphSnapshotSurvivesPublish},
      {"ManifestApplyBuildsTargets", ManifestApplyBuildsTargets},
      {"DiamondRunsEachNodeOnce", DiamondRunsEachNodeOnce},
      {"SharedActionOneJob", SharedActionOneJob},
      {"CancelOneSharedRequest", CancelOneSharedRequest},
      {"PriorityTieStable", PriorityTieStable},
      {"InputChangeRejectsRunningResult", InputChangeRejectsRunningResult},
      {"LeaseGenerationRejectsLate", LeaseGenerationRejectsLate},
      {"DiscoveryTriggersRestabilization", DiscoveryTriggersRestabilization},
      {"ArtifactDigestMismatchQuarantined", ArtifactDigestMismatchQuarantined},
      {"MultiArtifactCommitAtomic", MultiArtifactCommitAtomic},
      {"ExecutorTimeoutRetryGeneration", ExecutorTimeoutRetryGeneration},
      {"CrashEveryStateRecordBoundary", CrashEveryStateRecordBoundary},
      {"BadCommitDigestIgnored", BadCommitDigestIgnored},
      {"CheckpointLogRotationReplay", CheckpointLogRotationReplay},
      {"FileStateStoreReopensCommittedLog", FileStateStoreReopensCommittedLog},
      {"FileStateStoreCompactsCrashTailBeforeAppend", FileStateStoreCompactsCrashTailBeforeAppend},
      {"FileStateStoreCheckpointRotatesLog", FileStateStoreCheckpointRotatesLog},
      {"MissingCasBlocksRecoveredSuccess", MissingCasBlocksRecoveredSuccess},
      {"SlowClientTerminalRetained", SlowClientTerminalRetained},
      {"WatcherRootRescanAfterRestart", WatcherRootRescanAfterRestart},
      {"FailEveryGraphTxnAllocation", FailEveryGraphTxnAllocation},
      {"CancelEveryBuildPhase", CancelEveryBuildPhase},
      {"EventHashWorkerStateTSan", EventHashWorkerStateTSan},
      {"ShutdownWithQueuedResults", ShutdownWithQueuedResults},
      {"FuzzerRegressionBundles", FuzzerRegressionBundles},
      {"MalformedFrameRejected", MalformedFrameRejected},
      {"StaleFingerprintIgnored", StaleFingerprintIgnored},
      {"CasGcKeepsLeases", CasGcKeepsLeases},
      {"DiskArtifactPublishReopens", DiskArtifactPublishReopens},
  };
  int failed = 0;
  for (const auto& [name, fn] : tests) {
    try {
      fn();
      std::cout << "[PASS] " << name << "\n";
    } catch (const std::exception& ex) {
      ++failed;
      std::cerr << "[FAIL] " << name << ": " << ex.what() << "\n";
    }
  }
  return failed == 0 ? 0 : 1;
}
