#!/usr/bin/env python3

"""Verify the CI toolchain and persist the evidence as JSON."""

from __future__ import annotations

import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def run(*command: str, allow_failure: bool = False) -> str:
    completed = subprocess.run(command, capture_output=True, text=True)
    output = (completed.stdout + completed.stderr).strip()
    if completed.returncode != 0 and not allow_failure:
        raise RuntimeError(f"{' '.join(command)} failed ({completed.returncode}):\n{output}")
    return output


def compiler_version(kind: str, compiler: str) -> tuple[str, int]:
    if kind == "msvc":
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "probe.cpp"
            object_file = Path(directory) / "probe.obj"
            source.write_text("int toolchain_probe;\n", encoding="utf-8")
            output = run(
                compiler,
                "/nologo",
                "/Bv",
                "/c",
                str(source),
                f"/Fo{object_file}",
            )
        # VS 2022 prints "Compiler Version 19.xx" while VS 2026's /Bv output
        # prefixes the compiler path and prints only "Version 19.xx".
        match = re.search(r"(?:Compiler\s+)?Version\s+19\.(\d+)", output)
        # MSVC 14.44 is compiler version 19.44.
        major = int(match.group(1)) if match else -1
    else:
        output = run(compiler, "--version")
        pattern = {
            "gcc": r"(?:g\+\+|GCC).*?\b(\d+)\.",
            "apple-clang": r"Apple clang version (\d+)\.",
            "emscripten": r"em(?:cc|\+\+).*? (\d+)\.",
        }[kind]
        match = re.search(pattern, output, re.IGNORECASE | re.DOTALL)
        major = int(match.group(1)) if match else -1
    return output, major


def cmake_cache(path: Path) -> dict[str, str]:
    entries: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = re.match(r"([^#/:][^:]*):[^=]+=(.*)", line)
        if match:
            entries[match.group(1)] = match.group(2)
    return entries


def configured_compiler(cache_path: Path, entries: dict[str, str]) -> tuple[str, str]:
    if compiler := entries.get("CMAKE_CXX_COMPILER"):
        return compiler, "CMakeCache.txt"

    compiler_files = sorted(
        (cache_path.parent / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake")
    )
    for compiler_file in reversed(compiler_files):
        contents = compiler_file.read_text(encoding="utf-8")
        match = re.search(r'set\(CMAKE_CXX_COMPILER\s+"([^"]+)"\)', contents)
        if match:
            return match.group(1), str(compiler_file.relative_to(cache_path.parent))
    raise RuntimeError("CMAKE_CXX_COMPILER is absent from CMake's configured state")


def configured_cppstd(entries: dict[str, str]) -> str:
    toolchain = entries.get("CMAKE_TOOLCHAIN_FILE")
    if not toolchain:
        raise RuntimeError("CMAKE_TOOLCHAIN_FILE is absent from CMakeCache.txt")
    contents = Path(toolchain).read_text(encoding="utf-8")
    match = re.search(r"set\(CMAKE_CXX_STANDARD\s+(\d+)\)", contents)
    if not match:
        raise RuntimeError(f"CMAKE_CXX_STANDARD is absent from {toolchain}")
    return match.group(1)


def main() -> int:
    kind = os.environ["EXPECTED_COMPILER_KIND"]
    expected_major = int(os.environ["EXPECTED_COMPILER_MAJOR"])
    cache_path = os.environ.get("CMAKE_CACHE")
    configured: dict[str, str] = {}
    compiler_source = "environment preflight"
    if cache_path:
        cache = Path(cache_path)
        configured = cmake_cache(cache)
        compiler, compiler_source = configured_compiler(cache, configured)
    else:
        compiler = os.environ["CXX"]

    compiler_path = shutil.which(compiler)
    if compiler_path is None and Path(compiler).is_file():
        compiler_path = str(Path(compiler).resolve())
    if compiler_path is None:
        raise RuntimeError(f"compiler does not exist or is not on PATH: {compiler}")

    version_output, actual_major = compiler_version(kind, compiler_path)
    if actual_major != expected_major:
        print(version_output, file=sys.stderr)
        print(
            f"expected {kind} major {expected_major}, detected {actual_major}",
            file=sys.stderr,
        )
        return 1

    conan_version = run("conan", "--version")
    expected_conan = os.environ["CONAN_VERSION"]
    if not re.search(rf"\b{re.escape(expected_conan)}\b", conan_version):
        print(f"expected Conan {expected_conan}, got {conan_version}", file=sys.stderr)
        return 1

    expected_cppstd = os.environ.get("EXPECTED_CPPSTD")
    profile_name = os.environ.get("CONAN_PROFILE", "default")
    profile_command = [
        "conan",
        "profile",
        "show",
        "-pr:h",
        profile_name,
        "-pr:b",
        "default",
        "--format=json",
    ]
    profile = json.loads(run(*profile_command))

    cppstd = None
    if expected_cppstd:
        profile_cppstd = str(profile["host"]["settings"].get("compiler.cppstd", ""))
        if profile_cppstd != expected_cppstd:
            print(
                f"expected Conan C++{expected_cppstd}, got C++{profile_cppstd}",
                file=sys.stderr,
            )
            return 1
        cppstd = expected_cppstd
        if configured:
            configured_standard = configured_cppstd(configured)
            if configured_standard != expected_cppstd:
                print(
                    f"expected C++{expected_cppstd}, configured C++{configured_standard}",
                    file=sys.stderr,
                )
                return 1

    expected_runner_os = os.environ.get("EXPECTED_RUNNER_OS")
    actual_runner_os = os.environ.get("RUNNER_OS", platform.system())
    if expected_runner_os and actual_runner_os != expected_runner_os:
        print(
            f"expected runner OS {expected_runner_os}, got {actual_runner_os}",
            file=sys.stderr,
        )
        return 1
    expected_runner_arch = os.environ.get("EXPECTED_RUNNER_ARCH")
    native_arch = {
        "x86_64": "X64",
        "AMD64": "X64",
        "arm64": "ARM64",
        "aarch64": "ARM64",
    }.get(platform.machine(), platform.machine())
    actual_runner_arch = os.environ.get("RUNNER_ARCH", native_arch)
    if expected_runner_arch and actual_runner_arch != expected_runner_arch:
        print(
            f"expected runner architecture {expected_runner_arch}, got {actual_runner_arch}",
            file=sys.stderr,
        )
        return 1
    platform_toolchain: dict[str, str] = {}
    if expected_xcode := os.environ.get("EXPECTED_XCODE"):
        xcode = run("xcodebuild", "-version")
        if not xcode.startswith(f"Xcode {expected_xcode}\n"):
            print(f"expected Xcode {expected_xcode}, got:\n{xcode}", file=sys.stderr)
            return 1
        platform_toolchain["xcode"] = xcode
    if expected_vs := os.environ.get("EXPECTED_VISUAL_STUDIO"):
        vswhere = os.environ["VSWHERE"]
        visual_studio = run(vswhere, "-latest", "-products", "*", "-property", "catalog_productDisplayVersion")
        if not re.fullmatch(
            rf"{re.escape(expected_vs)}(?:\.\d+)*", visual_studio.strip()
        ):
            print(f"expected Visual Studio {expected_vs}, got {visual_studio}", file=sys.stderr)
            return 1
        platform_toolchain["visual_studio"] = visual_studio

    manifest = {
        "runner": {
            "os": actual_runner_os,
            "arch": actual_runner_arch,
            "image": os.environ.get("ImageOS", ""),
            "image_version": os.environ.get("ImageVersion", ""),
        },
        "compiler": {
            "kind": kind,
            "expected_major": expected_major,
            "path": compiler_path,
            "version": version_output,
            "source": compiler_source,
        },
        "platform_toolchain": platform_toolchain,
        "conan": {
            "version": conan_version,
            "host_profile": profile_name,
            "profile": profile,
        },
        "cmake": run("cmake", "--version").splitlines()[0],
        "cppstd": cppstd,
    }
    destination = Path(os.environ.get("TOOLCHAIN_MANIFEST", "toolchain-manifest.json"))
    destination.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"toolchain verification failed: {error}", file=sys.stderr)
        raise SystemExit(2)
