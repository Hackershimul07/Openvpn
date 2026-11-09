Extract over your OvpnApp project (keeps same paths).
You also need app/src/main/cpp/native-lib.cpp (already in your project).
PC/Linux:  export ANDROID_NDK_HOME=/path/to/ndk && ./build_native.sh
GitHub:    push project, Actions tab > build-native > download "jniLibs" artifact
Copy result to app/src/main/jniLibs/ (arm64-v8a, armeabi-v7a).
