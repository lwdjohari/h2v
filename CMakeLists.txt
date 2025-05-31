cmake_minimum_required(VERSION 3.5)
project(h2v LANGUAGES CXX)

option(H2V_USE_CATCH ON)
option(H2V_USE_TEST ON)
option(H2V_BUNDLE_ABSEIL ON)

set(H2V_USE_CATCH ON)
set(H2V_USE_TEST ON)
set(H2V_BUNDLE_ABSEIL ON)
# -----------------------
# ✅ Project Config
# -----------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------
# ✅ Catch2 Lib
# -----------------------
if(H2V_USE_CATCH)
 message(STATUS  "CATCH2::ADD_LIB.")
 find_package(Catch2 3 REQUIRED)
endif()

# -----------------------
# ✅ Threads Lib
# -----------------------
find_package(Threads REQUIRED)

# -----------------------
# ✅ Google Abseil
# -----------------------

if(H2V_BUNDLE_ABSEIL)
    message(STATUS  "Vectro:    Abseil - Use from bundle.")
    set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "")
    add_subdirectory(third_party/abseil build-abseil)
endif()

# -----------------------
# ✅ Google Highway
# -----------------------
# Turn off building of Highway’s tests and examples
option(HWY_ENABLE_TESTS    OFF)
option(HWY_ENABLE_EXAMPLES OFF)
set(HWY_ENABLE_TESTS    OFF)
set(HWY_ENABLE_EXAMPLES OFF)

add_subdirectory(third_party/highway build-highway)

# -----------------------
# ✅ CLI11
# -----------------------
add_subdirectory(third_party/cli11 build-cli11)

# -----------------------
# ✅ yaml-cpp
# -----------------------
# yaml-cpp
add_subdirectory(third_party/yaml-cpp build-yaml)

# -----------------------
# ✅ h2v
# -----------------------
# configure h2v::base lib
add_subdirectory(h2v_shared build-h2v-base)
# configure h2v::hpack lib
add_subdirectory(hpack build-hpack)
# configure h2v

# if(H2V_USE_CATCH AND H2V_USE_TEST)
#     add_subdirectory(tests build-test)
# endif()
