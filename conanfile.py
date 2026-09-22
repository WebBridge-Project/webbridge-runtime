import os
from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy, get


class WebbridgeConan(ConanFile):
    name = "webbridge"
    version = "1.0.0"
    license = "MIT"
    url = "https://github.com/WebBridge-Project/webbridge-runtime"
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
    )

    def validate(self):
        if self.settings.os != "Windows":
            raise ConanInvalidConfiguration("webbridge relies on Microsoft WebView2 and is Windows-only.")

    def requirements(self):
        self.requires("nlohmann_json/3.11.3", transitive_headers=True)

    def layout(self):
        cmake_layout(self)

    def source(self):
        data = self.conan_data["sources"][self.version]
        get(self, **data["webview"], destination=os.path.join("vendor", "webview"), strip_root=True)
        get(self, **data["webview2"], destination=os.path.join("vendor", "webview2"), filename="webview2.zip")

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
        copy(self, "webbridge-webview.cmake",
             src=os.path.join(self.source_folder, "cmake"),
             dst=os.path.join(self.package_folder, "cmake"))
        copy(self, "*", src=os.path.join(self.source_folder, "vendor", "webview"),
             dst=os.path.join(self.package_folder, "vendor", "webview"))
        copy(self, "*", src=os.path.join(self.source_folder, "vendor", "webview2"),
             dst=os.path.join(self.package_folder, "vendor", "webview2"))

    def package_info(self):
        self.cpp_info.libs = ["webbridge"]
        self.cpp_info.defines = ["_WIN32_WINNT=0x0A00"]
        self.cpp_info.set_property("cmake_target_name", "webbridge::webbridge")
        self.cpp_info.builddirs = ["cmake"]
        self.cpp_info.set_property("cmake_build_modules", [os.path.join("cmake", "webbridge-webview.cmake")])
