from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class WebbridgeTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires(self.tested_reference_str)

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        # Intentionally not run: `example` would need the WebView2 runtime
        # were it to construct a webview - build success is the test here.
        pass
