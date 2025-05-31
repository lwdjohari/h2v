// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025 Linggawasistha Djohari
// <linggawasistha.djohari@outlook.com>

#pragma once

#include <yaml-cpp/yaml.h>
#include <unordered_map>
#include <iostream>
#include <string>

namespace h2v {
namespace utils {
namespace yaml {

template <typename Enum>
Enum ParseEnum(const YAML::Node& node,
               const std::unordered_map<std::string, Enum>& map,
               const std::string& ctx) {
  if (!node || !node.IsScalar()) {
    throw std::runtime_error("[YAML ERROR] Expected scalar enum in context: " +
                             ctx);
  }
  const std::string key = node.as<std::string>();
  auto it = map.find(key);
  if (it == map.end()) {
    throw std::runtime_error("[YAML ERROR] Unknown enum value '" + key +
                             "' in context: " + ctx);
  }
  return it->second;
}

// Safely fetch a typed value from a YAML node by key, or return a default.
// Logs a clear error if conversion fails.
template <typename T>
T GetOrDefault(const YAML::Node& node, const std::string& key, const T& defval,
               const std::string& ctx = "") {
  try {
    if (node[key]) {
      return node[key].as<T>();
    }
  } catch (const std::exception& ex) {
    std::cerr << "[YAML ERROR] Failed to parse key '" << key << "' in context '"
              << ctx << "': " << ex.what() << std::endl;
  }
  return defval;
}

// Require a key to exist and be convertible; throw if not.
// Exception contains key and context name.
template <typename T>
T Require(const YAML::Node& node, const std::string& key,
          const std::string& ctx = "") {
  if (!node[key]) {
    throw std::runtime_error("[YAML ERROR] Missing required key '" + key +
                             "' in context '" + ctx + "'");
  }
  try {
    return node[key].as<T>();
  } catch (const std::exception& ex) {
    throw std::runtime_error("[YAML ERROR] Failed to parse key '" + key +
                             "' in context '" + ctx + "': " + ex.what());
  }
}

}  // namespace cli
}  // namespace utils
}  // namespace h2v