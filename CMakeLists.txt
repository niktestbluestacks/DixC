cmake_minimum_required(VERSION 3.21)
project(DIXC CXX C)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)

# if(NOT LLVM_ENABLE_RTTI)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
# endif()
# add_compile_definitions(LLVM_ENABLE_EH false)
# if(NOT LLVM_ENABLE_EH)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
# endif()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

file(GLOB_RECURSE COMPILER_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler/*.cpp"
)

file(GLOB_RECURSE STDLIB_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/stdlib/*.cpp"
)

set(ALL_SOURCES 
    src/main.cpp 
    ${COMPILER_SOURCES} 
    ${STDLIB_SOURCES}
)

add_executable(DIXC ${ALL_SOURCES})

target_include_directories(DIXC PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/compiler
    ${CMAKE_CURRENT_SOURCE_DIR}/stdlib
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(DIXC PRIVATE ${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs
    core
    support
    irreader
)

target_link_libraries(DIXC PRIVATE ${llvm_libs})

set_target_properties(DIXC PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
