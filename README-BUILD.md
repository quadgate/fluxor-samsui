# Native Build Scripts

This directory contains scripts to build the native C++ library separately from the full Android project build.

## Available Scripts

### Windows
- **`build-native.bat`** - Full build (includes APK packaging)
- **`build-native-only.bat`** - Native library only (faster, no APK)

### Linux/Mac
- **`build-native.sh`** - Full build (includes APK packaging)
- **`build-native-only.sh`** - Native library only (faster, no APK)

## Usage

### Windows

#### Full Build (with APK):
```cmd
build-native.bat [clean] [debug|release]
```

#### Native Library Only (faster):
```cmd
build-native-only.bat [clean] [debug|release]
```

### Linux/Mac

#### Full Build (with APK):
```bash
chmod +x build-native.sh
./build-native.sh [clean] [debug|release]
```

#### Native Library Only (faster):
```bash
chmod +x build-native-only.sh
./build-native-only.sh [clean] [debug|release]
```

## Examples

### Build debug version:
```bash
# Windows
build-native.bat debug

# Linux/Mac
./build-native.sh debug
```

### Clean and build release:
```bash
# Windows
build-native.bat clean release

# Linux/Mac
./build-native.sh clean release
```

### Build native library only (faster):
```bash
# Windows
build-native-only.bat debug

# Linux/Mac
./build-native-only.sh debug
```

## Build Output

The native library (.so files) will be located in:
- **Debug**: `app/build/intermediates/cmake/debug/obj/`
- **Release**: `app/build/intermediates/cmake/release/obj/`

Each architecture will have its own directory (e.g., `arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`).

## Notes

- The scripts use Gradle to build the native code, which requires the Android SDK and NDK
- Make sure you have `local.properties` configured with your Android SDK path
- The `-x lint` flag skips lint checks to speed up the build
- Native-only builds are faster as they skip Kotlin compilation and APK packaging

## Troubleshooting

### Build fails with "NDK not found"
- Make sure Android NDK is installed via Android Studio SDK Manager
- Verify `local.properties` contains `ndk.dir` or `sdk.dir`

### Permission denied (Linux/Mac)
- Run: `chmod +x build-native*.sh`

### CMake errors
- Clean build: `build-native.bat clean` or `./build-native.sh clean`
- Verify CMake 3.22.1+ is installed
