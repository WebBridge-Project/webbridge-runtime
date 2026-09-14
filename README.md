# C++ to JavaScript Bridge [webbridge]

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
- `cmake/webbridge.cmake` — the `webbridge_generate()` CMake function that drives the code generator, and the `FetchContent`/vendoring wiring for `webview`
- `tools/` — the Python/tree-sitter code generator
- `conanfile.py` — the Conan 2 recipe for this package
- `conandata.yml` — pinned URLs/checksums for the vendored `webview` and Microsoft WebView2 SDK sources (downloaded in `source()`)
- `test_package/` — the minimal consumer Conan builds via `conan create` to verify the package

## Getting started

This package is Conan-only. `nlohmann_json` is resolved as a regular Conan
dependency. `webview` has no ConanCenter recipe, 
so it can't be a real Conan dependency either — instead,
`source()` downloads both into`vendor/`, 
which is also bundled inside the built package. `cmake/webbridge.cmake`
then points CMake's `FetchContent` at those vendored copies instead of the
network, for this package's own build *and* automatically for every project
that consumes it — no live `git clone`/download happens during `build()`,
which is required for ConanCenter's network-sandboxed CI.

### Prerequisites

- **Visual Studio 2022** with C++ Desktop Development (MSVC compiler)
- **Conan 2** and **CMake 3.26+**
- **Python 3** (for the code generator; a private venv with its dependencies is provisioned automatically on first use)
- **Microsoft Edge WebView2 Runtime** (usually preinstalled on Windows 10/11)

### Using it in your own C++ project

In your `conanfile.py`:

```python
def requirements(self):
    self.requires("webbridge/1.0.0")
```

In your `CMakeLists.txt`:

```cmake
find_package(webbridge REQUIRED CONFIG)

target_compile_features(your_target PRIVATE cxx_std_20)
target_link_libraries(your_target PRIVATE webbridge::webbridge)

webbridge_generate(
    TARGET your_target
    AUTO
    LANGUAGE cpp
)
```

Replace your_target with the name of your own CMake target.

In your project:
1. Write a class that inherits from `webbridge::object` — see [Minimal Example](#minimal-example) below for what this looks like.

2. Register it where you create your webview window:
```cpp
webbridge::register_type<YourClass>(&your_webview);
```

### Building the package locally:

```bash
conan create . --build=missing
```

This builds `webbridge`, packages it into your local Conan cache, and builds/verifies `test_package` against the packaged artifacts.

Choose a build variant:

```bash
conan create . --build=missing                      # Release (default)
conan create . -s build_type=Debug --build=missing  # Debug
```

Both can be in your local cache side by side. `cmake_layout()` also keeps their build folders separate (e.g. `test_package/build/msvc-194-x86_64-14-release/` vs `.../14-debug/`), so building one never clobbers the other.

### Testing the package

`test_package/` is a minimal consumer that proves the packaged headers and static library work correctly together. It's built and run automatically as part of `conan create .` above.

To re-run just that check against an already-built `webbridge` package in your local cache:

```bash
conan test test_package webbridge/1.0.0
```

This intentionally doesn't call `webbridge_generate()` or launch a GUI window: the code generator needs a `pip install`-able Python venv, and a `webview::webview` window needs the WebView2 runtime and a message loop — neither fits ConanCenter's network-sandboxed, unattended build/test environment. Instead it directly exercises the plain C++ `property<T>`/`event<...>` API. To manually verify the code generator itself against the packaged `tools/` (not covered by the automated test), add a `webbridge_generate(TARGET example AUTO LANGUAGE cpp)` call to `test_package/CMakeLists.txt` locally and rebuild — that's how this was last verified by hand.

To actually see the automated test run (adjust the path for whichever `build_type` you built, see above):

```bash
# cmd.exe - Release
test_package\build\msvc-194-x86_64-14-release\Release\example.exe

# cmd.exe - Debug
test_package\build\msvc-194-x86_64-14-debug\Debug\example.exe
```

which should print `Hello, Conan!`.


## Concepts

Every class to be exposed to the web must inherit from `webbridge::object`. The API is inspired by Qt and provides the following mechanisms for JavaScript integration:

* **Methods** – Public C++ methods are automatically available in JavaScript (similar to Qt's Q_INVOKABLE)
* **Properties** – Exposed as Svelte-compatible stores (read-only, inspired by Qt's Q_PROPERTY)
* **Events** – Trigger custom event listeners in JavaScript (equivalent to Qt signals)
* **Constants** - Readonly JS values

The automatically generated code is functionally inspired by Qt and Qt's MOC (Meta-Object Compiler), but the WebBridge classes require significantly less boilerplate code than their Qt equivalents.

### Methods

All public methods of a `webbridge::object` class are automatically published to JavaScript.

A function marked with the `[[async]]` attribute is executed in a separate worker thread. This prevents blocking the main thread on the C++ side. On the JavaScript side, both synchronous and asynchronous methods always return a `Promise` and never block the main thread.

### Properties

Properties are similar to primitive data types but require access via the parenthesis operator `()` in C++. In JavaScript, properties are exposed as Svelte-compatible, reactive stores. They are read-only in JavaScript; changes to the property value in C++ are automatically and immediately propagated to JavaScript.

### Events

Events are the WebBridge equivalent of the Qt signal/slot mechanism.

### Constants

WebBridge supports exposing constants as both **static** (class-wide) and **non-static** (instance-specific). Both variants are automatically exported to JavaScript and are available there as read-only values.

### Error Handling

WebBridge implements robust error handling, distinguishing between JavaScript client errors (4xxx) and C++ server errors (5xxx). Errors are serialized as JSON objects and, for asynchronous operations, are propagated as rejected Promises.

**Error format:**
```json
{
  "error": {
    "code": 4001,
    "message": "Invalid argument type",
    "details": { "param": "value", "expected": "string" },
    "stack": "at function (file.js:10:5)",
    "origin": "javascript"
  }
}
```

**Error codes:**
- `4000-4999`: JavaScript errors (e.g., 4001 = JSON_PARSE_ERROR during parameter deserialization)
- `5000-5999`: C++ errors (e.g., 5000 = RUNTIME_ERROR during runtime errors)

Inspired by JSON-RPC 2.0, GraphQL, and HTTP status codes. Promises are automatically rejected on error, enabling clean exception handling with async/await syntax.

## Minimal Example

The following example shows how to define a C++ class with methods, properties, and events for web integration with WebBridge.

```cpp
#include "webbridge/object.h"

class MyObject : public webbridge::object
{
public:
    property<bool> aBool = false;
    property<std::string> strProp;
    event<int, bool> aEvent;
    inline static constexpr auto PI = 3.141592654;
    const std::string version = "1.0";

public:
    [[async]] void foo(std::string_view val) {
        // long-running action
        strProp = val;
        aEvent.emit(42, false);
    }

    bool bar() const {
        // Parenthesis operator accesses value
        return !aBool();
    }
};
```

### Tracking JavaScript Properties

```js
const myObj = await MyObject.create();
// ...
myObj.aBool.subscribe(value => {
    console.log('aBool updated:', value);
});
```

### Calling a C++ Method from JavaScript

```js
const myObj = await MyObject.create();

// Example: call a synchronous method
// Blocks the main thread in C++, but JavaScript waits asynchronously
const result = await myObj.bar();
console.log('Result of bar():', result);

// Example: call an asynchronous method ([[async]] = worker thread in C++)
// Does not block the main thread in either C++ or JavaScript
myObj.foo('new value').then(() => {
    console.log('foo() completed');
});
```

### Handling Events in JavaScript

```js
const myObj = await MyObject.create();

// Register event listener (similar to Node.js EventEmitter)
myObj.aEvent.on((intValue, boolValue) => {
    console.log('Event received:', intValue, boolValue);
});

// Alternatively, one-time event
myObj.aEvent.once((intValue, boolValue) => {
    console.log('One-time event:', intValue, boolValue);
});
```

### Accessing Constants in JavaScript

```js
const myObj = await MyObject.create();
console.log(myObj.version); // Instance constant: "1.0"
console.log(MyObject.PI);   // Static constant: 3.141592654
```

## Registration

To make C++ classes available in JavaScript, they must be explicitly registered. The code generator creates the necessary binding files, which are then included in CMake. In your `main.cpp`, you must call the generated registration function:

```cpp
#include "MyObject_registration.h"

int main() {
    // Register the class for JavaScript
    webbridge::register_type<MyObject>();

    // ... initialize and run your webview ...
}
```

## Known Limitations

The current implementation has the following limitations:

- Overloaded constructors and methods are not supported.
- Enums are automatically detected and exported to TypeScript, but complex enum use cases may require additional handling.
- Currently Windows-only (relies on Microsoft WebView2).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

Third-party licenses can be found in [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt).
