# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build.cppstd import check_min_cppstd
from conan.tools.files import copy
import os

required_conan_version = ">=2.0"

class UtxozConan(ConanFile):
    name = "utxoz"
    # version is set dynamically via --version parameter

    # Package metadata
    description = "High-performance UTXO Database for Bitcoin Cash"
    homepage = "https://github.com/utxo-z/utxo-z"
    url = "https://github.com/utxo-z/utxo-z"
    license = "MIT"
    author = "Fernando Pelliccioni <fpelliccioni@gmail.com>"
    topics = ("blockchain", "utxo", "database", "bitcoin", "cryptocurrency")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_tests": [True, False],
        "with_examples": [True, False],
        "with_benchmarks": [True, False],
        "log": ["custom", "spdlog", "none"],
        "statistics": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_tests": True,
        "with_examples": False,
        "with_benchmarks": False,
        "log": "custom",
        "statistics": True
    }

    # Sources are located in the same place as this recipe
    exports_sources = "CMakeLists.txt", "src/*", "include/*", "examples/*", "tests/*", "LICENSE", "README.md"

    def validate(self):
        if self.info.settings.compiler.cppstd:
            check_min_cppstd(self, "23")

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.options["fmt/*"].header_only = True
        self.options["spdlog/*"].header_only = True

    def requirements(self):
        self.requires("boost/1.90.0", transitive_headers=True, transitive_libs=True)
        self.requires("fmt/12.0.0", transitive_headers=True, transitive_libs=True)
        if self.options.log == "spdlog":
            self.requires("spdlog/1.16.0", transitive_headers=True, transitive_libs=True)

    def build_requirements(self):
        if self.options.with_tests:
            self.test_requires("catch2/3.11.0")
            self.test_requires("nanobench/4.3.11")

        if self.options.with_benchmarks:
            self.test_requires("nanobench/4.3.11")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.variables["CMAKE_VERBOSE_MAKEFILE"] = "ON"
        utxoz_version = str(self.version) if self.version else "0.0.0-dev"
        tc.variables["UTXOZ_VERSION"] = utxoz_version
        tc.variables["UTXOZ_BUILD_TESTS"] = self.options.with_tests
        tc.variables["UTXOZ_BUILD_EXAMPLES"] = self.options.with_examples
        tc.variables["UTXOZ_BUILD_BENCHMARKS"] = self.options.with_benchmarks
        tc.variables["UTXOZ_LOG_BACKEND"] = str(self.options.log)
        tc.variables["UTXOZ_STATISTICS_ENABLED"] = self.options.statistics
        tc.variables["UTXOZ_CONAN_BUILD"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", dst=os.path.join(self.package_folder, "licenses"), src=self.source_folder)
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["utxoz"]
        self.cpp_info.includedirs = ["include"]

        self.cpp_info.requires = ["boost::headers", "boost::filesystem", "fmt::fmt"]
        if self.options.log == "spdlog":
            self.cpp_info.requires.append("spdlog::spdlog")

        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.extend(["pthread", "rt"])

        # CMake integration
        self.cpp_info.set_property("cmake_file_name", "utxoz")
        self.cpp_info.set_property("cmake_target_name", "utxoz::utxoz")
