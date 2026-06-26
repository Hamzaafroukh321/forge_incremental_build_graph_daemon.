#include "forge/manifest.hpp"

#include <set>
#include <sstream>

namespace forge {
namespace {

std::string trim_comment(std::string line) {
  const auto comment = line.find('#');
  if (comment != std::string::npos) {
    line.erase(comment);
  }
  while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
    line.pop_back();
  }
  std::size_t first = 0;
  while (first < line.size() && (line[first] == ' ' || line[first] == '\t')) {
    ++first;
  }
  line.erase(0, first);
  return line;
}

Result<std::pair<std::string, std::string>> parse_option(std::string_view token) {
  const auto equals = token.find('=');
  if (equals == std::string_view::npos || equals == 0 || equals + 1 >= token.size()) {
    return Status::error(ErrorCode::format, "manifest", "node option must be key=value");
  }
  return std::pair<std::string, std::string>{std::string(token.substr(0, equals)), std::string(token.substr(equals + 1))};
}

Result<EdgeKind> parse_edge_kind(const std::string& text) {
  if (text == "declared") {
    return EdgeKind::declared;
  }
  if (text == "discovered") {
    return EdgeKind::discovered;
  }
  return Status::error(ErrorCode::format, "manifest", "edge kind must be declared or discovered");
}

}  // namespace

Result<GraphManifest> parse_manifest(std::istream& input) {
  GraphManifest manifest;
  std::string line;
  std::size_t line_number = 0;
  std::set<std::string> node_names;
  while (std::getline(input, line)) {
    ++line_number;
    line = trim_comment(std::move(line));
    if (line.empty()) {
      continue;
    }
    std::istringstream parts(line);
    std::string record;
    parts >> record;
    if (record == "node") {
      std::string name;
      std::string command;
      parts >> name >> command;
      if (name.empty() || command.empty()) {
        auto status = Status::error(ErrorCode::format, "manifest", "node requires name and command");
        status.offset = line_number;
        return status;
      }
      if (!node_names.insert(name).second) {
        auto status = Status::error(ErrorCode::graph_conflict, "manifest", "duplicate node name");
        status.offset = line_number;
        return status;
      }
      ManifestNode node{name, ActionSpec{command, {}}};
      std::string option;
      while (parts >> option) {
        auto parsed = parse_option(option);
        if (!parsed.ok()) {
          auto status = parsed.status();
          status.offset = line_number;
          return status;
        }
        auto inserted = node.action.options.emplace(std::move(parsed.value().first), std::move(parsed.value().second));
        if (!inserted.second) {
          auto status = Status::error(ErrorCode::format, "manifest", "duplicate node option");
          status.offset = line_number;
          return status;
        }
      }
      manifest.nodes.push_back(std::move(node));
    } else if (record == "edge") {
      std::string from;
      std::string to;
      std::string kind_text = "declared";
      parts >> from >> to;
      if (!(parts >> kind_text)) {
        kind_text = "declared";
      }
      if (from.empty() || to.empty()) {
        auto status = Status::error(ErrorCode::format, "manifest", "edge requires from and to");
        status.offset = line_number;
        return status;
      }
      auto kind = parse_edge_kind(kind_text);
      if (!kind.ok()) {
        auto status = kind.status();
        status.offset = line_number;
        return status;
      }
      manifest.edges.push_back(ManifestEdge{from, to, kind.value()});
    } else if (record == "target") {
      std::string name;
      parts >> name;
      if (name.empty()) {
        auto status = Status::error(ErrorCode::format, "manifest", "target requires node name");
        status.offset = line_number;
        return status;
      }
      manifest.targets.push_back(std::move(name));
    } else {
      auto status = Status::error(ErrorCode::format, "manifest", "unknown manifest record");
      status.offset = line_number;
      return status;
    }
  }
  if (manifest.nodes.empty()) {
    return Status::error(ErrorCode::format, "manifest", "manifest contains no nodes");
  }
  return manifest;
}

Result<AppliedManifest> apply_manifest(WorkspaceCore& workspace, const GraphManifest& manifest) {
  AppliedManifest applied;
  for (const auto& node : manifest.nodes) {
    auto id = workspace.put_node(node.name, node.action);
    if (!id.ok()) {
      return id.status();
    }
    applied.nodes_by_name.emplace(node.name, id.value());
  }
  for (const auto& edge : manifest.edges) {
    const auto from = applied.nodes_by_name.find(edge.from);
    const auto to = applied.nodes_by_name.find(edge.to);
    if (from == applied.nodes_by_name.end() || to == applied.nodes_by_name.end()) {
      return Status::error(ErrorCode::graph_conflict, "manifest", "edge references unknown node name");
    }
    auto inserted = workspace.put_edge(from->second, to->second, edge.kind);
    if (!inserted.ok()) {
      return inserted.status();
    }
  }
  for (const auto& target : manifest.targets) {
    const auto found = applied.nodes_by_name.find(target);
    if (found == applied.nodes_by_name.end()) {
      return Status::error(ErrorCode::graph_conflict, "manifest", "target references unknown node name");
    }
    applied.targets.push_back(found->second);
  }
  if (applied.targets.empty() && !manifest.nodes.empty()) {
    applied.targets.push_back(applied.nodes_by_name.at(manifest.nodes.back().name));
  }
  return applied;
}

}  // namespace forge
