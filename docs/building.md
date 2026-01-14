# Building binja-lua

This guide covers building the plugin from source.

## Prerequisites

- **CMake** 3.24 or newer
- **C++17 compiler** (Clang recommended, MSVC and GCC also work)
- **Binary Ninja** with the API headers (included as a git submodule)

Lua 5.4 and sol2 are fetched automatically during the CMake configure step.

## Quick Build

```bash
# Clone with submodules
git clone --recursive https://github.com/alecnunn/binja-lua.git
cd binja-lua

# Configure and build
cmake -B build
cmake --build build --config Release

# Install to Binary Ninja plugins folder
cmake --install build --config Release
```

## Platform-Specific Notes

### Windows

Use the Visual Studio developer command prompt or ensure `cl.exe` is in your PATH. Alternatively, use Clang:

```bash
cmake -B build -G "Ninja Multi-Config" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build --config Release
```

The plugin will be installed to `%APPDATA%\Binary Ninja\plugins\`.

### Linux

```bash
cmake -B build
cmake --build build --config Release
cmake --install build --config Release
```

The plugin will be installed to `~/.binaryninja/plugins/`.

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build type (Debug, Release, RelWithDebInfo) |
| `CMAKE_INSTALL_PREFIX` | (auto) | Override the plugin install location |

## Debug Builds

For debugging, use the Debug configuration:

```bash
cmake --build build --config Debug
cmake --install build --config Debug
```

Debug builds include additional logging and assertions.

## Troubleshooting

### Missing submodule

If you see errors about missing Binary Ninja headers:

```bash
git submodule update --init --recursive
```

### CMake can't find Binary Ninja

The build expects the `binaryninja-api` submodule in the project root. If you have Binary Ninja installed elsewhere, you may need to set `BN_API_PATH`:

```bash
cmake -B build -DBN_API_PATH=/path/to/binaryninja-api
```

### Plugin doesn't load

1. Check Binary Ninja's log for errors (View > Log)
2. Ensure you're using a compatible Binary Ninja version
3. Verify the plugin was copied to the correct plugins directory
4. On macOS, you may need to remove quarantine: `xattr -r -d com.apple.quarantine BinjaLua.dylib`
