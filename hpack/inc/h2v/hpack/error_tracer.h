#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace h2v {
namespace hpack {

/// @brief Signature for an error callback hook.
/// @param stream_id     HTTP/2 stream identifier where the error occurred.
/// @param code          32-bit internal domain+subcode error code.
/// @param message       Human-readable diagnostic message.
using ErrorCallback = std::function<void(uint32_t stream_id, uint32_t code,
                                         const std::string& message)>;

/// @brief Internal function to compose a 32-bit error code.
/// @param domain   high-16 bits (e.g. 0x0001 for HPACK/COMPRESSION_ERROR)
/// @param subcode  low-16 bits (library-specific subcode)
/// @return combined 32-bit code.
constexpr uint32_t make_error(uint16_t domain, uint16_t subcode) noexcept {
  return (static_cast<uint32_t>(domain) << 16) | subcode;
}

/// @brief Internal helper to embed an error code into a formatted message.
/// @param code     32-bit internal error code.
/// @param message  original error description.
/// @return string of the form "[0xDDSS] message", where DD=domain, SS=subcode.
inline std::string error_message(uint32_t code, const std::string& message) {
  char buf[32];
  // Format code as 0xhhhhhhhh
  std::snprintf(buf, sizeof(buf), "[0x%08X] ", code);
  return std::string(buf) + message;
}

/// @brief Global error callback registration for diagnostic hooks.
/// @details
///   Library users can set a callback to be invoked on every encode/decode
///   error. The callback receives the stream ID, internal error code, and
///   message.
/// @param cb  callable to invoke on errors; passing nullptr clears the hook.
// void SetErrorCallback(ErrorCallback cb) noexcept;

// /// @brief Retrieve the current error callback (or nullptr).
// ErrorCallback GetErrorCallback() noexcept;



static ErrorCallback g_error_cb = nullptr;

static void SetErrorCallback(ErrorCallback cb) noexcept {
  g_error_cb = std::move(cb);
}

static ErrorCallback GetErrorCallback() noexcept {
  return g_error_cb;
}
}  // namespace hpack
}  // namespace h2v