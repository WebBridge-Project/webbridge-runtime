# webbridge-runtime

C++ objects are seamlessly integrated into modern web applications as a modern **Qt alternative** for web-based UIs. While Qt uses QML/Qt Quick or Qt WebEngine for GUI development, WebBridge leverages standard web technologies.

**Motivation:** A key advantage over Qt is the significantly reduced boilerplate code. In Qt, a simple property requires extensive boilerplate with getter, setter, signal, and backing field:

```cpp
// Qt approach - verbose boilerplate
Q_PROPERTY(bool aBool READ aBool WRITE setABool NOTIFY aBoolChanged)
bool aBool() const {
    return _aBool;
}
void setABool(bool v) {
    if (v != _aBool) {
        _aBool = v;
        emit aBoolChanged();
    }
}
signals:
    void aBoolChanged();
private:
    bool _aBool;

// WebBridge approach - minimal and clean
property<bool> aBool;
```

The solution is based on **webview** (C++ wrapper for Microsoft WebView2/Chromium) and a **Python code generator** (`tools/generate.py`) that uses **tree-sitter** to analyze C++ classes and automatically generate C++ registration headers and TypeScript type definitions. The build process automatically invokes the code generator via CMake, making the workflow seamless. Code generation is required because C++26 reflection is not yet available.

## Repository layout

- `src/webbridge/` — the library itself
- `cmake/webbridge-webview.cmake` — the `FetchContent`/vendoring wiring for the `webview` C++ dependency
- `conanfile.py` — the Conan 2 recipe for this package
- `conandata.yml` — pinned URLs/checksums for the vendored `webview` and Microsoft WebView2 SDK sources
- `test_package/` — the minimal consumer Conan builds via `conan create` to verify the package

## Getting started

This package is Conan-only and covers the C++ library only. `nlohmann_json`
is resolved as a regular Conan dependency. `webview` has no ConanCenter
recipe, so it can't be a real Conan dependency either — instead, `source()`
downloads both into `vendor/`, which is also bundled inside the built
package. `cmake/webbridge-webview.cmake` then points CMake's `FetchContent`
at those vendored copies instead of the network, for this package's own
build *and* automatically for every project that consumes it — no live
`git clone`/download happens during `build()`, which keeps builds offline
and reproducible.

The code generator is a separate, pip-installable package,
[`webbridge-tools`](https://github.com/fsbondtec/webbridge-tools) — it's not
bundled here, so consuming this library also means installing that package.

### Prerequisites

- **Visual Studio 2022** with C++ Desktop Development (MSVC compiler)
- **Conan 2** and **CMake 3.26+**
- **Python 3** with [`webbridge-tools`](https://github.com/fsbondtec/webbridge-tools) installed (`pip install webbridge-tools`) — provides the code generator and its `webbridge_generate()` CMake function
- **Microsoft Edge WebView2 Runtime** (usually preinstalled on Windows 10/11)


### 0. Add our Conan server as a remote

`webbridge` is published on our internal Conan server, not ConanCenter, so consumers need to add it as a remote (and log in, since it isn't anonymous) before `conan install`/`conan create` can find the package:

```bash
conan remote add --insecure --index 0 fsb http://<server-host>:9300
conan remote login fsb <username> -p <password>
```

`--index 0` gives our server priority over `conancenter` when resolving `webbridge` itself. Other dependencies such as `nlohmann_json` are unaffected and still resolve from `conancenter` as usual.

### 1. In your `conanfile.py`

```python
def requirements(self):
    self.requires("webbridge/0.1.0")
```

### 2. Wire it into your own `CMakeLists.txt`

**`webbridge` itself** (this Conan package — the C++ library):

```cmake
find_package(webbridge REQUIRED CONFIG)

target_compile_features(your_target PRIVATE cxx_std_20)
target_link_libraries(your_target PRIVATE webbridge::webbridge)
```

**`webbridge-tools`** — the code generator. It's not a Conan dependency, so it isn't found by `find_package(webbridge)` above. So point CMake at it explicitly instead:

```cmake
find_package(Python REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND ${Python_EXECUTABLE} -m webbridge_tools --cmake-dir
    OUTPUT_VARIABLE WEBBRIDGE_TOOLS_CMAKE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
list(APPEND CMAKE_MODULE_PATH "${WEBBRIDGE_TOOLS_CMAKE_DIR}")
include(webbridge)

webbridge_generate(
    TARGET your_target
    AUTO
    LANGUAGE cpp
)
```

Replace your_target with the name of your own CMake target.

### 3. Write and register your class
1. Write a class that inherits from `webbridge::object` — see [Minimal Example](#minimal-example) below for what this looks like.

2. Register it where you create your webview window:
```cpp
webbridge::register_type<YourClass>(&your_webview);
```

### 4. Build and run your own project

From your project's root choose a build variant:

```bash
conan install . --output-folder=build --build=missing -s  build_type=Debug  # Debug
conan install . --output-folder=build --build=missing                       # Release (default)
```
Then
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake
```
Finally, choose the same build variant again:
```bash
cmake --build build --config Debug                  # Debug
cmake --build build --config Release                # Release (default)
cmake --build build --config Release --clean-first  # Clean rebuild
```

And executing with:
```bash
# Debug build (with DevTools):
build\Debug\your_target.exe
# Release build (without DevTools):
build\Release\your_target.exe
```

## Testing your changes locally

To check whether a change you made to this repository broke anything, rebuild the package from source and let `test_package` (a minimal consumer exercising the `property<T>`/`event<...>` API) verify it:

```bash
conan create . --build=missing                      # Release
conan create . -s build_type=Debug --build=missing  # Debug
```

If this succeeds, the packaged headers, static library, and CMake wiring (`cmake/webbridge-webview.cmake`) all still work together. To see it actually run (adjust the path if you built Debug):

```bash
test_package\build\msvc-194-x86_64-14-release\Release\example.exe   # Release
test_package\build\msvc-194-x86_64-14-debug\Debug\example.exe       # Debug
```

which should print `Hello, Conan!`.

This intentionally doesn't call `webbridge_generate()` or launch a GUI window: `webbridge_generate()` isn't part of this repository at all anymore — it lives in the separate [`webbridge-tools`](https://github.com/fsbondtec/webbridge-tools) pip package, which has its own tests for the code generator. And a `webview::webview` window needs the WebView2 runtime and a message loop, which doesn't fit an unattended build environment regardless.

If you've already built once and only changed something under `test_package/`, `conan test test_package webbridge/0.1.0` reruns just that check without rebuilding `webbridge` itself.


## Publishing a release to the private Conan server

1. Bump the version in three places: `version` in [conanfile.py](conanfile.py), `project(... VERSION ...)` in [CMakeLists.txt](CMakeLists.txt), and the `sources` key in [conandata.yml](conandata.yml).
2. Build and test both configurations locally:
   ```bash
   conan create . --build=missing                      # Release
   conan create . -s build_type=Debug --build=missing  # Debug
   ```
   Build any other profiles the team relies on too, e.g. `-s compiler.runtime=static`.
3. Upload the new version:
   ```bash
   conan upload "webbridge/<version>" -r fsb --confirm
   ```
4. Tag the commit as `<version>`.

Consumers who don't have a matching prebuilt binary can still fall back to `--build=missing`, but that needs internet access since `source()` downloads `webview` and the WebView2 SDK.


## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

Third-party licenses can be found in [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt).
