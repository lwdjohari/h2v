cmake_minimum_required(VERSION 3.5)
project(h2v-base)

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} 
    INTERFACE 
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
        yaml-cpp
)

target_include_directories(${PROJECT_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
# target_compile_definitions(headerlib INTERFACE SOMETHING=1)

# lib alias
add_library(h2v::base ALIAS ${PROJECT_NAME} )
