// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Linggawasistha Djohari
// <linggawasistha.djohari@outlook.com>


#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace h2v {
namespace utils {
namespace cli {

class TerminalColor {
 public:
  TerminalColor() {
#ifdef _WIN32
    // Detect modern Windows terminals that support ANSI natively
    const char* wt = std::getenv("WT_SESSION");
    const char* vscode = std::getenv("TERM_PROGRAM");
    const char* conemu = std::getenv("ConEmuPID");
    const char* msys = std::getenv("MSYSTEM");

    if (wt || (vscode && std::string(vscode) == "vscode") || conemu || msys) {
      color_enabled_ = true;
    } else {
      HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
      if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
          dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
          if (SetConsoleMode(hOut, dwMode)) {
            color_enabled_ = true;
          }
        }
      }
    }
#else
    const char* term = std::getenv("TERM");
    color_enabled_ = term && std::string(term) != "dumb";
#endif
  }

  std::string Reset() const {
    return color_enabled_ ? "\033[0m" : "";
  }

  std::string Bold() const {
    return color_enabled_ ? "\033[1m" : "";
  }

  std::string Dim() const {
    return color_enabled_ ? "\033[2m" : "";
  }

  std::string Red() const {
    return color_enabled_ ? "\033[31m" : "";
  }

  std::string Green() const {
    return color_enabled_ ? "\033[32m" : "";
  }

  std::string Yellow() const {
    return color_enabled_ ? "\033[33m" : "";
  }

  std::string Blue() const {
    return color_enabled_ ? "\033[34m" : "";
  }

  std::string Magenta() const {
    return color_enabled_ ? "\033[35m" : "";
  }

  std::string Cyan() const {
    return color_enabled_ ? "\033[36m" : "";
  }

  std::string White() const {
    return color_enabled_ ? "\033[37m" : "";
  }

  std::string BrightRed() const {
    return color_enabled_ ? "\033[91m" : "";
  }
  std::string BrightGreen() const {
    return color_enabled_ ? "\033[92m" : "";
  }
  std::string BrightYellow() const {
    return color_enabled_ ? "\033[93m" : "";
  }
  std::string BrightBlue() const {
    return color_enabled_ ? "\033[94m" : "";
  }
  std::string BrightMagenta() const {
    return color_enabled_ ? "\033[95m" : "";
  }
  std::string BrightCyan() const {
    return color_enabled_ ? "\033[96m" : "";
  }
  std::string BrightWhite() const {
    return color_enabled_ ? "\033[97m" : "";
  }

  // Semantic helpers
  std::string Info(const std::string& msg) const {
    return Bold() + Cyan() + msg + Reset();
  }

  std::string Success(const std::string& msg) const {
    return Bold() + Green() + msg + Reset();
  }

  std::string Warn(const std::string& msg) const {
    return Bold() + Yellow() + msg + Reset();
  }

  std::string Error(const std::string& msg) const {
    return Bold() + Red() + msg + Reset();
  }

  std::string DimText(const std::string& msg) const {
    return Dim() + msg + Reset();
  }

 private:
  bool color_enabled_ = false;
};

inline TerminalColor terminal_color;

}  // namespace cli
}  // namespace utils
}  // namespace h2v