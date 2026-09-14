import os
from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy


class WebbridgeConan(ConanFile):
    name = "webbridge"
    version = "1.0.0"
    license = "MIT"
    url = "https://github.com/fsbondtec/webbridge"
    description = (
        "C++ to JavaScript bridge for building WebView2-based desktop UIs "
        "with Qt-like properties/methods/events and no boilerplate."
    )
    topics = ("webview", "gui", "javascript", "codegen")
    package_type = "static-library"
    settings = "os", "compiler", "build_type", "arch"

    exports_sources = (
        "CMakeLists.txt",
        "cmake/*",
        "src/webbridge/*",
        "tools/*",
        "requirements.txt",
    )

    def validate(self):
        if self.settings.os != "Windows":
            raise ConanInvalidConfiguration("webbridge relies on Microsoft WebView2 and is Windows-only.")

    def requirements(self):
        self.requires("nlohmann_json/3.11.3", transitive_headers=True)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        CMakeDeps(self).generate()
        CMakeToolchain(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "*.h",
             src=os.path.join(self.source_folder, "src", "webbridge"),
             dst=os.path.join(self.package_folder, "include", "webbridge"))
        copy(self, "*.lib", src=self.build_folder,
             dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "webbridge.cmake",
             src=os.path.join(self.source_folder, "cmake"),
             dst=os.path.join(self.package_folder, "cmake"))
        copy(self, "*", src=os.path.join(self.source_folder, "tools"),
             dst=os.path.join(self.package_folder, "tools"))
        copy(self, "requirements.txt", src=self.source_folder,
             dst=self.package_folder)

    def package_info(self):
        self.cpp_info.libs = ["webbridge"]
        self.cpp_info.defines = ["_WIN32_WINNT=0x0A00"]
        self.cpp_info.set_property("cmake_target_name", "webbridge::webbridge")
        self.cpp_info.builddirs = ["cmake"]
        self.cpp_info.set_property("cmake_build_modules", [os.path.join("cmake", "webbridge.cmake")])
