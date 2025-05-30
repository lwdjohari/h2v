#pragma once

#include <cstdint>

namespace h2v {
namespace hpack {

/// @brief Identifies the form of an HPACK header field or instruction.
/// @details Used for tracing, metrics, and diagnostics to distinguish
///   how each header was represented on the wire.
enum class EntryType : uint8_t {
  /// A fully indexed header field (static or dynamic table lookup).
  /// See RFC 7541 §6.1.
  IndexedHeader = 0,

  /// Literal header with incremental indexing (insert into dynamic table).
  /// See RFC 7541 §6.2.1.
  LiteralWithIncrementalIndexing = 1,

  /// Literal header without any indexing (not inserted in dynamic table).
  /// See RFC 7541 §6.2.2.
  LiteralWithoutIndexing = 2,

  /// Literal header that must never be indexed (sensitive values).
  /// See RFC 7541 §6.2.3.
  LiteralNeverIndexed = 3,

  /// Dynamic table size update instruction (Table Size Update).
  /// See RFC 7541 §6.3.
  DynamicTableSizeUpdate = 4
};

}  // namespace hpack
}  // namespace h2v