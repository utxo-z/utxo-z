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
        "with_tools": [True, False],
        "with_large_benchmarks": [True, False],
        "log": ["custom", "spdlog", "none"],
        "statistics_level": ["off", "basic", "lookup"],
        "sanitizer": ["none", "address", "undefined", "address,undefined", "thread"]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_tests": True,
        "with_examples": False,
        "with_benchmarks": False,
        # The operational tools travel with the package. utxoz_census answers
        # "what is in this database", which is asked about a production directory
        # by whoever is running one — so a consumer who installs the library gets
        # the binary that answers it, not a source tree they have to build again.
        "with_tools": True,
        "with_large_benchmarks": True,
        "log": "custom",
        # `basic` is what a node should ship. `lookup` compiles the per-class
        # read-path telemetry, which costs about eleven per cent of a find() that
        # hits — measured — and is meant for measurement builds. Figures in
        # doc/statistics-levels.md.
        "statistics_level": "basic",
        "sanitizer": "none"
    }

    # Sources are located in the same place as this recipe
    # include/* carries the two .hpp.in templates, not the headers they generate:
    # those live in the build tree now, so a package never depends on a generated
    # header having been left behind in somebody's source directory.
    # The compatibility fixtures are excluded on purpose. They are eighty
    # megabytes of evidence about what earlier builds wrote, which this
    # repository's CI needs and a consumer building from source does not — and
    # exporting them would put those bytes in every recipe revision.
    exports_sources = ("CMakeLists.txt", "src/*", "include/*", "examples/*", "tests/*",
                       "!tests/fixtures/*", "benchmarks/*", "tools/*", "LICENSE",
                       "README.md")

    def validate(self):
        if self.info.settings.compiler.cppstd:
            check_min_cppstd(self, "23")

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")
        if self.settings.os == "Emscripten":
            self.options.with_tests = False
            self.options.with_benchmarks = False

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.options["fmt/*"].header_only = True
        self.options["spdlog/*"].header_only = True
        self.options["boost/*"].header_only = True
        self.options["boost/*"].without_cobalt = True

    def requirements(self):
        self.requires("boost/1.91.0", transitive_headers=True, transitive_libs=True)
        self.requires("fmt/12.1.0", transitive_headers=True, transitive_libs=True)
        if self.options.log == "spdlog":
            self.requires("spdlog/1.17.0", transitive_headers=True, transitive_libs=True)

    def build_requirements(self):
        if self.options.with_tests:
            self.test_requires("catch2/3.15.0")

        if self.options.with_tests or self.options.with_benchmarks:
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
        tc.variables["UTXOZ_BUILD_TOOLS"] = self.options.with_tools
        tc.variables["UTXOZ_BUILD_LARGE_BENCHMARKS"] = self.options.with_large_benchmarks
        tc.variables["UTXOZ_LOG_BACKEND"] = str(self.options.log)
        tc.variables["UTXOZ_STATISTICS_LEVEL"] = str(self.options.statistics_level)
        tc.variables["UTXOZ_CONAN_BUILD"] = True
        if str(self.options.sanitizer) != "none":
            tc.variables["UTXOZ_SANITIZER"] = str(self.options.sanitizer)
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

        self.cpp_info.requires = ["boost::headers", "fmt::fmt"]
        if self.options.log == "spdlog":
            self.cpp_info.requires.append("spdlog::spdlog")

        # The same reason as the PUBLIC link in CMakeLists.txt, said again where
        # a package consumer can hear it: the archive references BCryptGenRandom
        # and cannot resolve it on its own. Without this, `conan create` builds
        # the package and any consumer of it fails to link.
        if self.settings.os == "Windows":
            self.cpp_info.system_libs.append("bcrypt")

        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.extend(["pthread", "rt"])

        # CMake integration
        self.cpp_info.set_property("cmake_file_name", "utxoz")
        self.cpp_info.set_property("cmake_target_name", "utxoz::utxoz")
