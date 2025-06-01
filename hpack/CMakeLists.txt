cmake_minimum_required(VERSION 3.5)
project(h2v-hpack CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Default to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build (Debug, Release, RelWithDebInfo)" FORCE)
endif()

# LTO option (only enabled in Release)
option(HPAC_ENABLE_LTO "Enable Link Time Optimization" ON)

# if(HPAC_ENABLE_LTO)
#   include(CheckIPOSupported)
#   check_ipo_supported(RESULT ipo_supported)
#   if(ipo_supported)
#     # Only apply LTO in Release builds
#     set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
#   endif()
# endif()

# Vendor dependencies
# add from project root
# add_subdirectory(third_party/abseil-cpp)
# add_subdirectory(third_party/highway)

# Our library
add_library(${PROJECT_NAME}
  src/h2v/hpack/dynamic_table.cc
  src/h2v/hpack/generated/huffman_byte_table_full.cc
  src/h2v/hpack/huffman_codec.cc
  # src/h2v/hpack/hpack.cc
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    absl::config
    absl::core_headers
    absl::dynamic_annotations
    absl::type_traits
    absl::base
    absl::strings
    absl::node_hash_map
    absl::hash
    absl::status       
    absl::statusor 
    absl::time        
    absl::synchronization
    h2v::base       
)
target_compile_definitions(${PROJECT_NAME} PUBLIC HPAC_STATIC_TABLE_SIZE=61)

# Per-build-type compile flags
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Debug>:-Og -g>
  $<$<CONFIG:Release>:-O3 -march=native>
  $<$<CONFIG:RelWithDebInfo>:-O3 -march=native>
)

# lib alias
add_library(h2v::hpack ALIAS ${PROJECT_NAME} )

# Example executable
# add_executable(hpac_example examples/hpack_simple_encode_decode.cc)
# target_link_libraries(hpac_example 
#   PRIVATE   
#     h2v::hpack)
# target_compile_options(hpac_example PRIVATE
#   $<$<CONFIG:Debug>:-Og -g>
#   $<$<CONFIG:Release>:-O3 -march=native>
#   $<$<CONFIG:RelWithDebInfo>:-O3 -march=native>
# )

add_executable(h2v_example_huffman_roundtrip
  examples/huffman_codec_roundtrip_main.cc
)
target_link_libraries(h2v_example_huffman_roundtrip
  PRIVATE
    absl::config
    absl::core_headers
    absl::dynamic_annotations
    absl::type_traits
    absl::base
    absl::strings
    absl::node_hash_map
    absl::hash
    absl::status       
    absl::statusor 
    absl::time        
    absl::synchronization
    h2v::base  
    h2v::hpack)



add_executable(h2v_huffman_gen_v2
  src/h2v/hpack/codegen/huffman_table_gen_v2_main.cc)
target_include_directories(h2v_huffman_gen_v2
  PRIVATE
    src/h2v/codegen
)
target_link_libraries(h2v_huffman_gen_v2
  PRIVATE
    h2v::base
)

target_compile_options(h2v_huffman_gen_v2 PRIVATE
  $<$<CONFIG:Debug>:-Og -g>
  $<$<CONFIG:Release>:-O3 -march=native>
  $<$<CONFIG:RelWithDebInfo>:-O3 -march=native>
)



# Tests (Catch2 vendored under third_party/Catch2)
# add_executable(hpack_tests
#   tests/static_table_test.cc
#   tests/huffman_codec_test.cc
#   tests/static_table_test.cc
#   tests/dynamic_table_test.cc
#   tests/codec_integration_test.cc
# )
# target_link_libraries(hpack_tests
#   PRIVATE
#     Catch2::Catch2WithMain
#     absl::config
#     absl::core_headers
#     absl::dynamic_annotations
#     absl::type_traits
#     absl::base
#     absl::strings
#     absl::node_hash_map
#     absl::hash
#     absl::status       
#     absl::statusor 
#     absl::time        
#     absl::synchronization   
#     h2v::hpack
# )
# target_compile_options(hpack_tests PRIVATE
#   $<$<CONFIG:Debug>:-Og -g>
#   $<$<CONFIG:Release>:-O3 -march=native>
#   $<$<CONFIG:RelWithDebInfo>:-O3 -march=native>
# )

# # Install rules
# install(TARGETS ${PROJECT_NAME}
#         EXPORT ${PROJECT_NAME}Targets
#         LIBRARY DESTINATION lib
#         ARCHIVE DESTINATION lib
#         RUNTIME DESTINATION bin)
# install(DIRECTORY include/${PROJECT_NAME}
#         DESTINATION include)

# export(EXPORT ${PROJECT_NAME}Targets
#        FILE "${CMAKE_CURRENT_BINARY_DIR}/hpacTargets.cmake"
#        NAMESPACE h2v::hpack::)
