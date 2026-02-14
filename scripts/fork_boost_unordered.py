#!/usr/bin/env python3
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Fork boost::unordered_flat_map for direct mmap storage.
#
# This script clones boost/unordered at a specific tag, copies the files
# needed for flat_map, renames the namespace to utxoz::flatmap, remaps internal
# includes, and applies modifications for mmap buffer support.
#
# Usage: python3 fork_boost_unordered.py [boost-tag]
#   Default tag: boost-1.90.0
#
# To update to a newer boost version, re-run with the new tag.

import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

BOOST_TAG = sys.argv[1] if len(sys.argv) > 1 else "boost-1.90.0"
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_DIR = SCRIPT_DIR.parent
SRC_DIR = PROJECT_DIR / "src"
FOA_DIR = SRC_DIR / "utxoz" / "flatmap"


def run(cmd: list[str], **kwargs):
    print(f"  $ {' '.join(cmd[:4])}{'...' if len(cmd) > 4 else ''}")
    subprocess.run(cmd, check=True, **kwargs)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write(path: Path, content: str):
    path.write_text(content, encoding="utf-8")


PELLICCIONI_COPYRIGHT = "Copyright 2026 Fernando Pelliccioni."


def add_copyright(txt: str) -> str:
    """Insert Fernando's copyright after the last existing Copyright line,
    matching the comment style of the file (/* */ block or // lines)."""
    # Find the last "Copyright" line
    lines = txt.split('\n')
    last_copyright_idx = -1
    for i, line in enumerate(lines):
        if 'Copyright' in line:
            last_copyright_idx = i
    if last_copyright_idx < 0:
        # No copyright found, prepend
        return f"// {PELLICCIONI_COPYRIGHT}\n" + txt

    ref = lines[last_copyright_idx]
    if ref.lstrip().startswith('*') or ref.lstrip().startswith('/*'):
        # Block comment style: " * Copyright ..."
        new_line = f" * {PELLICCIONI_COPYRIGHT}"
    else:
        # Line comment style: "// Copyright ..."
        new_line = f"// {PELLICCIONI_COPYRIGHT}"
    lines.insert(last_copyright_idx + 1, new_line)
    return '\n'.join(lines)


# ── Files to copy from boost/unordered ────────────────────────────────────────
# Source path relative to include/boost/unordered/ → dest path relative to src/flatmap/
FILE_MAP: dict[str, str] = {
    # Main headers
    "unordered_flat_map.hpp":     "mmap_flat_map.hpp",
    "unordered_flat_map_fwd.hpp": "mmap_flat_map_fwd.hpp",
    # FOA detail
    "detail/foa/core.hpp":                   "detail/core.hpp",
    "detail/foa/table.hpp":                  "detail/table.hpp",
    "detail/foa/flat_map_types.hpp":         "detail/flat_map_types.hpp",
    "detail/foa/element_type.hpp":           "detail/element_type.hpp",
    "detail/foa/types_constructibility.hpp": "detail/types_constructibility.hpp",
    "detail/foa/ignore_wshadow.hpp":         "detail/ignore_wshadow.hpp",
    "detail/foa/restore_wshadow.hpp":        "detail/restore_wshadow.hpp",
    # Non-FOA detail
    "detail/mulx.hpp":            "detail/mulx.hpp",
    "detail/narrow_cast.hpp":     "detail/narrow_cast.hpp",
    "detail/static_assert.hpp":   "detail/static_assert.hpp",
    "detail/type_traits.hpp":     "detail/type_traits.hpp",
    "detail/opt_storage.hpp":     "detail/opt_storage.hpp",
}

# Optional files — copy if they exist in the boost version
OPTIONAL_FILES: dict[str, str] = {
    "detail/allocator_constructed.hpp":          "detail/allocator_constructed.hpp",
    "detail/foa/allocator_aware_hash_equal.hpp": "detail/allocator_aware_hash_equal.hpp",
    "detail/foa/cumulative_stats.hpp":           "detail/cumulative_stats.hpp",
    "detail/foa/reentrancy_check.hpp":           "detail/reentrancy_check.hpp",
    "detail/foa/tuple_rotate_right.hpp":         "detail/tuple_rotate_right.hpp",
    # Additional transitive dependencies
    "concurrent_flat_map_fwd.hpp":               "concurrent_flat_map_fwd.hpp",
    "detail/throw_exception.hpp":                "detail/throw_exception.hpp",
    "detail/unordered_printers.hpp":             "detail/unordered_printers.hpp",
    "detail/foa/rw_spinlock.hpp":                "detail/rw_spinlock.hpp",
}


# ═══════════════════════════════════════════════════════════════════════════════
# Section 1: Clone
# ═══════════════════════════════════════════════════════════════════════════════

def clone_boost(work_dir: Path) -> Path:
    print(f"\n=== Section 1: Clone boost/unordered at {BOOST_TAG} ===")
    repo_dir = work_dir / "unordered"
    run(["git", "clone", "--depth", "1", "--branch", BOOST_TAG,
         "https://github.com/boostorg/unordered.git", str(repo_dir)])
    return repo_dir / "include" / "boost" / "unordered"


# ═══════════════════════════════════════════════════════════════════════════════
# Section 2: Copy files
# ═══════════════════════════════════════════════════════════════════════════════

def copy_files(boost_src: Path):
    print(f"\n=== Section 2: Copy files to {FOA_DIR.relative_to(PROJECT_DIR)} ===")

    if FOA_DIR.exists():
        shutil.rmtree(FOA_DIR)
    (FOA_DIR / "detail").mkdir(parents=True)

    copied = 0
    for src_rel, dst_rel in FILE_MAP.items():
        src = boost_src / src_rel
        dst = FOA_DIR / dst_rel
        if not src.exists():
            print(f"  WARNING: required file missing: {src_rel}")
            continue
        shutil.copy2(src, dst)
        copied += 1

    for src_rel, dst_rel in OPTIONAL_FILES.items():
        src = boost_src / src_rel
        dst = FOA_DIR / dst_rel
        if src.exists():
            shutil.copy2(src, dst)
            copied += 1

    print(f"  Copied {copied} files")


# ═══════════════════════════════════════════════════════════════════════════════
# Section 3: Namespace rename  boost::unordered → utxoz::flatmap
# ═══════════════════════════════════════════════════════════════════════════════

def rename_namespaces():
    print("\n=== Section 3: Namespace rename ===")

    for hpp in FOA_DIR.rglob("*.hpp"):
        txt = read(hpp)

        # Qualified names (order: most specific first)
        txt = txt.replace("boost::unordered::detail", "utxoz::flatmap::detail")
        txt = txt.replace("boost::unordered", "utxoz::flatmap")

        # Namespace declarations  (preserve original spacing before {)
        txt = re.sub(r'namespace\s+boost(\s*)\{', r'namespace utxoz\1{', txt)
        txt = re.sub(r'namespace\s+unordered(\s*)\{', r'namespace flatmap\1{', txt)

        # Closing comments
        txt = txt.replace("/* namespace boost */",      "/* namespace utxoz */")
        txt = txt.replace("/* namespace unordered */",   "/* namespace flatmap */")
        txt = txt.replace("// namespace boost",          "// namespace utxoz")
        txt = txt.replace("// namespace unordered",      "// namespace flatmap")

        # After renaming namespace boost → utxoz, unqualified references to
        # boost:: types (like empty_value, empty_init) no longer resolve.
        # Add boost:: qualifier where needed.
        txt = re.sub(r'(?<!boost::)(?<!\w)empty_value\s*<',
                     'boost::empty_value<', txt)
        txt = re.sub(r'(?<!boost::)(?<!\w)(empty_init)\b(?!_)',
                     r'boost::\1', txt)

        write(hpp, txt)

    print("  Done")


# ═══════════════════════════════════════════════════════════════════════════════
# Section 4: Remap internal #includes
# ═══════════════════════════════════════════════════════════════════════════════

def remap_includes():
    print("\n=== Section 4: Remap internal includes ===")

    for hpp in FOA_DIR.rglob("*.hpp"):
        txt = read(hpp)

        # #include <boost/unordered/detail/foa/X> → #include <utxoz/flatmap/detail/X>
        txt = re.sub(
            r'#include\s*<boost/unordered/detail/foa/([^>]+)>',
            r'#include <utxoz/flatmap/detail/\1>', txt)

        # #include <boost/unordered/detail/X> → #include <utxoz/flatmap/detail/X>
        txt = re.sub(
            r'#include\s*<boost/unordered/detail/([^>]+)>',
            r'#include <utxoz/flatmap/detail/\1>', txt)

        # #include <boost/unordered/X> → #include <utxoz/flatmap/X>
        txt = re.sub(
            r'#include\s*<boost/unordered/([^>]+)>',
            r'#include <utxoz/flatmap/\1>', txt)

        write(hpp, txt)

    print("  Done")


# ═══════════════════════════════════════════════════════════════════════════════
# Section 5: Rename header guards
# ═══════════════════════════════════════════════════════════════════════════════

def rename_guards():
    print("\n=== Section 5: Rename header guards ===")

    for hpp in FOA_DIR.rglob("*.hpp"):
        txt = read(hpp)
        # Order matters: most specific first
        txt = txt.replace("BOOST_UNORDERED_DETAIL_FOA_", "UTXOZ_FLATMAP_DETAIL_FOA_")
        txt = txt.replace("BOOST_UNORDERED_DETAIL_",     "UTXOZ_FLATMAP_DETAIL_")
        txt = txt.replace("BOOST_UNORDERED_",            "UTXOZ_FLATMAP_")
        write(hpp, txt)

    print("  Done")


# ═══════════════════════════════════════════════════════════════════════════════
# Section 6: Modify table_arrays for mmap buffer support
# ═══════════════════════════════════════════════════════════════════════════════

MMAP_BUFFER_METHODS = r"""
  // === UTXO-Z: mmap buffer support ===

  static table_arrays from_buffer(
    void* buffer, std::size_t gsi, std::size_t gsm)
  {
    auto gs = gsm + 1;
    auto p  = static_cast<unsigned char*>(buffer);
    return table_arrays{
      gsi, gsm,
      reinterpret_cast<group_type*>(p),
      reinterpret_cast<value_type*>(p + gs * sizeof(group_type))
    };
  }

  static table_arrays from_buffer_init(
    void* buffer, std::size_t gsi, std::size_t gsm)
  {
    auto arrays = from_buffer(buffer, gsi, gsm);
    auto gs = gsm + 1;

    // Initialize all groups to empty (memset to 0, same as boost's initialize_groups)
    std::memset(arrays.groups(), 0, gs * sizeof(group_type));
    // Set sentinel on last group
    arrays.groups()[gs - 1].set_sentinel();
    return arrays;
  }

  static std::size_t mmap_buffer_size(std::size_t gsm) noexcept
  {
    auto gs = gsm + 1;
    return gs * sizeof(group_type)
         + (gs * group_type::N - 1) * sizeof(value_type);
  }

  static std::size_t capacity_for(std::size_t gsm) noexcept
  {
    return (gsm + 1) * group_type::N - 1;
  }

  // === end UTXO-Z additions ===
"""


def find_matching_brace(txt: str, open_pos: int) -> int:
    """Find the closing brace matching the opening brace at open_pos."""
    depth = 0
    i = open_pos
    while i < len(txt):
        if txt[i] == '{':
            depth += 1
        elif txt[i] == '}':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def modify_table_arrays():
    print("\n=== Section 6: Modify table_arrays for mmap buffer ===")

    core = FOA_DIR / "detail" / "core.hpp"
    txt = read(core)

    # Add copyright into the existing block comment
    txt = add_copyright(txt)

    # 6a. Make delete_ a no-op (handles nested braces via brace counting).
    m = re.search(r'static\s+void\s+delete_\s*\([^)]*\)\s*noexcept\s*\{', txt, re.DOTALL)
    if m:
        # Find the opening brace position
        open_brace = txt.rindex('{', m.start(), m.end())
        close_brace = find_matching_brace(txt, open_brace)
        if close_brace > 0:
            # Replace the entire body (from { to }) with no-op
            txt = (txt[:open_brace] +
                   '{\n    // No-op: buffer is mmap-managed, do not deallocate\n  }' +
                   MMAP_BUFFER_METHODS +
                   txt[close_brace + 1:])
            print("  Replaced delete_ body + added from_buffer/from_buffer_init/buffer_size/capacity_for")
        else:
            print("  WARNING: Could not find matching brace for delete_")
    else:
        print("  WARNING: Could not find delete_ function")

    write(core, txt)

    # 6c. Disable auto-rehash in table.hpp.
    table = FOA_DIR / "detail" / "table.hpp"
    ttxt = read(table)
    ttxt = re.sub(
        r'else\s*\{\s*return\s*\{\s*make_iterator\(\s*'
        r'this->unchecked_emplace_with_rehash\s*\(\s*'
        r'hash\s*,\s*std::forward<Args>\(args\)\.\.\.\)\s*\)\s*,\s*'
        r'true\s*\}\s*;\s*\}',
        'else{\n'
        '      BOOST_ASSERT(false && "mmap_flat_map: table full, rehash disabled");\n'
        '      UTXOZ_FLATMAP_ASSUME(false);\n'
        '    }',
        ttxt,
        flags=re.DOTALL)
    # Add copyright into the existing block comment
    ttxt = add_copyright(ttxt)
    write(table, ttxt)
    print("  Disabled auto-rehash in table.hpp")
    print("  Done")


# ═══════════════════════════════════════════════════════════════════════════════
# Section 7: Add create/attach factory methods to table
# ═══════════════════════════════════════════════════════════════════════════════

TABLE_FACTORY_METHODS = r"""
public:
  // === UTXO-Z: mmap factory methods ===

  /// Create a fresh table in an mmap buffer (initializes groups + sentinel).
  static table create(
    void* buffer, std::size_t groups_size_index, std::size_t groups_size_mask,
    Hash const& h = Hash{}, Pred const& pred = Pred{})
  {
    table t;
    t.h()    = h;
    t.pred() = pred;
    t.arrays = arrays_type::from_buffer_init(
        buffer, groups_size_index, groups_size_mask);
    t.size_ctrl.ml = static_cast<std::size_t>(
        std::ceil(static_cast<float>(
            arrays_type::capacity_for(groups_size_mask)) * mlf));
    t.size_ctrl.size = 0;
    return t;
  }

  /// Attach to an existing table already in an mmap buffer.
  static table attach(
    void* buffer,
    std::size_t groups_size_index, std::size_t groups_size_mask,
    std::size_t size, std::size_t ml,
    Hash const& h = Hash{}, Pred const& pred = Pred{})
  {
    table t;
    t.h()    = h;
    t.pred() = pred;
    t.arrays = arrays_type::from_buffer(
        buffer, groups_size_index, groups_size_mask);
    t.size_ctrl.size = size;
    t.size_ctrl.ml   = ml;
    return t;
  }

  std::size_t get_groups_size_index() const noexcept {
    return this->arrays.groups_size_index;
  }
  std::size_t get_groups_size_mask() const noexcept {
    return this->arrays.groups_size_mask;
  }
  std::size_t get_max_load() const noexcept {
    return this->size_ctrl.ml;
  }
  std::size_t capacity() const noexcept {
    return arrays_type::capacity_for(this->arrays.groups_size_mask);
  }
  static std::size_t required_buffer_size(std::size_t groups_size_mask) noexcept {
    return arrays_type::mmap_buffer_size(groups_size_mask);
  }

  // === end UTXO-Z factory methods ===
"""


def add_table_factory_methods():
    print("\n=== Section 7: Add create/attach factory methods to table ===")

    table = FOA_DIR / "detail" / "table.hpp"
    txt = read(table)

    # Find the last }; in the table class.
    # Strategy: find the pattern "using table_core_type::table_core_type;"
    # and insert our methods after the next line, OR find the final };
    # that closes the table struct/class.

    # Look for the table class closing. The table class is typically the last
    # class/struct in the file. We insert before the final };
    # Find all }; and use the second-to-last one (last is namespace closing)

    # More robust: find "struct table" or "class table" then find its closing
    # Let's find the LAST }; that's followed by namespace closings and insert before it.

    # Actually, let's just find a good anchor point.
    # In boost 1.90.0, table.hpp ends like:
    #   ... (methods) ...
    # }; /* table */
    # } /* namespace detail */
    # } /* namespace flatmap */
    # } /* namespace utxoz */

    # Insert before the first }; that closes the table class
    lines = txt.split('\n')
    insert_idx = None

    # Find the table class DEFINITION (not forward declaration).
    # The definition has 'class table:table_core_impl' (with inheritance),
    # not 'class table;' (forward decl).
    in_table = False
    brace_depth = 0
    for i, line in enumerate(lines):
        stripped = line.strip()
        # Match class definition with inheritance, not forward declaration
        if re.match(r'class\s+table\s*:', stripped) and not stripped.endswith(';'):
            in_table = True
            brace_depth = 0

        if in_table:
            brace_depth += line.count('{') - line.count('}')
            if brace_depth <= 0 and stripped.startswith('};'):
                insert_idx = i
                break

    if insert_idx is not None:
        lines.insert(insert_idx, TABLE_FACTORY_METHODS)
        txt = '\n'.join(lines)
        print(f"  Inserted factory methods at line {insert_idx}")
    else:
        print("  WARNING: Could not find table class closing to insert methods")

    write(table, txt)
    print("  Done")


# ═══════════════════════════════════════════════════════════════════════════════
# Section 8: Rename unordered_flat_map → mmap_flat_map
# ═══════════════════════════════════════════════════════════════════════════════

def rename_to_mmap():
    print("\n=== Section 8: Rename to mmap_flat_map ===")

    for hpp in FOA_DIR.rglob("*.hpp"):
        txt = read(hpp)
        txt = txt.replace("unordered_flat_map", "mmap_flat_map")
        write(hpp, txt)

    print("  Done")


# ═══════════════════════════════════════════════════════════════════════════════
# Section 9: Strip serialization code (not needed for mmap)
# ═══════════════════════════════════════════════════════════════════════════════

def strip_serialization():
    print("\n=== Section 9: Strip serialization code ===")

    # 9a. Remove serialize_container include and serialize functions from mmap_flat_map.hpp
    main_hpp = FOA_DIR / "mmap_flat_map.hpp"
    txt = read(main_hpp)
    txt = re.sub(r'#include\s*[<"][^>"]*serialize_container\.hpp[>"]\s*\n', '', txt)
    # Remove the serialize free function (template with detail::serialize_container call)
    txt = re.sub(
        r'\n\s*template\s*<[^>]*>\s*void\s+serialize\s*\([^)]*Archive[^)]*\)\s*\{[^}]*serialize_container[^}]*\}\s*\n',
        '\n',
        txt,
        flags=re.DOTALL)
    write(main_hpp, txt)
    print("  Stripped serialize from mmap_flat_map.hpp")

    # 9b. Remove serialize_tracked_address include and serialize method from table.hpp
    table_hpp = FOA_DIR / "detail" / "table.hpp"
    txt = read(table_hpp)
    txt = re.sub(r'#include\s*[<"][^>"]*serialize_tracked_address\.hpp[>"]\s*\n', '', txt)
    # Remove serialize member functions that use serialize_tracked_address
    # These are typically: template<typename Archive> void serialize(Archive& ar, unsigned int version)
    # in the const_iterator and/or table_iterator classes
    txt = re.sub(
        r'\n\s*template\s*<[^>]*>\s*void\s+serialize\s*\([^)]*Archive[^)]*\)\s*\{[^}]*serialize_tracked_address[^}]*\}\s*\n',
        '\n',
        txt,
        flags=re.DOTALL)
    write(table_hpp, txt)
    print("  Stripped serialize from table.hpp")

    print("  Done")


# ═══════════════════════════════════════════════════════════════════════════════
# Section 10: Add forwarding factory methods to mmap_flat_map
# ═══════════════════════════════════════════════════════════════════════════════

MMAP_FLAT_MAP_FACTORY = r"""
      // === UTXO-Z: mmap factory methods ===

      /// Create a fresh map in an mmap buffer (initializes groups + sentinel).
      static mmap_flat_map create(
        void* buffer, std::size_t groups_size_index, std::size_t groups_size_mask,
        hasher const& h = hasher(), key_equal const& pred = key_equal())
      {
        mmap_flat_map m;
        m.table_ = table_type::create(buffer, groups_size_index, groups_size_mask, h, pred);
        return m;
      }

      /// Attach to an existing map already in an mmap buffer.
      static mmap_flat_map attach(
        void* buffer,
        std::size_t groups_size_index, std::size_t groups_size_mask,
        std::size_t size, std::size_t ml,
        hasher const& h = hasher(), key_equal const& pred = key_equal())
      {
        mmap_flat_map m;
        m.table_ = table_type::attach(buffer, groups_size_index, groups_size_mask, size, ml, h, pred);
        return m;
      }

      std::size_t get_groups_size_index() const noexcept {
        return table_.get_groups_size_index();
      }
      std::size_t get_groups_size_mask() const noexcept {
        return table_.get_groups_size_mask();
      }
      std::size_t get_max_load() const noexcept {
        return table_.get_max_load();
      }
      std::size_t capacity() const noexcept {
        return table_.capacity();
      }
      static std::size_t required_buffer_size(std::size_t groups_size_mask) noexcept {
        return table_type::required_buffer_size(groups_size_mask);
      }

      // === end UTXO-Z factory methods ===
"""


def add_mmap_flat_map_factory():
    print("\n=== Section 10: Add factory methods to mmap_flat_map ===")

    main_hpp = FOA_DIR / "mmap_flat_map.hpp"
    txt = read(main_hpp)

    # Add copyright
    txt = add_copyright(txt)

    # Find "class mmap_flat_map" then "public:" and insert after the first
    # block of using declarations, before the first constructor.
    # Strategy: insert just before "mmap_flat_map() : mmap_flat_map(0) {}"
    anchor = "mmap_flat_map() : mmap_flat_map(0) {}"
    pos = txt.find(anchor)
    if pos >= 0:
        txt = txt[:pos] + MMAP_FLAT_MAP_FACTORY + "\n      " + txt[pos:]
        print("  Inserted factory methods in mmap_flat_map")
    else:
        print("  WARNING: Could not find default constructor anchor")

    write(main_hpp, txt)
    print("  Done")


# ═══════════════════════════════════════════════════════════════════════════════
# Main
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    print(f"=== Fork boost/unordered ({BOOST_TAG}) for mmap storage ===")
    print(f"Project dir: {PROJECT_DIR}")
    print(f"Output dir:  {FOA_DIR}")

    with tempfile.TemporaryDirectory() as work_dir:
        boost_src = clone_boost(Path(work_dir))
        copy_files(boost_src)

    rename_namespaces()
    remap_includes()
    rename_guards()
    modify_table_arrays()
    add_table_factory_methods()
    rename_to_mmap()
    strip_serialization()
    add_mmap_flat_map_factory()

    print(f"\n=== Fork complete ===")
    print(f"Generated files:")
    for hpp in sorted(FOA_DIR.rglob("*.hpp")):
        print(f"  {hpp.relative_to(PROJECT_DIR)}")
    print(f"\nBoost tag: {BOOST_TAG}")


if __name__ == "__main__":
    main()
