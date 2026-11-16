# webbridge-runtime

This repo is part of a project called webbridge. You can find its documentation on [`ReadTheDocs`](https://webbridge-docs.readthedocs.io/en/latest/index.html).


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
[`webbridge-tools`](https://github.com/WebBridge-Project/webbridge-tools) — it's not
bundled here, so consuming this library also means installing that package.

### Prerequisites

- **Visual Studio 2022** with C++ Desktop Development (MSVC compiler)
- **Conan 2** and **CMake 3.26+**
- **Python 3** with [`webbridge-tools`](https://github.com/WebBridge-Project/webbridge-tools) installed (`pip install webbridge-tools`) — provides the code generator and its `webbridge_generate()` CMake function
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
    self.requires("webbridge/1.0.0")

def configure(self):
    self.options["webbridge"].with_webview = True
```

`with_webview` defaults to `False`. The library's core (`property<T>`,
`event<Args...>`, error handling) builds without it on any OS, but the
WebView2-based JavaScript bridge (`register_type()`, `webbridge_generate()`)
needs `with_webview=True`, which is Windows-only.

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

To check whether a change you made to this repository broke anything, rebuild the package from source and let `test_package` (a minimal consumer exercising the `property<T>`/`event<...>` API) verify it without webview (false by default):

```bash
conan create . --build=missing                      # Release
conan create . -s build_type=Debug --build=missing  # Debug
```

How to test it with webview activated:

```bash
conan create . -o with_webview=True --build=missing                         # Release
conan create . -s build_type=Debug -o with_webview=True --build=missing     # Debug
```

Choose one of the 4 versions and run their matching `build_type`:

```bash
test_package\build\msvc-194-x86_64-14-release\Release\example.exe   # Release
test_package\build\msvc-194-x86_64-14-debug\Debug\example.exe       # Debug
```

which should print `Hello, Conan!`.

This intentionally doesn't call `webbridge_generate()` or launch a GUI window: `webbridge_generate()` isn't part of this repository at all anymore — it lives in the separate [`webbridge-tools`](https://github.com/WebBridge-Project/webbridge-tools) pip package, which has its own tests for the code generator. And a `webview::webview` window needs the WebView2 runtime and a message loop, which doesn't fit an unattended build environment regardless.

If you've already built once and only changed something under `test_package/`, `conan test test_package webbridge/1.0.0` reruns just that check without rebuilding `webbridge` itself.


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
