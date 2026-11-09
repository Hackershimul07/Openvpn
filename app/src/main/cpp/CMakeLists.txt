cmake_minimum_required(VERSION 3.18)
project(ovpnbridge CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(TP ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(O3 ${TP}/openvpn3)

set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "" FORCE)
add_subdirectory(${TP}/mbedtls mbedtls EXCLUDE_FROM_ALL)
add_subdirectory(${TP}/lz4/build/cmake lz4 EXCLUDE_FROM_ALL)

add_library(ovpnbridge SHARED native-lib.cpp ${O3}/client/ovpncli.cpp)
# extra openvpn3 translation units needed at link time (DataChannelEpoch etc.)
foreach(f openvpn/crypto/data_epoch.cpp)
  if(EXISTS ${O3}/${f})
    target_sources(ovpnbridge PRIVATE ${O3}/${f})
  endif()
endforeach()
target_include_directories(ovpnbridge PRIVATE ${O3} ${O3}/client ${TP}/asio/asio/include
    ${TP}/mbedtls/include ${TP}/lz4/lib ${TP}/fmt/include)
target_compile_definitions(ovpnbridge PRIVATE
    USE_ASIO ASIO_STANDALONE ASIO_NO_DEPRECATED ASIO_HAS_STD_ARRAY ASIO_HAS_STD_CHRONO
    FMT_HEADER_ONLY USE_MBEDTLS HAVE_LZ4)
target_link_libraries(ovpnbridge mbedtls mbedx509 mbedcrypto lz4_static log)
