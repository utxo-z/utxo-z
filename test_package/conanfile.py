# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, cmake_layout


class utxozTestConan(ConanFile):
    """Compiles and runs a consumer against the packaged library.

    `conan create` used to stop at building the package, which cannot tell
    whether the package is usable. A header missing from the install produces no
    error here and no error in CI — it produces a broken #include in whoever
    consumes the package next, which for this project means Knuth.

    So this consumer includes the two generated headers by name. If the install
    stops carrying them, `conan create` fails in this repository instead of
    somewhere downstream.
    """

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
        if can_run(self):
            self.run(os.path.join(self.cpp.build.bindir, "test_package"), env="conanrun")
