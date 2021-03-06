cmake_minimum_required(VERSION 2.8)

project(onions-client)

find_package(Boost COMPONENTS system REQUIRED)

#compiler options for Clang and GCC/G++
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-g -O3 -std=c++11 -Wno-c++98-compat-pedantic -pedantic -Weverything -Wno-exit-time-destructors -Wno-weak-vtables -Wno-documentation -Wno-documentation-unknown-command -Wno-reserved-id-macro -Wno-missing-noreturn -Wno-padded")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "-g -O3 -std=c++11 -Wall -Wextra -pedantic -Wdouble-promotion -Wfloat-equal -Wunsafe-loop-optimizations")
endif()

include_directories(${CMAKE_INSTALL_PREFIX}/include/onions-common /usr/include/botan-1.10)

add_definitions(-DINSTALL_PREFIX=std::string\("${CMAKE_INSTALL_PREFIX}"\))

add_executable(onions-client
  main.cpp
  Client.cpp

  tcp/IPC.cpp
  tcp/IPCSession.cpp
)

add_executable(onions-tbb
  tbb-mitm/TorWatch.cpp
  tbb-mitm/ProcessWatch.cpp
)

add_dependencies(onions-client onions-tbb)

FIND_LIBRARY(onions-common NAMES "onions-common" REQUIRED
  PATHS ${CMAKE_INSTALL_PREFIX}/lib/onions-common NO_DEFAULT_PATH)
FIND_LIBRARY(onions-cyoencode NAMES "onions-cyoencode" REQUIRED
  PATHS ${CMAKE_INSTALL_PREFIX}/lib/onions-common NO_DEFAULT_PATH)
FIND_LIBRARY(onions-jsoncpp NAMES "onions-jsoncpp" REQUIRED
  PATHS ${CMAKE_INSTALL_PREFIX}/lib/onions-common NO_DEFAULT_PATH)
FIND_LIBRARY(onions-scrypt NAMES "libscrypt.so.0" REQUIRED
  PATHS ${CMAKE_INSTALL_PREFIX}/lib/onions-common NO_DEFAULT_PATH)

if (${onions-common} MATCHES ".*-NOTFOUND")
  message(FATAL_ERROR "OnioNS-common could not be found! Please install it.")
else()
  message(STATUS "Found OnioNS-common library")
endif()

if (${onions-cyoencode} MATCHES ".*-NOTFOUND")
  message(FATAL_ERROR "OnioNS-common's CyoEncode component was not found!")
else()
  message(STATUS "Found OnioNS-common's CyoEncode component")
endif()

if (${onions-jsoncpp} MATCHES ".*-NOTFOUND")
  message(FATAL_ERROR "OnioNS-common's jsoncpp component was not found!")
else()
  message(STATUS "Found OnioNS-common's JsonCpp component")
endif()

if (${onions-scrypt} MATCHES ".*-NOTFOUND")
  message(FATAL_ERROR "OnioNS-common's libscrypt component was not found!")
else()
  message(STATUS "Found OnioNS-common's libscrypt component")
endif()

#http://www.cmake.org/Wiki/CMake_RPATH_handling
SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib/onions-common)
message(STATUS ${CMAKE_INSTALL_PREFIX}/lib/onions-common/)

target_link_libraries(onions-tbb    ${onions-common} ${onions-cyoencode}
  ${onions-jsoncpp} ${onions-scrypt} popt botan-1.10 pthread
  ${Boost_SYSTEM_LIBRARY} ${Boost_LIBRARIES})
target_link_libraries(onions-client ${onions-common} ${onions-cyoencode}
  ${onions-jsoncpp} ${onions-scrypt} popt botan-1.10 pthread
  ${Boost_SYSTEM_LIBRARY} ${Boost_LIBRARIES})

#installation of files
set(ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
install(TARGETS onions-client               DESTINATION bin/)
install(TARGETS onions-tbb                  DESTINATION bin/)
install(FILES ${ASSETS}/onions-stem.py      DESTINATION bin/)
install(FILES ${ASSETS}/onions-client.1.gz  DESTINATION share/man/man1/)
