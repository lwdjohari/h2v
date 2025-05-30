// include/hpack/static_table.h
#pragma once

#include <cstdint>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "h2v/hpack/header.h"

namespace h2v {
namespace hpack {

/// @brief RFC-7541 defined static header table.
/// @details
///   The static table contains a fixed set of common header fields
///   (e.g. ":method: GET", "content-type: text/html") with predefined
///   indices [1…N]. It never changes at runtime and requires no locking.
/// @see RFC 7541 §2.3 (“Static Table”)
class StaticTable {
 public:
  /// @brief Lookup a header by its static index.
  /// @param index  1-based index into the static table.
  /// @return the Header (name/value pair) if index ∈ [1, size], otherwise an
  /// empty optional.
  static absl::optional<Header> GetByIndex(uint32_t index) noexcept;

  /// @brief Find the static index for a given header name and optional value.
  /// @param name   header name to search for.
  /// @param value  header value to search for (empty ⇒ match name only).
  /// @return 1-based index if found; 0 if no match.
  static uint32_t FindIndex(absl::string_view name,
                            absl::string_view value = {}) noexcept;

  /// @brief Total number of entries in the static table.
  /// @return static table size (constant at compile time).
  static constexpr uint32_t Size() noexcept {
    return kTableSize;
  }

 private:
  // Underlying constexpr array of Header entries,
  // indexed [0…kTableSize-1] ⇒ logical indices [1…kTableSize].
  static constexpr uint32_t kTableSize = 61;  // N per RFC
                                              // clang-format off
  static constexpr Header kEntries[kTableSize] = {
    {":authority", ""},                         //  1: :authority
    {":method", "GET"},                         //  2: :method GET
    {":method", "POST"},                        //  3: :method POST
    {":path", "/"},                             //  4: :path /
    {":path", "/index.html"},                   //  5: :path /index.html
    {":scheme", "http"},                        //  6: :scheme http
    {":scheme", "https"},                       //  7: :scheme https
    {":status", "200"},                         //  8: :status 200
    {":status", "204"},                         //  9: :status 204
    {":status", "206"},                         // 10: :status 206
    {":status", "304"},                         // 11: :status 304
    {":status", "400"},                         // 12: :status 400
    {":status", "404"},                         // 13: :status 404
    {":status", "500"},                         // 14: :status 500
    {"accept-charset", ""},                     // 15: accept-charset
    {"accept-encoding", "gzip, deflate"},       // 16: accept-encoding gzip, deflate
    {"accept-language", ""},                    // 17: accept-language
    {"accept-ranges", ""},                      // 18: accept-ranges
    {"accept", ""},                             // 19: accept
    {"access-control-allow-origin", ""},        // 20: access-control-allow-origin
    {"age", ""},                                // 21: age
    {"allow", ""},                              // 22: allow
    {"authorization", ""},                      // 23: authorization
    {"cache-control", ""},                      // 24: cache-control
    {"content-disposition", ""},                // 25: content-disposition
    {"content-encoding", ""},                   // 26: content-encoding
    {"content-language", ""},                   // 27: content-language
    {"content-length", ""},                     // 28: content-length
    {"content-location", ""},                   // 29: content-location
    {"content-range", ""},                      // 30: content-range
    {"content-type", ""},                       // 31: content-type
    {"cookie", ""},                             // 32: cookie
    {"date", ""},                               // 33: date
    {"etag", ""},                               // 34: etag
    {"expect", ""},                             // 35: expect
    {"expires", ""},                            // 36: expires
    {"from", ""},                               // 37: from
    {"host", ""},                               // 38: host
    {"if-match", ""},                           // 39: if-match
    {"if-modified-since", ""},                  // 40: if-modified-since
    {"if-none-match", ""},                      // 41: if-none-match
    {"if-range", ""},                           // 42: if-range
    {"if-unmodified-since", ""},                // 43: if-unmodified-since
    {"last-modified", ""},                      // 44: last-modified
    {"link", ""},                               // 45: link
    {"location", ""},                           // 46: location
    {"max-forwards", ""},                       // 47: max-forwards
    {"proxy-authenticate", ""},                 // 48: proxy-authenticate
    {"proxy-authorization", ""},                // 49: proxy-authorization
    {"range", ""},                              // 50: range
    {"referer", ""},                            // 51: referer
    {"refresh", ""},                            // 52: refresh
    {"retry-after", ""},                        // 53: retry-after
    {"server", ""},                             // 54: server
    {"set-cookie", ""},                         // 55: set-cookie
    {"strict-transport-security", ""},          // 56: strict-transport-security
    {"transfer-encoding", ""},                  // 57: transfer-encoding
    {"user-agent", ""},                         // 58: user-agent
    {"vary", ""},                               // 59: vary
    {"via", ""},                                // 60: via
    {"www-authenticate", ""}                    // 61: www-authenticate
    };
// clang-format off
  };


inline absl::optional<hpack::Header> StaticTable::GetByIndex(uint32_t index) noexcept {
  if (index == 0 || index > kTableSize) {
    return absl::nullopt;
  }
  return kEntries[index - 1];
}

inline uint32_t StaticTable::FindIndex(absl::string_view name,
                                absl::string_view value) noexcept {
  for (uint32_t i = 0; i < kTableSize; ++i) {
    const Header& h = kEntries[i];
    if (h.name == name && (value.empty() || h.value == value)) {
      return i + 1;
    }
  }
  return 0;
}

}  // namespace hpack
}  // namespace h2v