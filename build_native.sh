#!/usr/bin/env bash
# Builds libovpnbridge.so for arm64-v8a + armeabi-v7a into app/src/main/jniLibs
# Needs: git, cmake>=3.18, ninja, Android NDK (ANDROID_NDK_HOME set), native-lib.cpp in app/src/main/cpp
set -euo pipefail
cd "$(dirname "$0")"
NDK="${ANDROID_NDK_HOME:?set ANDROID_NDK_HOME}"
CPP=app/src/main/cpp; TP=$CPP/third_party; mkdir -p $TP
[ -d $TP/openvpn3 ] || git clone --depth 1 https://github.com/OpenVPN/openvpn3 $TP/openvpn3
[ -d $TP/asio ]     || git clone --depth 1 https://github.com/chriskohlhoff/asio $TP/asio
[ -d $TP/lz4 ]      || git clone --depth 1 --branch v1.9.4 https://github.com/lz4/lz4 $TP/lz4
[ -d $TP/fmt ]      || git clone --depth 1 --branch 10.2.1 https://github.com/fmtlib/fmt $TP/fmt
[ -d $TP/mbedtls ]  || git clone --depth 1 --branch v3.6.0 --recurse-submodules --shallow-submodules https://github.com/Mbed-TLS/mbedtls $TP/mbedtls
for ABI in arm64-v8a armeabi-v7a; do
  B=build/$ABI; rm -rf $B
  cmake -S $CPP -B $B -GNinja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=$ABI -DANDROID_PLATFORM=android-24 -DANDROID_STL=c++_static \
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=$PWD/app/src/main/jniLibs/$ABI
  cmake --build $B --target ovpnbridge -j"$(nproc)"
done
ls -la app/src/main/jniLibs/*/
