window.BENCHMARK_DATA = {
  "lastUpdate": 1786380278578,
  "repoUrl": "https://github.com/utxo-z/utxo-z",
  "entries": {
    "Benchmark": [
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ddd0fa0e99d97003de76df4eab78cddb22c74536",
          "message": "feat: Add nanobench benchmark infrastructure with CI tracking (#10)\n\nAdd self-contained benchmark suite using nanobench for insert, find,\nerase, and mixed workload (simulated IBD) benchmarks. CI job on Ubuntu\nruns benchmarks on push/PR and tracks regressions via github-action-benchmark\non gh-pages. Build scripts support --bench flag.",
          "timestamp": "2026-02-13T11:19:19+01:00",
          "tree_id": "90261725f2c9e330f120606a8784bb57da7d4149",
          "url": "https://github.com/utxo-z/utxo-z/commit/ddd0fa0e99d97003de76df4eab78cddb22c74536"
        },
        "date": 1770978074799,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 65.65,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 65.66,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 65.7,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 65.75,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.53,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 9.81,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 46.51,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.95,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.07,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.45,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.58,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 46.01,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.77,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.54,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.1,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.47,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.79,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a1ee9cb9aad91dd08854057619de0f7be764e816",
          "message": "test: Add storage correctness tests for persistence, rotation, and cross-version ops (#11)\n\n- Reopen persistence: data survives close/reopen cycles\n- Multiple reopen cycles: accumulative inserts across 5 cycles\n- All four container sizes persist correctly\n- File rotation: 200K inserts trigger rotation, data remains accessible\n- Deferred erase across versions: delete from old files via batch processing\n- Deferred lookups across versions: find in old files via batch processing\n- Compaction integrity: data correct after compact_all\n- Reopen after rotation: multi-version files survive reopen\n- Value integrity: byte-exact content for all container size boundaries\n- Erase persistence: deletions survive close/reopen",
          "timestamp": "2026-02-13T14:38:39+01:00",
          "tree_id": "1fe38ed3689a41b26a669afc813f550f56f17ed0",
          "url": "https://github.com/utxo-z/utxo-z/commit/a1ee9cb9aad91dd08854057619de0f7be764e816"
        },
        "date": 1770990036136,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 65.37,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 65.77,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 65.41,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 65.22,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.72,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 9.76,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.47,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.84,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.18,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.4,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.3,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.76,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.44,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.56,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.99,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.74,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.57,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2c716ce1be25c003fa4a9876f24851b39201299c",
          "message": "bench: Add storage overhead report and close/reopen benchmarks (#12)\n\nStorage overhead report fills each container to its rotation point\n(values at max capacity) and measures the structural overhead ratio:\nfile_size / (N * sizeof(pair)). Checks every insert for precise\nrotation detection.\n\nClose/reopen benchmarks measure the cost of serializing/deserializing\nthe map at different fill levels (useful for comparing master vs\nserialization branch).",
          "timestamp": "2026-02-13T22:08:06+01:00",
          "tree_id": "b73253172c14d25bb5a1766ef679d98d881b87d4",
          "url": "https://github.com/utxo-z/utxo-z/commit/2c716ce1be25c003fa4a9876f24851b39201299c"
        },
        "date": 1771017036358,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.66,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.37,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.43,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.4,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.47,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.33,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.39,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.2,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.53,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.69,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.43,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.15,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.21,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.57,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.79,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.22,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.46,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.59,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.34,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.49,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.5,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.51,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.46,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "89ce70aa5167a45b43b455c63102e869acc0b5cb",
          "message": "ci: Add release workflow and scripts (#14)\n\n- Extend ci.yml with release/hotfix branch triggers and version tags\n- Add setup job with determine-version action\n- Use dynamic version instead of hardcoded 0.0.0-ci\n- Add release.sh, post-release.sh, rollback-release.sh scripts\n- Add determine-version composite action",
          "timestamp": "2026-02-14T22:41:49+01:00",
          "tree_id": "c33f7e61983ceab619f499b184c6a36e58aa51e4",
          "url": "https://github.com/utxo-z/utxo-z/commit/89ce70aa5167a45b43b455c63102e869acc0b5cb"
        },
        "date": 1771105458724,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.31,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.37,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.48,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.39,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 32.64,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.17,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 44.87,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.09,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.72,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.53,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.43,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.16,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.29,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.75,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.34,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 44.94,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.1,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.78,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.42,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.45,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.2,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "89ce70aa5167a45b43b455c63102e869acc0b5cb",
          "message": "ci: Add release workflow and scripts (#14)\n\n- Extend ci.yml with release/hotfix branch triggers and version tags\n- Add setup job with determine-version action\n- Use dynamic version instead of hardcoded 0.0.0-ci\n- Add release.sh, post-release.sh, rollback-release.sh scripts\n- Add determine-version composite action",
          "timestamp": "2026-02-14T22:41:49+01:00",
          "tree_id": "c33f7e61983ceab619f499b184c6a36e58aa51e4",
          "url": "https://github.com/utxo-z/utxo-z/commit/89ce70aa5167a45b43b455c63102e869acc0b5cb"
        },
        "date": 1771105726539,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.48,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.5,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.28,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.47,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.28,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.26,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 44.64,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.14,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.5,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.81,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.48,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.17,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 44.93,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 35.25,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.4,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 46.02,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.81,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.84,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.96,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.85,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.59,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "322d0f603fba95753d67387e8dfbc819710be523",
          "message": "ci: Add release workflow and scripts (#14)\n\n- Extend ci.yml with release/hotfix branch triggers and version tags\n- Add setup job with determine-version action\n- Use dynamic version instead of hardcoded 0.0.0-ci\n- Add release.sh, post-release.sh, rollback-release.sh scripts\n- Add determine-version composite action",
          "timestamp": "2026-02-14T22:47:40+01:00",
          "tree_id": "07a7c5e8d26fe2c6a2d317d559352f6377a6c845",
          "url": "https://github.com/utxo-z/utxo-z/commit/322d0f603fba95753d67387e8dfbc819710be523"
        },
        "date": 1771105804879,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.72,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.97,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.93,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.98,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 34.24,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.49,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 46.05,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 46.16,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 28,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 35.35,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.91,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 46.23,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 46.22,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 35.17,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.44,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.87,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.89,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.93,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.83,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.84,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.86,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "c59cc4cc1927bf29e9a1f11a7a38dae8bddc76aa",
          "message": "release: update version to 0.1.0",
          "timestamp": "2026-02-14T22:56:50+01:00",
          "tree_id": "975c6c4e58c9066486244d4b2fd5d36b43fb3ae4",
          "url": "https://github.com/utxo-z/utxo-z/commit/c59cc4cc1927bf29e9a1f11a7a38dae8bddc76aa"
        },
        "date": 1771106359776,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.66,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.75,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.91,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.97,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.74,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.29,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.61,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.6,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.52,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.73,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.38,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.25,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.09,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.92,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.07,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.16,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.54,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.67,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.65,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.45,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.53,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "67c97fbac48255c1e6fcab4201ce61e5768996ac",
          "message": "fix: Clean working tree before benchmark gh-pages switch (#18)\n\nconfigure_file generates version.hpp in the source tree during build.\nWhen the benchmark action tries to git switch to gh-pages, it fails\nbecause version.hpp is dirty. Clean generated files before that step.",
          "timestamp": "2026-02-15T17:09:23+01:00",
          "tree_id": "6bec1efc0cfb9daf82efd4a0876edb7df85236ea",
          "url": "https://github.com/utxo-z/utxo-z/commit/67c97fbac48255c1e6fcab4201ce61e5768996ac"
        },
        "date": 1771171902810,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.74,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.7,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.9,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.77,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.96,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 9.79,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.4,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.58,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.84,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 35.01,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.49,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.63,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.59,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.9,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.24,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.7,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.6,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.73,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.76,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.74,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.75,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.72,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "87fc4a3c454aa90657fa2e72bdae911a113e31af",
          "message": "feat: Add sanitizer support (ASAN, UBSAN, TSAN) (#17)\n\nAdd build-time sanitizer configuration via CMake + Conan:\n- UTXOZ_SANITIZER cache variable in CMakeLists.txt\n- sanitizer option in conanfile.py (address, undefined, thread, or combined)\n- ASAN+UBSAN CI job (Linux GCC 15, Debug build)\n\nUsage: conan install . -o sanitizer=address,undefined -s build_type=Debug",
          "timestamp": "2026-02-15T17:16:28+01:00",
          "tree_id": "12c3eaa9296faac1f1a5daab848d2eec8cd3fea3",
          "url": "https://github.com/utxo-z/utxo-z/commit/87fc4a3c454aa90657fa2e72bdae911a113e31af"
        },
        "date": 1771172334414,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.94,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.58,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 64.12,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 34.31,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.56,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.9,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 46.18,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 28.06,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 35.36,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 46.36,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 46.13,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.98,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.82,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.26,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.46,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.97,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.99,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 69.15,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 69.06,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 69.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 69.09,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "9204d97f7184af9b7e6110f39db44d56fe93d38b",
          "message": "fix: Disable boost cobalt module and add sanitizer build script (#19)\n\n- Disable boost cobalt to work around Conan recipe bug where\n  boost_cobalt_io_ssl is built but not registered in Debug mode\n- Add scripts/build-sanitizer.sh for local sanitizer builds",
          "timestamp": "2026-02-15T17:28:09+01:00",
          "tree_id": "ad7f4b908e991bdc7329d92d8b3a3e593365de9b",
          "url": "https://github.com/utxo-z/utxo-z/commit/9204d97f7184af9b7e6110f39db44d56fe93d38b"
        },
        "date": 1771173024901,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 64.05,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.97,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.58,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.24,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.49,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.35,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.46,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 44.05,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.27,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.9,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.45,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.34,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.18,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.64,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.05,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.1,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.38,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.07,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.35,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.38,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.51,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "0a3f03545854c7a1b78ce96c827ed53e22357485",
          "message": "ci: Add WebAssembly (Emscripten) build (#20)\n\n- Add WASM build job to CI using emsdk/3.1.73 via Conan profile\n- Configure Boost as header-only and disable tests for Emscripten\n- Add scripts/build-wasm.sh for local WASM builds",
          "timestamp": "2026-02-15T18:31:55+01:00",
          "tree_id": "f43af4d07f6ba2c1264dde856eb3f8ff14c70d23",
          "url": "https://github.com/utxo-z/utxo-z/commit/0a3f03545854c7a1b78ce96c827ed53e22357485"
        },
        "date": 1771176867705,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 61.74,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 61.84,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 62.51,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 62.41,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 32.52,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 44.34,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 44.19,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.04,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.02,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 44.69,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 44.46,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 44.63,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.31,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 40.73,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 44.18,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 43.21,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 67.39,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 67.46,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 67.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 67.57,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 67.72,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 67.45,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "c473ae1bb2d8210a03b43ee540290a904445f3cb",
          "message": "release: update version to 0.2.0",
          "timestamp": "2026-02-15T19:26:23+01:00",
          "tree_id": "bdcdace7ebc9d884be7199e67586e5c5dd0756e2",
          "url": "https://github.com/utxo-z/utxo-z/commit/c473ae1bb2d8210a03b43ee540290a904445f3cb"
        },
        "date": 1771180135414,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.9,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.94,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.94,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.89,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.46,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.31,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.37,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.75,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.56,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.6,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.27,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.44,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 44.79,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.71,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.98,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.28,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.27,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 69.02,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.62,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 69.09,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.95,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "2014b625bd329dbe8a779968588522f571446c52",
          "message": "docs: update release notes for v0.2.0",
          "timestamp": "2026-02-15T19:31:10+01:00",
          "tree_id": "9c22bf95698df8113ff2fe83ad4d1659b1d61c43",
          "url": "https://github.com/utxo-z/utxo-z/commit/2014b625bd329dbe8a779968588522f571446c52"
        },
        "date": 1771180389103,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 125.47,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 124.86,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 124.75,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 125.54,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 63.65,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 16.82,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 83.98,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 82.63,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 45.24,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 63.57,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 83.85,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 84.18,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 83.34,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 64.47,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 79.1,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 85.74,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 83.55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 136.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 137.3,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 136.67,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 136.11,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 135.98,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 135.38,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "0cafb25bc3ccc332a1cf2cfbf6166a01675fcc48",
          "message": "release: update version to 0.2.0 (#22)",
          "timestamp": "2026-02-15T19:31:03+01:00",
          "tree_id": "bdcdace7ebc9d884be7199e67586e5c5dd0756e2",
          "url": "https://github.com/utxo-z/utxo-z/commit/0cafb25bc3ccc332a1cf2cfbf6166a01675fcc48"
        },
        "date": 1771180403150,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 64.61,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 64.06,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 64.1,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 64.11,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 34.17,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.48,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.76,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.83,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.91,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.56,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.28,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.64,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.63,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 35.24,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.66,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 46.18,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.95,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 69.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 69.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 69.01,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 69.21,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 69.38,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "2014b625bd329dbe8a779968588522f571446c52",
          "message": "docs: update release notes for v0.2.0",
          "timestamp": "2026-02-15T19:31:10+01:00",
          "tree_id": "9c22bf95698df8113ff2fe83ad4d1659b1d61c43",
          "url": "https://github.com/utxo-z/utxo-z/commit/2014b625bd329dbe8a779968588522f571446c52"
        },
        "date": 1771180421581,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 64.14,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.53,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.69,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.69,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.38,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.28,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.13,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.07,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.49,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.56,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.31,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 44.82,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 44.92,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.66,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.75,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.22,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.12,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.13,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.3,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.17,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.24,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.17,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.09,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "47a5bc02a5c964c2c4e10eeb8d9534ca73f50af9",
          "message": "feat: Add large-scale benchmarks with production file sizes (#23)\n\nAdd utxoz_benchmarks_large binary that uses db.configure() (2GB containers)\ninstead of configure_for_testing() (10MB). Includes IBD simulation (50M\ninserts, 30M erases across multiple generations) and individual operation\nmicro-benchmarks on 15-25M entry databases. CI skips these via\nwith_large_benchmarks=False.",
          "timestamp": "2026-02-16T11:48:01+01:00",
          "tree_id": "31290c2eafb523ff771bb88f3fadc887ebbc1527",
          "url": "https://github.com/utxo-z/utxo-z/commit/47a5bc02a5c964c2c4e10eeb8d9534ca73f50af9"
        },
        "date": 1771239021359,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.72,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.98,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 64.26,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 64.2,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 34.49,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.5,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 46.03,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 46.03,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.89,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 35.64,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 46.16,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.85,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 46.13,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 35.57,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.84,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 46.11,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 45.56,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.87,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.78,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.53,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.29,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.81,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ea6eda21c89a1cadcd4e1fa0434f7df523dcda81",
          "message": "test: Add edge case and regression tests for storage correctness (#24)\n\nAdd 15 new test cases covering:\n- Empty DB close/reopen\n- Single entry persistence per container size\n- Erase all + close/reopen\n- Multi-cycle with interleaved deletes\n- Compaction persistence (compact + close + reopen)\n- High fill near rotation point per container\n- Rotation + close/reopen with full cross-version verification\n- Duplicate insert rejection after reopen\n- Mixed container sizes with close/reopen\n- Size consistency across close/reopen cycles\n- Compaction with 3+ rotations (regression)\n- Compaction with deletions across multiple versions\n- Compaction size() correctness\n\nFix bug: entries_count_ was not initialized on reopen, causing\ndb.size() to return 0 after close + configure. Now counts entries\nfrom existing containers during configure_internal.\n\nAlso fix (void) casts for unused return values in existing compaction test.",
          "timestamp": "2026-02-16T16:24:20+01:00",
          "tree_id": "afe8f88021e1ff6cc46bc97c73c20b863b393fc7",
          "url": "https://github.com/utxo-z/utxo-z/commit/ea6eda21c89a1cadcd4e1fa0434f7df523dcda81"
        },
        "date": 1771255605383,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 62.95,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.42,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.07,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 62.55,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.07,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.33,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 44.87,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 44.74,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.43,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.06,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.14,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 44.81,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 44.78,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 33.47,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.4,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.04,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.42,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.34,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.76,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.53,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.53,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.73,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "fc75baeb4963fbc5eb5dd9f0b947937eee3d0247",
          "message": "fix: Implement metadata persistence (save/load to disk) (#25)\n\nsave_metadata_to_disk and load_metadata_from_disk were TODO stubs,\ncausing file_metadata_ to be empty after reopen. This meant the\nkey_in_range optimization in find_in_prev_versions was lost,\nforcing all previous version files to be searched on every\ndeferred lookup.\n\nImplement binary save/load of per-version metadata (min/max key\nranges, block height ranges, entry count). 88 bytes per meta file.\n\nAdd 3 tests for metadata persistence:\n- Meta files are created on disk after close\n- Meta files created for all versions on rotation\n- Key ranges survive close/reopen and deferred lookups work",
          "timestamp": "2026-02-16T16:38:28+01:00",
          "tree_id": "c40a926a30162df29ee29c2c0f8d2a6fc0a30ed1",
          "url": "https://github.com/utxo-z/utxo-z/commit/fc75baeb4963fbc5eb5dd9f0b947937eee3d0247"
        },
        "date": 1771256449326,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 64.3,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 64.26,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.93,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.86,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 34.47,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.57,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 46.16,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 46.05,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 28.1,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 35.57,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 46.08,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.83,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.99,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 35.45,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.67,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 46.03,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 45.16,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 69.07,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 69.05,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 69.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 69.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 69.01,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.7,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "cd99b33cb9107273449ddd3fdb5b66ca31fe5308",
          "message": "feat: Add sizing report for container/file size optimization (#26)\n\nAdd get_sizing_report() and print_sizing_report() to analyze waste per\ncontainer and value size distribution across the full UTXO set. Computed\nfrom existing statistics data with no hot-path overhead.",
          "timestamp": "2026-02-16T17:57:26+01:00",
          "tree_id": "8a52ca0de4a67cd0b260a4c55f641c7b19074e5b",
          "url": "https://github.com/utxo-z/utxo-z/commit/cd99b33cb9107273449ddd3fdb5b66ca31fe5308"
        },
        "date": 1771261190819,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.15,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.46,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.55,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.36,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.79,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.39,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.44,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.48,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.48,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.87,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.4,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.49,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.4,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.87,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.01,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.3,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.28,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.32,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.21,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.29,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.31,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.33,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.14,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7837bb1fbb81c470ebd52ba02e6ddc316a8fd2e3",
          "message": "fix: Route values by data capacity, not container size (#27)\n\nget_index_from_size() compared value.size() against container_sizes[i],\nbut utxo_value<Size> has overhead (block_height + size field) that\nreduces the actual data capacity. This caused silent truncation of\nvalues near the container limit — including 82% of all UTXOs (P2PKH\nat 43 bytes truncated to 39 in the 44-byte container).\n\nAdd data_capacity() and container_capacities[] to compute effective\nstorage per container, and use them for routing decisions.",
          "timestamp": "2026-02-16T18:18:12+01:00",
          "tree_id": "c738bbaea6f7e214c322479ea333f05c4d4f28d6",
          "url": "https://github.com/utxo-z/utxo-z/commit/7837bb1fbb81c470ebd52ba02e6ddc316a8fd2e3"
        },
        "date": 1771262432473,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.91,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 64.07,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.42,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.93,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.4,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 44.81,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 44.78,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.3,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.79,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.47,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.22,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.44,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.68,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.95,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.56,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 43.83,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.14,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 66.6,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 67.88,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.34,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.48,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.09,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7ea3985f1f9b6e441a32737d1d7d226c99da4460",
          "message": "fix: Adjust sizing report test expectations after routing fix (#28)\n\nThe truncation routing fix (#27) changed where 40-byte values land:\nthey now go to container 1 (capacity 123) instead of container 0\n(capacity 39). Update test expectations accordingly and replace\nhardcoded loop bounds with container_count.",
          "timestamp": "2026-02-16T19:28:09+01:00",
          "tree_id": "c9564efd6f3e51d253bf50b0c0db997a4973f438",
          "url": "https://github.com/utxo-z/utxo-z/commit/7ea3985f1f9b6e441a32737d1d7d226c99da4460"
        },
        "date": 1771266626414,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.23,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.21,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.24,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.32,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.44,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.34,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 44.98,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.28,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.28,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.29,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.37,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.38,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.74,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 35.03,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.18,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.19,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.5,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.21,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.26,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.43,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.3,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.22,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "bb5dbe1b8bfc6be00fafa9b8f1c02e377412c4f1",
          "message": "fix: Update entries_count_ and container stats on deferred deletions (#29)\n\nprocess_deferred_deletions_in_file() erased entries from the map but\ndid not update container_stats current_size/total_deletes, and\nprocess_pending_deletions() did not decrement entries_count_. This\ncaused db.size() and statistics to under-count deletions, with the\nerror growing proportionally to the number of deferred deletions.",
          "timestamp": "2026-02-16T20:27:03+01:00",
          "tree_id": "fa9d117db2c6405a8cd3298832e57ecefdc5ab76",
          "url": "https://github.com/utxo-z/utxo-z/commit/bb5dbe1b8bfc6be00fafa9b8f1c02e377412c4f1"
        },
        "date": 1771270165301,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.74,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 64.05,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.86,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.92,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 34.17,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.46,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.64,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.56,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.62,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.94,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.76,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.54,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.62,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.87,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 42.07,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.91,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.88,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.57,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 68.44,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 68.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 68.73,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 68.74,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "657b6433d0db8cdd57b326bf115a1c6b4e1a4ed6",
          "message": "release: update version to 0.3.0",
          "timestamp": "2026-02-17T13:02:42+01:00",
          "tree_id": "15dd978c2444581371d2251c563ad6b4246cb071",
          "url": "https://github.com/utxo-z/utxo-z/commit/657b6433d0db8cdd57b326bf115a1c6b4e1a4ed6"
        },
        "date": 1771329898508,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 64.79,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 64.55,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 64.96,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 65.05,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.31,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 9.68,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 44.84,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 44.45,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 26.55,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.2,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.35,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.19,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 45.22,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 33.84,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.88,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.79,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.94,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 70.24,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 70.57,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 70.42,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 70.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 70.48,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 70.69,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "324cac2696c967729dc4ead0daca0885a2e55a55",
          "message": "docs: update release notes for v0.3.0",
          "timestamp": "2026-02-17T13:14:29+01:00",
          "tree_id": "b0454ce9cde7f9f7eae3b155f621d403b68b39e8",
          "url": "https://github.com/utxo-z/utxo-z/commit/324cac2696c967729dc4ead0daca0885a2e55a55"
        },
        "date": 1771330611192,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.84,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.41,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.58,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.53,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 33.76,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.29,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 45.45,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 45.08,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 27.24,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 34.46,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.07,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 45.19,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 44.85,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.73,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.99,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 45.46,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.32,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 68.36,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 67.27,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 67.28,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 67.62,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 67.3,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8327bac2b86be1f7cdb6d16aa5509280f25e2fe7",
          "message": "release: update version to 0.3.0 (#31)",
          "timestamp": "2026-02-17T13:14:22+01:00",
          "tree_id": "15dd978c2444581371d2251c563ad6b4246cb071",
          "url": "https://github.com/utxo-z/utxo-z/commit/8327bac2b86be1f7cdb6d16aa5509280f25e2fe7"
        },
        "date": 1771330615239,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.61,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.53,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.07,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.42,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 32.58,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 10.27,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 44.95,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 44.75,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 26.7,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 33.84,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 45.03,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 44.99,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 44.69,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 34.5,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 41.18,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 44.07,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.26,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 68.26,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 67.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 67.57,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 67.73,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 67.18,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 67.96,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "324cac2696c967729dc4ead0daca0885a2e55a55",
          "message": "docs: update release notes for v0.3.0",
          "timestamp": "2026-02-17T13:14:29+01:00",
          "tree_id": "b0454ce9cde7f9f7eae3b155f621d403b68b39e8",
          "url": "https://github.com/utxo-z/utxo-z/commit/324cac2696c967729dc4ead0daca0885a2e55a55"
        },
        "date": 1771330623590,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 63.43,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 63.64,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 63.48,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 63.74,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 31.15,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 9.22,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 43.3,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 43.75,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 26.2,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 32.33,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 43.56,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 43.21,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 43.55,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 32.79,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 40.04,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 43.6,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 44.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 69.74,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 69.98,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 69.66,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 69.18,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 69.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 69.5,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "848dfa551275afee6558ca57caeba837be8d2993",
          "message": "fix: Rebuild metadata files after compaction (#32)\n\nCompaction moves entries between data files and deletes/renumbers\nthem, but metadata files were not updated, leaving stale or\norphaned meta_X_YYYYY.dat files on disk.\n\nAfter compaction finishes reorganizing data files, delete all old\nmetadata files for the container, scan remaining data files to\nrebuild metadata from actual entries, and save to disk.",
          "timestamp": "2026-02-17T17:37:39+01:00",
          "tree_id": "713372fc37715e03b650c314ebc8a6efee0074dd",
          "url": "https://github.com/utxo-z/utxo-z/commit/848dfa551275afee6558ca57caeba837be8d2993"
        },
        "date": 1771346386819,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert 44B value",
            "value": 120.94,
            "unit": "ops/sec"
          },
          {
            "name": "insert 128B value",
            "value": 122.48,
            "unit": "ops/sec"
          },
          {
            "name": "insert 512B value",
            "value": 122.05,
            "unit": "ops/sec"
          },
          {
            "name": "insert 10KB value",
            "value": 122.24,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (44B)",
            "value": 58.76,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (mixed sizes)",
            "value": 16.27,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 80.16,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 80.79,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (mixed sizes)",
            "value": 44.29,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 60.78,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 80.89,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 80.65,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 81.58,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 60.81,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 73.54,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 79.64,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 77.6,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (44B)",
            "value": 131.53,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (44B)",
            "value": 131.55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (44B)",
            "value": 131.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (44B)",
            "value": 131.53,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (128B)",
            "value": 132.04,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (128B)",
            "value": 132.1,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "55c910a11be56f64a41dd193690376dca76cd0f9",
          "message": "feat: 5-container layout, height range stats, key/entry iteration (#30)\n\n- Change container layout from 4 to 5 containers {48, 94, 128, 256, 10240}\n  to better match BCH chain value size distribution\n- Add height range stats tracking inserts/deletes per 10K-block ranges\n- Add for_each_key and for_each_entry iteration over all stored entries\n- Update all benchmarks to use realistic BCH chain value distribution\n  (82% P2PKH 43B, 13% P2SH 41B, 4% 123B, 1% 89B)\n- Fix benchmark fixture: create once outside nanobench loop to measure\n  pure operation cost without open/close overhead per iteration\n- Dynamic storage overhead report adapts to any container count",
          "timestamp": "2026-02-18T11:26:17+01:00",
          "tree_id": "37328c5ef2af9d27e852cd081cbefc1d8b179054",
          "url": "https://github.com/utxo-z/utxo-z/commit/55c910a11be56f64a41dd193690376dca76cd0f9"
        },
        "date": 1771410502979,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 295079.55,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 319743.04,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 315339.64,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 484400.92,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 470.13,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 508.2,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12183440.14,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12799010.92,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12378229.71,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13471.52,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12773859.44,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 12487599.01,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 107387.48,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12911.23,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2264.32,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3232.92,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2499.01,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.38,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.5,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.23,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.19,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "13bd8498a3f820ecdefae6bb2ab7db3cff0ecccb",
          "message": "docs: update README for 0.3.0 (#33)\n\n- Update container layout to 5 containers (48B, 94B, 128B, 256B, 10KB)\n- Update version references from 0.1.0 to 0.3.0\n- Fix make_key -> make_outpoint, key_t -> raw_outpoint\n- Add for_each_key/for_each_entry usage example\n- Add deferred lookups, sizing report, height range stats to API table\n- Add CI benchmark tracking section with variability disclaimer\n- Add instructions for running benchmarks locally",
          "timestamp": "2026-02-18T11:46:55+01:00",
          "tree_id": "215652e3bfe2a7e753f2233eac7edc9bd216253f",
          "url": "https://github.com/utxo-z/utxo-z/commit/13bd8498a3f820ecdefae6bb2ab7db3cff0ecccb"
        },
        "date": 1771411744202,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 265014.2,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 375233.02,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 340096.17,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 579041.79,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 476.49,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 453.33,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11624125.87,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12955678.88,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12048135.64,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13313.76,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13638022.33,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14482311.41,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 99316.52,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 10263.02,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2611.69,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3093.47,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2770.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.58,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.93,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.43,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.49,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "20b49bf13abe4a8cb56a1ae5e2a19233a303b032",
          "message": "release: update version to 0.4.0",
          "timestamp": "2026-02-18T13:47:50+01:00",
          "tree_id": "64cd690e280f0231a77f6eeb1f263a5815bea598",
          "url": "https://github.com/utxo-z/utxo-z/commit/20b49bf13abe4a8cb56a1ae5e2a19233a303b032"
        },
        "date": 1771418986969,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 277169.9,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 354891.72,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 275411.77,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 519202.95,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 501.22,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 460.05,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12091490.1,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 8206444.64,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12143365.13,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13977.84,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 14539254.14,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 16208507,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 121374.52,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13700.56,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2453.93,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3548.67,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2887.67,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 55.2,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.1,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.46,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 55.15,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 55.07,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 55.18,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "b9bc2710e6aa8d01e8eb6f6e96be30a658d211ca",
          "message": "release: update version to 0.4.0",
          "timestamp": "2026-02-18T13:54:25+01:00",
          "tree_id": "64cd690e280f0231a77f6eeb1f263a5815bea598",
          "url": "https://github.com/utxo-z/utxo-z/commit/b9bc2710e6aa8d01e8eb6f6e96be30a658d211ca"
        },
        "date": 1771419390698,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 276351.82,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 277356.45,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 161080.65,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 434934.25,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 490.8,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 465.95,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11862138.51,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12075963.62,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12198501.62,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13519.24,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13839273.16,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15787605.73,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 119453.24,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12067.34,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2174.29,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3261.79,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2844.44,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.96,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.96,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.09,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.71,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.8,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "01d047cadc93e2b42f4d1b3e1470434141e8b977",
          "message": "release: update version to 0.4.0 (#34)",
          "timestamp": "2026-02-18T14:03:40+01:00",
          "tree_id": "64cd690e280f0231a77f6eeb1f263a5815bea598",
          "url": "https://github.com/utxo-z/utxo-z/commit/01d047cadc93e2b42f4d1b3e1470434141e8b977"
        },
        "date": 1771419947149,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 305327.06,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 286361.39,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 268251.25,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 552647.18,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 472.16,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 487.17,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12031484.15,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13247236.83,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12025070.31,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13170,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13978067.26,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15714764.76,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 105903.37,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12740.84,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2196.14,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3159.35,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2878.34,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.51,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.02,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.24,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.98,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "5103b7adc01eafe56f56b37d4b75185ce13e87a9",
          "message": "docs: update release notes for v0.4.0",
          "timestamp": "2026-02-18T14:03:48+01:00",
          "tree_id": "5caf3644220890bef936c45ecd1dd19730cedee1",
          "url": "https://github.com/utxo-z/utxo-z/commit/5103b7adc01eafe56f56b37d4b75185ce13e87a9"
        },
        "date": 1771419952802,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 266537.68,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 275961.82,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 256463.29,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 365083.85,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 389.96,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 424.58,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12485744.71,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12511092.59,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12288831.34,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11934.56,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 7958822.49,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 9015088.88,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 81180.79,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 10452,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2133.02,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2780.72,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2942.49,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.07,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.98,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.81,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.76,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.05,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "5103b7adc01eafe56f56b37d4b75185ce13e87a9",
          "message": "docs: update release notes for v0.4.0",
          "timestamp": "2026-02-18T14:03:48+01:00",
          "tree_id": "5caf3644220890bef936c45ecd1dd19730cedee1",
          "url": "https://github.com/utxo-z/utxo-z/commit/5103b7adc01eafe56f56b37d4b75185ce13e87a9"
        },
        "date": 1771419956555,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 277882.36,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 288790.42,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 299162.35,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 380425.26,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 408.2,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 478.4,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12194509.47,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13749577.05,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13157323.67,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13288.94,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 11147181.53,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13868632.04,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 118486.42,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13767.69,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 3008.32,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3585.29,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2945.88,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 55.49,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.38,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.23,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.99,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 55.08,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a90440f89b573f5e9c021a961531c07019a9c009",
          "message": "ci: use statistical comparison for benchmark alerts (#36)\n\nReplace single-commit comparison (alert-threshold: 120%) with median\nof the last 10 commits. This reduces false regression alerts caused\nby GHA runner performance variability.\n\n- Add scripts/benchmark_compare.py: compares current results against\n  historical median with configurable window and threshold (±15%)\n- Post benchmark report as sticky PR comment and step summary\n- Keep github-action-benchmark for data storage and charting only\n- Skip sticky comment on forked PRs (read-only token)\n- Validate inputs and handle malformed data.js gracefully",
          "timestamp": "2026-03-19T14:12:16+01:00",
          "tree_id": "d7827bb0ad1a657292195e67226957cb9907e5b0",
          "url": "https://github.com/utxo-z/utxo-z/commit/a90440f89b573f5e9c021a961531c07019a9c009"
        },
        "date": 1773926077317,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 270253.23,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 242486.7,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 248026.86,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 309638.75,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 378.41,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 444.68,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11223958.45,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12591301.62,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11501114.29,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11011.23,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 7515247.74,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 12450816,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 65504.19,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 4997.92,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2658.21,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3259.84,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2767.46,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.33,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.94,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.63,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.83,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.82,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.88,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "180a4cc59a505930dd66848e5edbd891d05584d5",
          "message": "fix: count all versions in entries_count_ on reopen and reset impl_ on close (#37)\n\n- entries_count_ now includes entries from previous (non-compacted) versions,\n  not just the active container. Prevents undercount after reopen and\n  potential underflow on deletes from historical versions.\n- close() resets impl_ to release mapped files and prevent use-after-close.\n  A subsequent configure() will create a fresh impl_ as expected.",
          "timestamp": "2026-03-19T16:49:02+01:00",
          "tree_id": "0dc8a57d7e825478b21a5aff3ca35e5e8655c928",
          "url": "https://github.com/utxo-z/utxo-z/commit/180a4cc59a505930dd66848e5edbd891d05584d5"
        },
        "date": 1773935472526,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 275860.17,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 321526.38,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 331128.26,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 491874.79,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 474.71,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 543.32,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12132361.27,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13733306.56,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13085376.16,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13774.54,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 14307096.9,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15395947.09,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 131672.27,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14189.03,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2386.34,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3452.93,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2648.14,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.82,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.74,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.94,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.87,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.93,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "9ecc01866f8c6398f9e15a97db2020d4d51aff05",
          "message": "ci: update actions/checkout v4 to v5 (Node.js 24) (#40)\n\nSilences the Node.js 20 deprecation warnings. Actions will be forced\nto run with Node.js 24 by default starting June 2nd, 2026.",
          "timestamp": "2026-03-19T16:50:40+01:00",
          "tree_id": "434c6b72a2b60ba1d1fa5827c923dbe818ad7002",
          "url": "https://github.com/utxo-z/utxo-z/commit/9ecc01866f8c6398f9e15a97db2020d4d51aff05"
        },
        "date": 1773935562958,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 273296.38,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 286309.3,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 214177.19,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 389298.7,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 493.15,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 448.12,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11737172.82,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11653173.86,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11514511.19,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12383.04,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12016542.07,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 10607479.91,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 91975.64,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 10515.58,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2566.84,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3127.12,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2725.83,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.35,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.57,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.5,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.1,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.76,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f251ed2e9f2bacd7ef2f2ccbd0c535bad707b975",
          "message": "feat: add compact storage mode with type-safe split API (#35)\n\nAdd storage_mode::compact for non-pruned nodes that can recover full\nUTXO data from block files. In compact mode, UTXO-Z stores only a\nsmall fixed-size reference (file_number + offset) in a single container,\nreducing storage from ~5 GiB to ~3.6 GiB for 60M UTXOs.\n\nSplit the monolithic `db` class into a type-safe hierarchy:\n- `db_base`: shared methods (close, size, erase, statistics, iteration)\n- `full_db`: variable-size byte values, returns `full_find_result`\n- `compact_db`: typed insert(key, file_number, offset, height),\n  returns `compact_find_result` with named fields — no byte serialization\n- `using db = full_db;` preserves backward compatibility\n\nInternal changes:\n- Add storage_mode enum and compact constants to types.hpp\n- Add full_find_result / compact_find_result result types\n- Add typed methods to database_impl (compact_insert_typed,\n  compact_find_typed, compact_process_pending_lookups, etc.)\n- Implement single-container architecture with dedicated compact_v*.dat files\n- Add config persistence (utxoz_config.dat) with mode mismatch detection\n- Update all tests for the new class hierarchy\n- Update README with new API documentation",
          "timestamp": "2026-03-19T17:21:21+01:00",
          "tree_id": "34bfa6016d3052232721118fc3095614a9d16aea",
          "url": "https://github.com/utxo-z/utxo-z/commit/f251ed2e9f2bacd7ef2f2ccbd0c535bad707b975"
        },
        "date": 1773937417489,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 275562.75,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 298272.45,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 321232.1,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 413582.03,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 403.89,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 449.4,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11502543.98,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11855073.11,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12601460.13,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13925.95,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12612318.22,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14280774.09,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 109111.89,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12041.61,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2377.06,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3035.12,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3129.97,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 53.97,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.44,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.66,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.98,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f4a0ed32ebbc9e776a56c20af9e89a1e1ae8b7ea",
          "message": "refactor: replace exceptions with std::expected in public API (#42)\n\n- Add error_code enum (not_configured, storage_mode_mismatch,\n  config_file_corrupt, value_too_large) and error struct to types.hpp\n- Add result<T> = std::expected<T, error> alias\n- configure() returns result<> instead of throwing on mode mismatch\n  or corrupt config\n- insert() returns result<bool> instead of throwing on unconfigured\n  db or value too large\n- load_config_from_disk() returns result<> instead of throwing\n- Keep boost::interprocess::bad_alloc (OOM after retries) as exception\n  since it's irrecoverable\n- Update all tests and examples to use .value() or check error codes",
          "timestamp": "2026-03-19T17:58:30+01:00",
          "tree_id": "66e0f0a70b9cb38e6ad4c64ea9bd35f0c8de9b11",
          "url": "https://github.com/utxo-z/utxo-z/commit/f4a0ed32ebbc9e776a56c20af9e89a1e1ae8b7ea"
        },
        "date": 1773939643837,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 290119.63,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 390012.36,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 298006.94,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 539801.71,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 496.85,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 507.98,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12672588.54,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13436884.29,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13332853.94,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13505.21,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 10792873.85,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14941456.28,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 130755.68,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13819.77,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2648.63,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3090.65,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2920.85,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 53.93,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.3,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.33,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.45,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.98,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "0e5cc82c67a7c20192c874d0fcf0522b9404f28e",
          "message": "release: update version to 0.5.0",
          "timestamp": "2026-03-19T17:59:28+01:00",
          "tree_id": "da28e0c2cc2b5bc1f2d39064abe89cc700fafb0f",
          "url": "https://github.com/utxo-z/utxo-z/commit/0e5cc82c67a7c20192c874d0fcf0522b9404f28e"
        },
        "date": 1773939696626,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 272680.08,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 280131.66,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 267328.38,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 386569.47,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 380.83,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 467.93,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12775646.33,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13426850.32,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12052418.59,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13179.73,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 7867821.19,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13822515.8,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 97238.06,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 10175.11,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2449.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3255.53,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2904.46,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 53.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.48,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.19,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.16,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "358ba7946c0cc79ec77b4b8feb7f88cab158f0dc",
          "message": "release: update version to 0.5.0 (#44)",
          "timestamp": "2026-03-19T18:51:36+01:00",
          "tree_id": "da28e0c2cc2b5bc1f2d39064abe89cc700fafb0f",
          "url": "https://github.com/utxo-z/utxo-z/commit/358ba7946c0cc79ec77b4b8feb7f88cab158f0dc"
        },
        "date": 1773942821012,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 297724.53,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 316065.84,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 229288.77,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 415833.27,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 395.77,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 433.26,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12925856.63,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12135686.5,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12273371.52,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13300.56,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 10157480.65,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 11568737.18,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 90855.65,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 7671.63,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2229.99,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2950.35,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2898.05,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.37,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.67,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.05,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.4,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.89,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "3dcf90eb325e7b6aa724bb31562776ffecfdd6a1",
          "message": "docs: update release notes for v0.5.0",
          "timestamp": "2026-03-19T18:51:44+01:00",
          "tree_id": "d42c1f71fd0a7d85d48ac576b4d3c296fdd7c18a",
          "url": "https://github.com/utxo-z/utxo-z/commit/3dcf90eb325e7b6aa724bb31562776ffecfdd6a1"
        },
        "date": 1773942833814,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 313007.06,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 296701.96,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 257941.63,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 481812.72,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 489.19,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 474.89,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11773373.58,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12043828.32,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 9066710,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12974.54,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12472946.28,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13734112.05,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 117993.05,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13013.45,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2760.88,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3305.93,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3074.88,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 55.15,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.4,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.38,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 55.41,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 55.21,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 55.36,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "3dcf90eb325e7b6aa724bb31562776ffecfdd6a1",
          "message": "docs: update release notes for v0.5.0",
          "timestamp": "2026-03-19T18:51:44+01:00",
          "tree_id": "d42c1f71fd0a7d85d48ac576b4d3c296fdd7c18a",
          "url": "https://github.com/utxo-z/utxo-z/commit/3dcf90eb325e7b6aa724bb31562776ffecfdd6a1"
        },
        "date": 1773942838501,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 347650.68,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 359507.35,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 386875.78,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 407186.22,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 455.05,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 487.12,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12760324.02,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 10149802.56,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12251437.62,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13753.53,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12242452.37,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 12193415.84,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 105362.29,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 9424.34,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2695.77,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2949.15,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3270.85,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.4,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.65,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.3,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.12,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.73,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.88,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "756e14aca03c74a2678e4b67387aafab00c09943",
          "message": "fix: suppress nodiscard warnings and fix release script workflow lookup (#45)\n\n- Add [[maybe_unused]] for erase() and process_pending_deletions()\n  return values in test_compact_mode.cpp\n- Use --workflow ci.yml instead of --workflow \"Build and Test\" in\n  release.sh to avoid ambiguity with duplicate workflow names",
          "timestamp": "2026-03-19T19:07:40+01:00",
          "tree_id": "b741e555ae65ea5bb6513f5bce38050d8e114c5f",
          "url": "https://github.com/utxo-z/utxo-z/commit/756e14aca03c74a2678e4b67387aafab00c09943"
        },
        "date": 1773943797806,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 255276.4,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 191620.7,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 241116.64,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 332342.26,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 359.05,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 392.54,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12450067.48,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12422718.37,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12424147.44,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13556.06,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 6749656.23,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 12406324.14,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 60063.26,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 6468.18,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1782,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2785.83,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2862.62,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 53.16,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 52.89,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.07,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.06,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 52.74,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.09,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "aa509b97cfb2937c6b57157dd08118957ccfaf7f",
          "message": "refactor: replace default ctor + configure() with named constructors (#46)\n\n- Add static full_db::open() / open_for_testing() returning result<full_db>\n- Add static compact_db::open() / open_for_testing() returning result<compact_db>\n- Make default constructors private — objects are always configured\n- Destructor calls close() automatically (safe to call multiple times)\n- Remove not_configured error code (no longer possible)\n- Remove configure() / configure_for_testing() from public API\n- Update all tests, examples, and README\n\nCloses #43",
          "timestamp": "2026-03-19T21:36:13+01:00",
          "tree_id": "02c0ef835bd08380b3b4bcd4d18b33e7951cc519",
          "url": "https://github.com/utxo-z/utxo-z/commit/aa509b97cfb2937c6b57157dd08118957ccfaf7f"
        },
        "date": 1773952700428,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 282367.44,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 394872.36,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 328880.22,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 576063.53,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 495.97,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 538.31,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12357115.29,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13544910.3,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12369092.32,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13872.68,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 14372832.31,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15171435.2,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 135299.39,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13673.78,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2415.66,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3506.13,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2726.91,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.42,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.16,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.67,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.43,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.41,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.46,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "605e76a8ff13f9bdccf724d8145b8f8f8f0ad6f0",
          "message": "release: update version to 0.6.0",
          "timestamp": "2026-03-19T21:37:45+01:00",
          "tree_id": "74bbec3b63edade08d9b0f358be5c5c8cd3dd301",
          "url": "https://github.com/utxo-z/utxo-z/commit/605e76a8ff13f9bdccf724d8145b8f8f8f0ad6f0"
        },
        "date": 1773952787691,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 313667.13,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 319297.69,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 263460.43,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 398530.22,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 499,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 456.53,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12991575.57,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13880272.75,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13498985.62,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13598.92,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13593409.87,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15668362.96,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 134602.4,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13691.96,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2447.93,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3546.3,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2696.67,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.98,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.04,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.98,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.89,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b5597a1bce0485d66998ddab77926c6a2c3d2962",
          "message": "release: update version to 0.6.0 (#48)",
          "timestamp": "2026-03-19T23:12:50+01:00",
          "tree_id": "74bbec3b63edade08d9b0f358be5c5c8cd3dd301",
          "url": "https://github.com/utxo-z/utxo-z/commit/b5597a1bce0485d66998ddab77926c6a2c3d2962"
        },
        "date": 1773958487632,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 273361.72,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 390377.8,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 241604.25,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 488341.42,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 451.65,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 500.55,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12474647.32,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13907142.38,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13582771.52,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13545.19,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13436327.95,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15113499.9,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 114180.61,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13243.35,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2728.12,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3320.02,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2962.68,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.38,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.23,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.24,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.2,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.05,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.06,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "2f14d7dd53e0a5e16f9729e5ea241afd14a22089",
          "message": "docs: update release notes for v0.6.0",
          "timestamp": "2026-03-19T23:12:57+01:00",
          "tree_id": "492794f6702dfa5fb3e94cfe9ae2cef35d7d258e",
          "url": "https://github.com/utxo-z/utxo-z/commit/2f14d7dd53e0a5e16f9729e5ea241afd14a22089"
        },
        "date": 1773958497315,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 1166607.13,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 1184529.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 1319353.73,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1376599.47,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 425.53,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 476.38,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12453593.46,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 6385744.07,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11995936.57,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11894.08,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 11414793.5,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 10478792.79,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 119694.46,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12321.84,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2639.13,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3933.94,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3322.04,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 104.28,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 105.26,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 105.68,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 105.83,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 105.67,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 106.28,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "2f14d7dd53e0a5e16f9729e5ea241afd14a22089",
          "message": "docs: update release notes for v0.6.0",
          "timestamp": "2026-03-19T23:12:57+01:00",
          "tree_id": "492794f6702dfa5fb3e94cfe9ae2cef35d7d258e",
          "url": "https://github.com/utxo-z/utxo-z/commit/2f14d7dd53e0a5e16f9729e5ea241afd14a22089"
        },
        "date": 1773958501246,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 242904.53,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 258905.96,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 237948.29,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 399443.15,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 456.71,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 429.59,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11561179.5,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11879253.42,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11710969.7,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13262.56,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 9676314.06,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13391099.85,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 97812.62,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 9775.07,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2301.75,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2906.12,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3023.45,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.16,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.62,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.72,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 55.53,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 55.65,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 55.59,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b7a3c64270e9278e6004b150c14c004d694fcb0e",
          "message": "ci: add Conan package publishing on release branches (#49)\n\nPublish utxoz package to kth Conan remote (packages.kth.cash) on:\n- Tag pushes (v*)\n- Release branches (release/**)\n- Hotfix branches (hotfix/**)\n\nBuilds clean packages (no tests/examples/benchmarks) for Linux, macOS,\nand Windows after all build jobs pass.",
          "timestamp": "2026-03-21T11:20:24+01:00",
          "tree_id": "5d6d10ca8f72c512fbaa2cf855a4639714610da6",
          "url": "https://github.com/utxo-z/utxo-z/commit/b7a3c64270e9278e6004b150c14c004d694fcb0e"
        },
        "date": 1774088553557,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 310434.24,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 366782.77,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 371812.68,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 498270.16,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 491.95,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 459.46,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12973953.3,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13481221.21,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12506047.99,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13494.94,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13750853.92,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14539113.79,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 135529.36,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13677.83,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2656.84,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3245.17,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2971.8,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 55.19,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 55.04,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.81,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.96,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "ee5098a0c4f616e7d8f63d43c81cd2dc1d103f17",
          "message": "release: update version to 0.7.0",
          "timestamp": "2026-03-21T11:21:09+01:00",
          "tree_id": "2ec4730271d00421b6f058e1e083dd3eb4a0406d",
          "url": "https://github.com/utxo-z/utxo-z/commit/ee5098a0c4f616e7d8f63d43c81cd2dc1d103f17"
        },
        "date": 1774088596445,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 295789.16,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 315848.58,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 336109.89,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 481227.37,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 493.23,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 473.58,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11979597.89,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13295638.56,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13424702.86,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13431.43,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13898617.93,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15233471.07,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 106737.46,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13749.15,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2358.35,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3241.96,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2696.89,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 53.56,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.66,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.63,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.59,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.64,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8e1d01f6b9b854f52951c3433fa47636b311f244",
          "message": "release: update version to 0.7.0 (#50)",
          "timestamp": "2026-03-21T11:34:22+01:00",
          "tree_id": "2ec4730271d00421b6f058e1e083dd3eb4a0406d",
          "url": "https://github.com/utxo-z/utxo-z/commit/8e1d01f6b9b854f52951c3433fa47636b311f244"
        },
        "date": 1774089395262,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 278811.22,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 285414.27,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 264185.99,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 407204.26,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 468.14,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 453.18,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12638360.77,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12224043.98,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12090116.04,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13293.94,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 11028237.01,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13730547.78,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 118903.46,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13416.01,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2740.49,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3382.77,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2967.04,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.35,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.2,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.2,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.4,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.19,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.2,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "7088a3757f0a1e5fc141fceb412b566113c7f83d",
          "message": "docs: update release notes for v0.7.0",
          "timestamp": "2026-03-21T11:34:28+01:00",
          "tree_id": "a11b5c78cb986286b483424c52a4656913a29225",
          "url": "https://github.com/utxo-z/utxo-z/commit/7088a3757f0a1e5fc141fceb412b566113c7f83d"
        },
        "date": 1774089402764,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 268069.56,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 270245.08,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 260690.64,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 410188.93,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 413.44,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 497.36,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12730810.69,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12961579.74,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13522655.51,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12642.1,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 9262822.37,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 12323995.96,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 102935.43,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 9500.79,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2761.55,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3420.34,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2965.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.41,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.27,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.91,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.33,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.96,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "7088a3757f0a1e5fc141fceb412b566113c7f83d",
          "message": "docs: update release notes for v0.7.0",
          "timestamp": "2026-03-21T11:34:28+01:00",
          "tree_id": "a11b5c78cb986286b483424c52a4656913a29225",
          "url": "https://github.com/utxo-z/utxo-z/commit/7088a3757f0a1e5fc141fceb412b566113c7f83d"
        },
        "date": 1774089407673,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 302139.98,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 117381.99,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 272284.7,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 255993.86,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 389.59,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 474.15,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12109551.7,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12011706.07,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12298318.6,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13158.69,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 9088162.87,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 11854723.79,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 82812.23,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 9308.74,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2575.45,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3371.25,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2968.76,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.23,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.71,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.29,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.71,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.35,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "05e737a4fdee8d275de551eadf6f8365b4a45cca",
          "message": "chore(deps): bump third-party libs to latest CCI (#51)\n\n- boost 1.90.0 → 1.91.0\n- fmt 12.0.0 → 12.1.0\n- spdlog 1.16.0 → 1.17.0 (bundled fmt now 12.1.0)\n- catch2 3.11.0 → 3.15.0\n\nnanobench held at 4.3.11 (already latest CCI). The existing\nwithout_cobalt=True option already handles the CCI 1.91.0 cobalt_io_ssl\nrecipe bug.",
          "timestamp": "2026-06-16T15:22:15+02:00",
          "tree_id": "5aafdbe341e977eff6ce979f4d5367fd433a432c",
          "url": "https://github.com/utxo-z/utxo-z/commit/05e737a4fdee8d275de551eadf6f8365b4a45cca"
        },
        "date": 1781616267025,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 277466.46,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 279261.67,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 265628.58,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 270072.15,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 341.2,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 359.28,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12144607.26,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11295460.35,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11666577.97,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13488.67,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 8751620.71,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13428897.27,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 60298.24,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 6484.67,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1998.67,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2831.6,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2906.72,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 58.58,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 58.55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 58.96,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 58.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 58.39,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 58.4,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "7d29adf66da8ee16f93962aa77846465da3c865e",
          "message": "release: update version to 0.8.0",
          "timestamp": "2026-06-16T16:52:58+02:00",
          "tree_id": "2dde98e2e0297858915e3df32326233d9a42d87f",
          "url": "https://github.com/utxo-z/utxo-z/commit/7d29adf66da8ee16f93962aa77846465da3c865e"
        },
        "date": 1781621715230,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 273043.97,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 375029.69,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 378280.86,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 452187.79,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 470.31,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 473.06,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12049346.93,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12466672.05,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12004433.14,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13329.61,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12486523.28,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15520536.4,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 116020.46,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12551.18,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2802.34,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3388.1,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2985.2,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 64.42,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 59.75,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 59.61,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 59.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 59.5,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 59.66,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "be88578167e71b7f46cd410d1674859299152504",
          "message": "docs: update release notes for v0.8.0",
          "timestamp": "2026-06-16T17:12:51+02:00",
          "tree_id": "b5d2e51ae6f4c7260f293279e2cf73036ce569d1",
          "url": "https://github.com/utxo-z/utxo-z/commit/be88578167e71b7f46cd410d1674859299152504"
        },
        "date": 1781622906645,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 272397.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 269844.84,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 269216.49,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 347681.75,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 353.8,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 354.98,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11496414.98,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12797968.99,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12812917.38,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12981.7,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 9421124.36,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 12732187.64,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 60293.97,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 10640.93,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2086.72,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2730.97,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3002.27,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 59.44,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 59.7,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 59.3,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 59.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 59.07,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 57.99,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "be88578167e71b7f46cd410d1674859299152504",
          "message": "docs: update release notes for v0.8.0",
          "timestamp": "2026-06-16T17:12:51+02:00",
          "tree_id": "b5d2e51ae6f4c7260f293279e2cf73036ce569d1",
          "url": "https://github.com/utxo-z/utxo-z/commit/be88578167e71b7f46cd410d1674859299152504"
        },
        "date": 1781622911563,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 270929.29,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 268542.89,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 248411.03,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 260937.64,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 376.43,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 379.98,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 10916738.87,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11199508.15,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11609048.06,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12411.34,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 6289358.75,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 12427751.5,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 59664.9,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 6610.04,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1658.22,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2623.17,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2880.15,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 58.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 58.24,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 58.39,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 58.5,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 58.88,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 58.63,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "04c8a9daebb7257d72f72a395f9e3434e04ef3cb",
          "message": "release: update version to 0.8.0 (#52)",
          "timestamp": "2026-06-16T17:12:41+02:00",
          "tree_id": "2dde98e2e0297858915e3df32326233d9a42d87f",
          "url": "https://github.com/utxo-z/utxo-z/commit/04c8a9daebb7257d72f72a395f9e3434e04ef3cb"
        },
        "date": 1781622948861,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 781503.19,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 790755.62,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 769875.87,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 985665.39,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 592.63,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 470.65,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 14114294.93,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 15726486.4,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 15711119.1,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 17392.01,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 10578555.34,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 17150597.15,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 125498.52,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 11413.7,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2582.85,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3547.34,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3549.98,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 3.59,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 3.39,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 3.28,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 3.6,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 3.62,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 3.5,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6106dbddf7cdbce505bb10cd3c9e6159d7ab81fa",
          "message": "docs: document the deferred lookup/deletion contract, add rotation tests (#53)\n\nfind() and erase() only see the currently mapped (latest) version of each\ncontainer, queueing anything else for process_pending_lookups() /\nprocess_pending_deletions(). Neither the not_found from find() nor the 0 from\nerase() is authoritative, but nothing said so: the headers documented find() as\n\"error with not_found on failure\" and erase() as \"0 if not found\", which reads\nas a definitive answer.\n\nThat gap bites only after the first rotation, and with production sizing\ncontainer 0 does not rotate until ~6.6M live UTXOs (BCH mainnet ~block\n245'000). Until then every lookup is served from the single mapped version and\nnever defers, so an integrator treating not_found as definitive works for\nthousands of blocks and then fails abruptly, for a large number of keys at\nonce.\n\n- document the two-phase contract in database.hpp (find, erase and both\n  process_pending_* on each mode) and in a README section, including the\n  ordering rule (lookups before deletions, which remove the entries the\n  pending lookups still need to read) and the measured rotation thresholds\n- add tests/test_find_across_rotations.cpp: inline before rotation, deferred\n  after, one sweep resolving deferrals from all five containers, truly absent\n  keys reported as failed, the erase counterpart, the lookups-before-deletions\n  order, compact mode, and a [.slow][scale] case at production sizing that\n  crosses the ~6.6M rotation. No test in the suite used to cross a rotation,\n  which is why the contract was never pinned.\n- drop the file cache on compact_all(): the cache is keyed by (container,\n  version) and compaction renumbers versions, so cached mappings must not\n  outlive it",
          "timestamp": "2026-07-31T18:21:58+02:00",
          "tree_id": "dd1c9217049a5284924f4ecf198174f2f1d55d86",
          "url": "https://github.com/utxo-z/utxo-z/commit/6106dbddf7cdbce505bb10cd3c9e6159d7ab81fa"
        },
        "date": 1785515036176,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 1047364.13,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 1308555.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 1317709.07,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1300482.47,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 418.59,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 488,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12734947.15,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 7661442.3,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12303934.51,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12615.34,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 11234189.71,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 9989388.14,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 102474.97,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 11444.28,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2106.14,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 4031.27,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3318.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 108.16,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 107.68,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 108.17,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 108.04,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 108.3,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 108.06,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "3cca53b2e74fdc6224aff6e9c8a90e0fbf45c9c9",
          "message": "fix: make release.sh work with BSD userland (macOS)\n\nTwo GNU-only constructs made the release script fail on macOS:\n\n- `sed -i \"s/.../.../\" file`: GNU sed takes the backup suffix attached to the\n  flag, BSD sed takes it as a separate argument, so it consumed the expression\n  as the suffix and tried to run the file path as the script (\"command i\n  expects \\ followed by text\"). Replaced with a plain sed into a temp file plus\n  a mv, which behaves identically on both, and added a grep check so a failed\n  substitution aborts instead of committing an unchanged version.\n\n- `base64 --decode`: long option is GNU-only, BSD uses -D. This one sits in the\n  CI wait loop, so it would have aborted the release right after pushing the\n  branch and opening the PR. Now detects the accepted form once and reuses it.\n\nThe `date` invocation already had a darwin branch; these were the two left.",
          "timestamp": "2026-07-31T18:29:58+02:00",
          "tree_id": "b39b1e21f63da8c33abe7d833a3b73b761809034",
          "url": "https://github.com/utxo-z/utxo-z/commit/3cca53b2e74fdc6224aff6e9c8a90e0fbf45c9c9"
        },
        "date": 1785515522976,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 284868.87,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 348591.65,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 298383.4,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 441184.82,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 337.25,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 385.31,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12304040.64,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 10752256.13,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11871648.44,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13063.78,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 11044271.78,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14869888.48,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 85416.18,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 11249.1,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2506.02,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2952.39,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3020.68,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 58.2,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 58.18,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 58.33,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 58.71,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 58.83,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 58.86,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "1bce7184d919c5b5548bf3833a767a71e11dda98",
          "message": "release: update version to 0.8.1",
          "timestamp": "2026-07-31T18:31:47+02:00",
          "tree_id": "66a8a2b24e698bdd3f4b787fbc490b6d364063de",
          "url": "https://github.com/utxo-z/utxo-z/commit/1bce7184d919c5b5548bf3833a767a71e11dda98"
        },
        "date": 1785515666651,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 13599649.96,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 14670854.6,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 8404886.13,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 6107388.24,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 611.18,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 845.06,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 15676407.31,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 9329601.63,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11945897.51,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 15841.58,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12048909.55,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13281496.21,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 145843.52,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12225.97,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 3320.47,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 7110.5,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3728.7,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 5.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 5.05,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 5.09,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 5.11,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 5.14,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 5.17,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "2bfe0b6b4341c67c7cb3bc90ed8a2c5e39d81be7",
          "message": "fix: merge the release PR synchronously and verify the bump before tagging\n\npost-release.sh used `gh pr merge --squash --auto`, which does not work here and\nhides a worse failure behind it.\n\n`--auto` requires auto-merge to be enabled on the repository, and it is not\n(allow_auto_merge is false), so gh rejects the call and the script stops at its\nfirst action. Enabling the flag would not be the right fix either: --auto\nreturns as soon as the merge is queued, while the next lines already check out\nmaster, pull, and tag it. If the merge has not landed by then, v<version> ends\nup pointing at a master without the version bump, and the release ships with the\nprevious version string.\n\nMerge synchronously instead, and assert that master really carries the bump\nbefore creating any tag, so a merge that did not land aborts the script rather\nthan producing a mislabelled release.",
          "timestamp": "2026-07-31T18:35:49+02:00",
          "tree_id": "fa287ed133ed551f34c52dff13cb2e571f94a6be",
          "url": "https://github.com/utxo-z/utxo-z/commit/2bfe0b6b4341c67c7cb3bc90ed8a2c5e39d81be7"
        },
        "date": 1785515914920,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 14915548.93,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 14578458.35,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 7251075.67,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 7071907.51,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 607.93,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 897.95,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 14976174.27,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 10038472.58,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12451609.26,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 15114.43,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12106374.27,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 10257705.85,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 152283.98,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12082.13,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 3094.37,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 6528.63,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3822.91,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 4.95,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 4.91,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 5.02,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 4.97,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 4.95,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 4.97,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "251c38e8237316dcda0b3e200f5ce8913eb9e9db",
          "message": "fix: merge the release PR synchronously and verify the bump before tagging\n\npost-release.sh used `gh pr merge --squash --auto`, which does not work here and\nhides a worse failure behind it.\n\n`--auto` requires auto-merge to be enabled on the repository, and it is not\n(allow_auto_merge is false), so gh rejects the call and the script stops at its\nfirst action. Enabling the flag would not be the right fix either: --auto\nreturns as soon as the merge is queued, while the next lines already check out\nmaster, pull, and tag it. If the merge has not landed by then, v<version> ends\nup pointing at a master without the version bump, and the release ships with the\nprevious version string.\n\nMerge synchronously instead, and assert that master really carries the bump\nbefore creating any tag, so a merge that did not land aborts the script rather\nthan producing a mislabelled release.",
          "timestamp": "2026-07-31T18:38:23+02:00",
          "tree_id": "cccef83b77f1c42577167f21ce1eab882954346d",
          "url": "https://github.com/utxo-z/utxo-z/commit/251c38e8237316dcda0b3e200f5ce8913eb9e9db"
        },
        "date": 1785516094373,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 799707.8,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 829878,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 845421.42,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1034885.17,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 657.75,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 508.76,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 15135340.44,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 14398928.31,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 16739426.76,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 17326.49,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 15951509.59,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15095684.61,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 124348.91,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14128.59,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2750.47,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3893.24,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3854.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 3.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 3.66,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 3.65,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 3.66,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 3.65,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 3.64,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3a5dfa2798d0f542363251980a5663a4282ccc86",
          "message": "release: 0.8.1 (#54)\n\n* release: update version to 0.8.1\n\n* fix: merge the release PR synchronously and verify the bump before tagging\n\npost-release.sh used `gh pr merge --squash --auto`, which does not work here and\nhides a worse failure behind it.\n\n`--auto` requires auto-merge to be enabled on the repository, and it is not\n(allow_auto_merge is false), so gh rejects the call and the script stops at its\nfirst action. Enabling the flag would not be the right fix either: --auto\nreturns as soon as the merge is queued, while the next lines already check out\nmaster, pull, and tag it. If the merge has not landed by then, v<version> ends\nup pointing at a master without the version bump, and the release ships with the\nprevious version string.\n\nMerge synchronously instead, and assert that master really carries the bump\nbefore creating any tag, so a merge that did not land aborts the script rather\nthan producing a mislabelled release.",
          "timestamp": "2026-07-31T18:47:49+02:00",
          "tree_id": "cccef83b77f1c42577167f21ce1eab882954346d",
          "url": "https://github.com/utxo-z/utxo-z/commit/3a5dfa2798d0f542363251980a5663a4282ccc86"
        },
        "date": 1785516604825,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 342959.62,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 699699.87,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 670474.3,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 897409.4,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 490.87,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 540.46,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12722536.89,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12888832.42,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13475958.57,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13172.75,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 14319609.98,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15229896.22,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 128780.07,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13934.14,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2400.6,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3704.23,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2779.53,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.32,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.88,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.91,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.1,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.33,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.11,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "e79546ff38374faa2586bec13cc7d8da2b23a12b",
          "message": "docs: update release notes for v0.8.1",
          "timestamp": "2026-07-31T18:47:58+02:00",
          "tree_id": "accbf9cb473b0312d3c64ed8ac51e27aeae2ab1d",
          "url": "https://github.com/utxo-z/utxo-z/commit/e79546ff38374faa2586bec13cc7d8da2b23a12b"
        },
        "date": 1785516607498,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 278278.98,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 282857.04,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 264559.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 271076.17,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 402.66,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 467.22,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12726498.19,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 9939906.25,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13118388.84,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12757.97,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 10139998.2,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13112272.65,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 69327.54,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 7416.01,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2397.11,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3093.83,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2902.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.32,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.42,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.48,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.44,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.33,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "e79546ff38374faa2586bec13cc7d8da2b23a12b",
          "message": "docs: update release notes for v0.8.1",
          "timestamp": "2026-07-31T18:47:58+02:00",
          "tree_id": "accbf9cb473b0312d3c64ed8ac51e27aeae2ab1d",
          "url": "https://github.com/utxo-z/utxo-z/commit/e79546ff38374faa2586bec13cc7d8da2b23a12b"
        },
        "date": 1785516620124,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 555477.59,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 276771.51,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 311949.05,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 388293.55,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 345.88,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 394.52,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12234859.77,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12149472.06,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11835521.54,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12929.67,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 9879218.02,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14032975.72,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 73008.75,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 6301.02,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2222.18,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2884.58,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2745.7,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 58.39,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 58.18,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 58.42,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 58.32,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 58.62,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 58.44,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "0156416baf2238411e6cc70cdb9d18186f7fb85c",
          "message": "docs: write down the threading and ownership invariants of the API (#55)\n\nEverything here is already true of the code; none of it was stated anywhere, so\nintegrators had to infer it, and the inferences went wrong in the direction that\ncosts the most.\n\n- db_base and README: a database instance supports one operation at a time,\n  const methods included. Not a \"reads are safe\" guarantee — it covers the whole\n  object, find() included, since with statistics compiled in even a lookup\n  appends to a shared unsynchronised vector.\n\n- process_pending_lookups / process_pending_deletions: the queue is global, not\n  per caller. The sweep takes everything, including keys queued elsewhere, and\n  hands it to whoever called. Exactly one component may own it, and it has to\n  route results back. That is an ownership rule, stated separately from the\n  threading one so it cannot be read as a concurrency exception.\n\n- Both sweeps: a version file that cannot be read is logged and skipped, so its\n  keys come back in the failed list, indistinguishable from genuinely absent.\n  The erase() and find() docs and the README examples now say the same thing, so\n  no entry point still describes that list as proof of absence. Closing the hole\n  properly needs a separate bucket in the return value, which is a breaking\n  change and belongs in its own commit.\n\n- file_cache: any map reference the cache hands out — from get_or_open_file() or\n  get_or_open_compact_file() alike — is valid only until the next cache\n  operation, because eviction unmaps the segment it points into. That bites\n  single-threaded code as soon as anything interleaves two version files, and\n  the default of one cached file makes it easy to trip.\n\n- search_stats: unbounded. One ~16-byte record per successful find() and\n  erase(), freed only by reset_search_stats(); over a full sync that is\n  gigabytes. Also notes that statistics are ON by default and are baked into the\n  installed config.hpp by this project's build, so consumers inherit them rather\n  than opting in.",
          "timestamp": "2026-08-05T15:59:31+02:00",
          "tree_id": "00307a43ef278878d4e8dc752c9e1163a3db5b92",
          "url": "https://github.com/utxo-z/utxo-z/commit/0156416baf2238411e6cc70cdb9d18186f7fb85c"
        },
        "date": 1785938544510,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 786594.47,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 803304.45,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 803128.09,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1018714.16,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 596.63,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 438.24,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 15611024.2,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 16404491.64,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 16121834.5,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 17028.52,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 11656840.25,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 17486515.14,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 107677.16,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14994.81,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2507.99,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3700.27,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3751.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 3.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 3.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 3.7,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 3.66,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 3.68,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 3.68,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "78c846b77c40d504d52a63511a1067ca85a58a62",
          "message": "feat!: shard search statistics into atomic counters, making find() concurrent (#56)\n\nsearch_stats appended a search_record to a shared std::vector on every\nsuccessful find() and erase(). That had two consequences.\n\nIt raced. With statistics compiled in — the default, baked into the installed\nconfig.hpp by this project's own build — a concurrent find() was a data race on\nthe vector, and on reallocation a heap-use-after-free. ThreadSanitizer reports\nboth against the old code on the tests added here. Since find() otherwise only\nreads the active maps and queues into a concurrent set, the statistics were the\nsingle thing preventing concurrent readers.\n\nAnd it grew without bound. One ~16-byte record per operation, retained forever,\nfreed only by an explicit reset. Over a full sync that is gigabytes of records\nnobody reads: the raw vector is never exposed, get_summary() is the only\naccessor, and it computes nothing but sums and ratios of sums.\n\nSo keep the aggregates instead of the records. Seven counters, sharded across 64\nslots with each thread assigned a slot round-robin on first use, all updated\nwith relaxed fetch_add. Recording is O(1) in space, and concurrent recorders\nstay off each other's cache lines — the slots are padded to 128 bytes, the\nwidest line among the platforms we target, since 64 would still share a line on\nApple Silicon. Slots are reused once more threads have recorded than there are\nslots, so a distinct slot per thread is the common case rather than a guarantee;\ncorrectness does not rest on it.\n\nThe counters are read one at a time, so a summary taken while recording is in\nflight can show a numerator an increment ahead of its denominator. The ratios\nare clamped rather than allowed to exceed 1, and the header says plainly that\ncross-field consistency needs a quiescent moment.\n\nsearch_record and its get_utxo_age() go with the vector they existed for. They\nhad no other consumer, and leaving an orphaned record type in the public header\nis an invitation to reintroduce the per-record path.\n\nThe threading contract in the header and README is updated to match. find()\nbecomes the documented exception to the one-operation-at-a-time rule, and only a\npartial one: concurrent find() is permitted strictly while no insert, erase,\nrotation, sweep, compaction or cache operation can run, and providing that\nreader/writer barrier is the caller's responsibility — there is no lock here to\nlean on. Sharding removes an internal writer; it does not make the active map\nsafe against modification, and nothing here does: a rotation unmaps the whole\nactive segment and briefly leaves the container pointer null. The contract also\nnow covers the statistics calls themselves, since get_statistics() is not const\n— it recomputes the fragmentation counters as it goes.\n\nTests cover exact totals, totals under 8 concurrent readers, the miss path\nconcurrently queueing into the deferred set with exact deduplication, and that\nrecording stays bounded.\n\nBREAKING CHANGE: removes the unused public search_record type. Source-level\nonly — it was a standalone struct with no consumer — but it ships in an\ninstalled header, so this needs the 0.9.0 minor release rather than a patch.",
          "timestamp": "2026-08-05T18:58:33+02:00",
          "tree_id": "a47d3996d502afc52499b6b866b88a9327987f6b",
          "url": "https://github.com/utxo-z/utxo-z/commit/78c846b77c40d504d52a63511a1067ca85a58a62"
        },
        "date": 1785949255397,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 277054.36,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 253168.83,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 239984.64,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 257429.25,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 319.8,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 346.97,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11746449.08,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11941604.33,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11148945.45,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11604.11,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 5776229.45,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 10933044.76,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 58468.04,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 6154.65,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1740.88,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2649.23,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2569.12,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 58.06,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 58.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 57.41,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 57.11,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.94,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 57.16,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "074a61f564a720726d38a1d23130d784fb267ed6",
          "message": "fix!: compaction reports a duplicate key instead of silently dropping one (#58)\n\nA stored key is unique across the whole database: two physical entries for one\nkey mean the database is locally inconsistent. Compaction is the one routine\noperation positioned to notice, since it holds two version files open at once\nand already reads every key it moves.\n\nIt did the opposite. The result of the insert into the target was ignored and\nthe source entry was erased regardless:\n\n    target_map->emplace(key, value);\n    it = source_map->erase(it);\n\nso a collision quietly dropped one of the two copies, choosing by nothing more\nthan which file happened to be the target that round. It also left\nentries_count_ untouched, so the database went on reporting a size that included\nan entry no longer on disk.\n\nNow the insert result is checked. A collision proves two version files held the\nkey — either from the start, or because an earlier round of this same run moved\nit there from a third file — and both mean a published state that should not\nexist. So it reports and does not repair: the source copy stays, the target's is\nuntouched, the entry counter is left alone, and compact_all() returns\nerror_code::duplicate_key for the owner to treat as fatal. Choosing a copy would\nerase the evidence of a corrupt database behind a plausible answer.\n\nThree things the failing path owes the caller, in both storage modes:\n\n- an active container. compaction closes one before it starts, so an early\n  return that skipped that would leave the next operation dereferencing null;\n- metadata that describes the files that are actually there. The run may already\n  have drained, removed and renumbered versions before reaching the duplicate,\n  and metadata describes exactly that layout, so the rebuild belongs on both\n  exits rather than only on the successful one;\n- no further mutation. compact_all() stops at the first container that reports\n  the database inconsistent instead of compacting the rest, which would change\n  more state after a condition the owner treats as fatal and destroy more of the\n  evidence of how it got that way.\n\ncompact_all() returns result<> rather than void, since an operation that can\nrefuse has to be able to say so, and the examples check it rather than\ndiscarding it — the point of the return value is that a collision reaches the\nowner.\n\nWhat this does not do: it does not roll back the moves completed before the\ncollision — each is individually consistent, so the database is left partially\ncompacted rather than damaged — and it does not turn ordinary lookups into\nduplicate detection, which would cost them their early exit.",
          "timestamp": "2026-08-05T22:32:41+02:00",
          "tree_id": "121260b8dba1c9d0b74819c12c9bcc2a95975273",
          "url": "https://github.com/utxo-z/utxo-z/commit/074a61f564a720726d38a1d23130d784fb267ed6"
        },
        "date": 1785962106352,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 301616.44,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 357403.95,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 254151.81,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 426070.4,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 383.31,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 435.62,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11879553,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12998169.32,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11101642.6,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11817.15,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 8955374.27,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13449145.9,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 117766.43,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 5820.87,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2021.32,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2738.44,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2573.06,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 58.78,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 58.73,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 58.83,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 58.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 58.94,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 58.77,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b83b60027b368650689ad14d51af5b5aadbe7e05",
          "message": "feat!: split the search counters into probes and historical resolution (#60)\n\nOne search_summary described two different phases at once, and the result was a\nset of numbers that could not mean what their names said.\n\nEvery one of the fourteen add_record() call sites passed found=true, because a\nlookup that resolved nothing recorded nothing at all. So found_operations\ntracked total_operations exactly and hit_rate was the constant 1.0 — a hit rate\nthat cannot express a miss. Two of those call sites, the deferred deletion\npaths, passed an insertion height of zero, so avg_utxo_age averaged real ages\ntogether with absolute block heights. And a lookup that missed the active map\nand was then resolved by a sweep landed in the same denominator twice, once per\nphase.\n\nThe phases are now counted apart, because they answer different questions:\n\n- probe_stats covers find() and nothing else: how many probes there were, how\n  many the active map held, how many had to be deferred, and the age of the ones\n  it answered. find() now records on every call, misses included, which is the\n  part that was missing rather than merely miscomputed.\n\n- resolution_stats covers the historical lookup sweeps: keys resolved, keys left\n  unresolved, the depth they were found at, and how many version files were\n  visited against how many came from the file cache.\n\nDeletions are folded into neither. They resolve through their own sweep and\ndeferred_stats already reports them; the age of a delete is not the age of a\nread. That is also what removes the insertion-height-of-zero contamination,\nsince those call sites were the deletion sweeps.\n\nThe ratio is named active_map_hit_rate rather than anything about avoiding\nresolution, because that is what it measures today: the height argument does not\naffect the result yet, so an entry created above the height being validated\nagainst is still found and counted, even though the caller discards it\nafterwards. When max_creation_height becomes a real bound those move to\ndeferred and the two meanings converge; until then the name and the\ndocumentation say the narrower thing.\n\nRecording is compiled out entirely when statistics are disabled — no counters\nand no call, guarded in the types rather than at each call site, so a call added\nlater cannot forget the guard and put a fetch_add back on the concurrent path.\nThe suite is green in both configurations, which also fixes a pre-existing test\nthat asserted counter values unconditionally.\n\nBREAKING CHANGE: the search statistics API is replaced. database_statistics\n::search becomes ::probes and ::resolution; search_summary and search_stats are\nremoved; get_search_stats() is removed, since the summaries are reachable\nthrough get_statistics() and the accessor returned a non-copyable object whose\nonly use was get_summary(). reset_search_stats() stays and now clears both sets\nof counters. Source-level only, and it needs the 0.9.0 minor release rather than\na patch.",
          "timestamp": "2026-08-05T22:33:29+02:00",
          "tree_id": "f694d3671a618edd14b95f6bf9f3a9055416d436",
          "url": "https://github.com/utxo-z/utxo-z/commit/b83b60027b368650689ad14d51af5b5aadbe7e05"
        },
        "date": 1785962184204,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 14013666.11,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 12854430.07,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 6733694.31,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 7724075.72,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 557.35,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 841.11,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12544861.2,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 8864708,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12149991.46,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12891.2,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 14921032.08,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 11770669.14,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 156586.91,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14976.88,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 3300.08,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 5790.38,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2729.22,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 4.6,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 4.56,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 4.62,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 4.57,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 4.62,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 4.66,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "39d505edac68df41b6f90bb2bfa03c58910a4aed",
          "message": "fix!: catalogue version files by identity, fail closed when it cannot be read (#61)\n\nVersion discovery probed cont_<i>_v00000, v00001, … and stopped at the\nfirst name that was not there. Compaction, having drained a file, removed\nit and then renamed every version above it down one slot to keep the\nnumbering dense.\n\nThat renumbering is a multi-step rewrite of the catalogue with no\natomicity. Interrupted, it leaves the rename applied to some prefix of the\nversions above the hole and not to the rest — a numbering with a gap in\nit. Discovery then stopped at the gap and reported every version past it\nas absent: entries on disk, intact, and unreachable, with no error\nanywhere. The active version was misidentified too, so the next insert\nwent into a file that already held live data.\n\nVersion numbers become identities rather than positions. A drained file is\nremoved and nothing is renumbered behind it, so the cascade and the window\nit opened are both gone. Identities are never reissued while the process\nlives: rotation takes one past the highest ever used, so a number a\ncompaction just retired cannot come back naming a different file while\nsomething is still keyed by the old pair.\n\nThe set of versions lives in a per-container catalogue, built once at open\nand kept current as versions are created and removed. Every traversal, the\nmetadata and compaction itself go through it, so nothing is sized or\niterated by the highest number — which only ever climbs, and would\notherwise cost more forever while the database itself does not. Metadata\nis keyed by version rather than indexed by it. No operation scans the\ndirectory.\n\nReading the catalogue is fail-closed. A directory that cannot be read is\nnot an empty directory, and reporting one as the other is how a database\nwith versions in it gets opened as if it had none and written over.\nEnumeration returns a result and open() propagates it; nothing is\npublished unless the whole directory was walked. The same applies to the\nexistence checks configure() makes on the way in — asked the throwing way\nthey raised out of a result-typed open(), asked the swallowing way they\nanswered \"no\" to a question they could not answer.\n\nOnly canonical names are catalogued: the parsed number must reformat to\nexactly the name on disk, and the entry must be a regular file. An alias\nwould otherwise be catalogued under one name and reopened under another.\n\nTests fabricate each interrupted state directly on the files — the removal\nplus the first j renames, one case for every j the interruption could have\nstopped at — reopen, and require every entry visible exactly once with the\ncorrect active version. Verified non-vacuous: they fail against the\nprevious discovery. The catalogue's own properties are pinned directly,\nsince neither is visible from the public API.\n\nRemoving a drained file is a controlled failure rather than a silent one.\nThe removals report through error_code instead of throwing, the catalogue\ndrops a version only once its data file is actually gone, and a metadata\nfile left behind after its data went is an error too — identities are only\nunique for the life of the process, so a stale record can later be read as\ndescribing a reissued version, and metadata is what a search consults to\nskip a file. Compaction closes the active container before it starts, so\nevery exit now puts one back, including an exit taken by a throw.\n\nThis narrows the exposure to ordered prefix states, where the renames that\nlanded are the first j. It is not a durability guarantee: nothing here\nforces the removal or the metadata to disk, and a directory entry that\nreaches disk out of order is still out of scope. That is separate work.\n\nBREAKING CHANGE: sizing_report::file_count now reports the number of\nversion files present rather than the highest version index plus one. The\ntwo differ once compaction has removed a file. error_code gains\ncatalog_unreadable, returned by open() when the set of version files\ncannot be determined, and removal_failed, returned by compact_all() when a\ndrained version file or its metadata could not be removed.",
          "timestamp": "2026-08-06T12:26:07+02:00",
          "tree_id": "8133990a2473dff072ad88d2b4698f613f718478",
          "url": "https://github.com/utxo-z/utxo-z/commit/39d505edac68df41b6f90bb2bfa03c58910a4aed"
        },
        "date": 1786012108810,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 332452.01,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 458163.72,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 517198.09,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 795599.77,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 457.07,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 515.78,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 13385176.73,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13406933.56,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 7936807.14,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13410.89,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 16058953.83,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14886071.07,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 104492.91,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14701.27,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2707.33,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3573.12,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2829.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.63,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.12,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.45,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e4d90dc5c741ed147770d85c7fe2253d266fcd81",
          "message": "test: restore directory permissions from a guard in the catalogue test (#67)\n\nThe unreadable-directory case took the permissions off the directory and put\nthem back after its assertions. Catch2 aborts a test case by throwing, so on\nthe one run where that restore matters — the failing one — it is skipped, and\nthe directory is left with no permissions at all. Nothing can then read it,\ndelete it, or reuse the path, and every later run of the suite trips over it.\n\nThe two other cases that do this were already guarded; this one was missed.\nSame treatment: arm the restore before the permissions come off.",
          "timestamp": "2026-08-06T13:24:46+02:00",
          "tree_id": "efcdc45a51acf0800053d57ee36f36f003c4ddc1",
          "url": "https://github.com/utxo-z/utxo-z/commit/e4d90dc5c741ed147770d85c7fe2253d266fcd81"
        },
        "date": 1786015617595,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 437135.62,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 343174.87,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 423869.13,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 587588,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 419.12,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 505.3,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12630574.03,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12921763.81,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12141497.8,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12454.76,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12039115.79,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14507969.84,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 138276.62,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12147.66,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2602.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2997.76,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2161.41,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 62.28,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 62.51,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 62.35,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 62.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 62.29,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 58.19,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "4efbd7ce28c6eb44aa20310e6c4453c1f6f80862",
          "message": "fix!: publish metadata atomically and refuse a record that cannot be trusted (#68)\n\nA metadata record summarises a version file so a search can decide the file\ncannot hold the key. Nothing consults the ranges yet, so a wrong summary is\ninert today; it stops being inert the moment historical resolution uses it,\nand then it turns a key that is present into a key that is missing, silently,\non the read path, far from the write that caused it.\n\nBoth sides made that possible. The writer used a plain ofstream: no check on\nany write or on the close, no sync, and written in place, so an interrupted\nwrite left a short file under the final name. The reader checked that the\nfile opened and then checked nothing at all — and created the catalogue entry\nbefore reading into it, so a short file produced a record that find_metadata\nreported as present, carrying whatever the reads had managed to fill in. A\ndamaged record did not degrade to unknown; it became an apparently valid\nsummary.\n\nRecords now carry a marker, a format version and a checksum, and are a fixed\nsize. Five completed reads only prove the file was long enough — not that it\nis ours, that the build agrees about the layout, or that the write finished.\nThe reader validates all of it plus internal coherence, and returns nothing\nat all on any failure, so unknown is the only thing a bad record can become.\nThe marker is checked before the length, so a database written by an earlier\nbuild reads as foreign rather than as damaged.\n\nWriting publishes rather than overwrites: a temp beside the target, every\nwrite and the close checked, then an atomic replace. A reader sees the old\nrecord or the new one. The replace is its own primitive because\nstd::filesystem::rename does not replace an existing destination on Windows;\nremoving the destination first is not an alternative, since that leaves a\nwindow with nothing at the name.\n\nPublication is not durability and the two are reported apart. A failure\nbefore or during the replace leaves the previous record untouched and is\nmetadata_write_failed. A barrier that fails after the replace cannot un-\npublish it, so it is sync_failed: the new record is visible and only its\npersistence is unconfirmed.\n\nOrdinary publication asks for no barrier. Metadata is derived, and every\noutcome a crash can leave — the old record, the new one, nothing, or\nsomething that fails validation — is safe, because anything invalid becomes\nunknown. Barriers are not free: every rotation publishes a record, so\nsyncing there put an fsync on the insert path, measured at 15.1M inserts/s\nwithout and 5.7M with, on a workload with 29 rotations.\n\nThe sync primitives arrive with it for the compaction protocol to use, and\nreport what each platform actually does instead of succeeding silently:\nPOSIX has both barriers, Windows has no directory barrier and says so, and\nunder Emscripten fsync exists and does nothing, which is reported as\nunsupported rather than passed off as success. Where a barrier is asked for\nand the platform has none, that is absorbed rather than propagated — a\ncaller that needs to know asks platform_sync_support().\n\nAbsence stays what it was: an ordinary state meaning unknown. Nothing is\nrebuilt automatically at open, and no traversal consults the ranges yet.\n\nBREAKING CHANGE: the metadata record gains a marker, a format version and a\nchecksum, and its size changes from 88 to 100 bytes. Records written by an\nearlier build are reported as foreign and treated as unknown; they are\nrebuilt by the next operation that writes them. Metadata is derived, so no\ndata is lost. error_code gains sync_unsupported, sync_failed, rename_failed and\nmetadata_write_failed.",
          "timestamp": "2026-08-06T15:27:45+02:00",
          "tree_id": "0559cd272a5dd54ea4add2508046ea1f6529c06a",
          "url": "https://github.com/utxo-z/utxo-z/commit/4efbd7ce28c6eb44aa20310e6c4453c1f6f80862"
        },
        "date": 1786023010323,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 378696.01,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 545432.28,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 471067.42,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 490175.48,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 427.36,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 491.54,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12980749.21,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11715424.73,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12714332.1,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13154.44,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 9241566.66,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14299626.99,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 150307.73,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 15919.89,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2363.2,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3614.06,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2925.59,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 55.31,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.21,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.32,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.4,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.37,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c573f6e246b69b92566cba1acff4904ab4cab45b",
          "message": "feat!: make compaction crash-atomic by building a new file and swapping it in (#70)\n\nCompaction drained one version file into another in place: it inserted an\nentry into the target and then erased it from the source, mutating both\nmapped files with no ordering between them. A process that died in that\nwindow left the entry in both files — a durable, silent duplicate — and,\nbecause writeback of the two files is independent, could equally leave it\nin neither. The first breaks the uniqueness invariant. The second loses a\nlive UTXO. Neither was detectable afterwards, and a partially written\ntarget could also be left structurally inconsistent, since emplace dirties\nthe segment's allocator metadata along with the entry.\n\nA merge now builds a new version file from whole sources and swaps it in.\nThe canonical name is granted last and only to a file whose contents are\nalready durable, so the existence of that name is itself the proof that the\nfile is complete: recovery never verifies a merge, re-reads a source, or\ncompares payloads. Sources are only ever read, so the states a crash can\nleave are the ones this design chose, not whatever the allocator was doing.\n\nThe sequence is build under a name discovery does not accept, barrier the\npages, barrier the file, publish a sidecar naming the target and what it\nsupersedes, publish the target, retire the sources, remove the sidecar,\nwith a directory barrier after each rename and each batch of unlinks. The\nsidecar goes before the target, never after: reversed, a crash between them\nwould leave the target and its sources all canonical with nothing recording\nwhich are redundant.\n\nA sidecar names its target by identity, and an identity is only a number.\nNothing locks the database, so a file at that number is not necessarily the\nfile the record meant, and retiring the sources on that assumption deletes\nthe only copies of their entries. Each merge therefore draws a 128-bit\nidentifier from the system's cryptographic generator — named directly, since\nstd::random_device is permitted to be deterministic, and never derived from\nthe clock or the process id, which two containers can share — writes it into\nthe target before the barriers and into the record that names it, and\nverifies the two match before retiring anything. A target with no marker, a\ndifferent marker, more than one, or one that cannot be read all stop with\nevery file intact. Failing to obtain entropy stops the merge before anything\nis written.\n\nPublishing the target cannot be undone by wishing. A failure there leaves a\ndurable record, so calling the merge off means withdrawing that record and\nhaving the directory confirm it; anything less latches the instance rather\nthan reporting a clean failure. And the target's canonical name is not\ndurable until the directory says so — retiring sources before then is how a\npower cut leaves the sources gone and the target unreachable, so on\nplatforms with that barrier its failure stops the merge with nothing\nretired. Where the platform has no directory barrier the guarantee is\nweaker, and says so.\n\nCompaction closes the active container before it starts, and putting one\nback is part of the typed result rather than something a scope guard does:\na guard runs in a destructor, cannot report, and must swallow what it\ncatches, so a reopen that failed left the call returning success with\nnothing mapped and the next operation dereferencing a null container. A\nfailed reopen now reports and latches.\n\nRecovery is a mandatory phase of open(), before any container is opened, so\nan intermediate state is never observable. It acts only on evidence the\nstore wrote, and removes only names that parse inside its reserved\nnamespace. A sidecar with no target abandons the merge; a sidecar with a\ntarget finishes retiring the sources; a sidecar that cannot be fully\nvalidated, or more than one for a container, is fatal. Nothing is guessed.\n\nPublication cannot overwrite. The target takes an identity that was never\nissued, and it is placed with a move that fails if the name is taken rather\nthan a rename that would replace it — single-instance exclusion is why the\ncheck is meaningful, and this is what happens when that precondition does\nnot hold.\n\nA merge that publishes and then cannot retire what it superseded leaves\nseveral canonical files holding the same keys. Exclusion is what kept that\nunobservable and exclusion ends when the call returns, so the instance\nlatches: every operation that reads or changes stored state returns\nrecovery_required until it is closed and reopened. size(), the queue sizes\nand the statistics keep answering, and are documented as diagnostic rather\nthan access to the authoritative state.\n\nGroups are whole files and never fewer than two. A source is never\npartially consumed, because one that was would have to survive holding\nentries the new file also holds.\n\nBoth storage modes.\n\nBREAKING CHANGE: six public operations now return result<>, so that an\ninstance with cleanup pending can refuse them rather than answer from a state\nholding duplicates: db_base::erase(), db_base::process_pending_deletions(),\ndb_base::for_each_key(), full_db::process_pending_lookups() and\ncompact_db::process_pending_lookups(), and full_db::for_each_entry() and\ncompact_db::for_each_entry(). The iteration methods are [[nodiscard]], since a\nresult a caller may silently drop is not a refusal. erase() previously returned\na count whose zero was already documented as not authoritative, which would\nhave made a refusal indistinguishable from a deferral. error_code gains\nentropy_unavailable, file_open_failed, identity_collision,\ninsufficient_space, recovery_required and recovery_failed. The merge record\nformat moves to version 2 and version 1 is refused rather than read, so a\ndatabase left mid-merge by an earlier build stops at open with\nrecovery_failed. Such a record must not simply be deleted: without an\nidentifier the store cannot tell whether the target was published or how\nmany sources are already retired, and in every state but the first the\nrecord is the only thing saying what supersedes what. Reading it instead\nwould retire sources on an identity match alone. All of it lands in the unreleased 0.9.0.",
          "timestamp": "2026-08-07T15:07:26+02:00",
          "tree_id": "9959b209a13ba838c83e9f7834a8d3b4ff45f0df",
          "url": "https://github.com/utxo-z/utxo-z/commit/c573f6e246b69b92566cba1acff4904ab4cab45b"
        },
        "date": 1786108195601,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 301603.98,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 301900.81,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 676165.81,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 852315.1,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 492.92,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 528.83,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 13179908.36,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13149923.67,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12904638.99,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13206.53,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 15740042.8,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15971522.69,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 168444.03,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14726.28,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2928.46,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3595.06,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2699.53,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.17,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.16,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.13,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.37,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.21,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.24,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d68a607f2b926a6e8513f369f5adc10f2330374f",
          "message": "fix: refuse an unusable version file at once instead of waiting five minutes (#76)\n\nBoost, handed a mapped file whose header does not read as initialised,\nassumes another process is midway through creating it and spins waiting for\nthat to finish. It waits for\nBOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC, which is\nfive minutes by default, and only then reports corruption.\n\nNothing here ever waits on another process creating a segment — one\ninstance owns a database — so that wait is pure delay. A truncated or\nforeign file at a canonical version name stalled the operation that met it\nfor five minutes, and for five minutes per file if more than one was\ndamaged. An open that meets three of them took a quarter of an hour to\nreport a problem it knew about immediately.\n\nEvery version file is now opened through one function that first refuses\nwhat cannot be one of ours — including the two openings inside the file\ncache, which is the path historical resolution actually takes and had its\nown wait. There is no other open_only left in src/, so the check is a\nproperty of the store rather than of the call sites that remembered it. The floor is derived from the sizing constants\nrather than chosen, so it follows them: no configuration in this build\ncreates a version file below the smallest configured size, and a file under\nthat is rejected for the cost of one stat. The build also caps the Boost\ntimeout, which bounds whatever the size check cannot recognise.\n\nThe two are complementary and the test tells them apart by the only\nobservable that differs, since the error is the same either way: with the\nsize check a traversal over a truncated file takes milliseconds, with only\nthe cap about twenty seconds, with neither five minutes. The bound is five\nseconds — three orders of magnitude above the real figure, and failing if\neither protection is removed.\n\nThis was found while investigating the CI critical path in #74. One test\ncase accounted for 91% of the suite's runtime, all of it spent in that\nwait rather than in anything the test was checking. The suite goes from\n334 to 35 seconds with no case removed, shortened or weakened.\n\nCloses #74",
          "timestamp": "2026-08-07T16:42:24+02:00",
          "tree_id": "d3e06ec34e6bf56eaffb4c9914b42d0b4c60c7d5",
          "url": "https://github.com/utxo-z/utxo-z/commit/d68a607f2b926a6e8513f369f5adc10f2330374f"
        },
        "date": 1786113918246,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 15382089.8,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 15355703.37,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 10337501.64,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 9197040.32,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 576.17,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 884.02,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 14407584.43,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 10738592.45,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13528176,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11982.46,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 15562991.63,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14480669.03,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 169319.48,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 16399.69,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 3600.12,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 6914.4,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2769.57,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 5.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 5.04,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 5.13,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 5.01,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 5.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 5.12,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c2954f4b85a169e911ad562c6cf486526437a4d4",
          "message": "feat!: claim a database exclusively while an instance holds it (#77)\n\nThe store is written for a single owner: one catalogue, one set of deferred\nqueues, one active container per size class. Two instances over one\ndirectory would each hold their own and neither would know about the\nother's rotations, merges or recoveries. Compaction was made safe against a\nsecond process destroying data — a merge cannot retire a source unless the\ntarget carries its identifier — but safe against that one hazard is not\nmulti-process safe.\n\nAn instance now takes an exclusive claim on the directory and holds it for\nits life. What that excludes is cooperating processes using this library; a\nprocess that ignores the claim, or someone deleting files by hand, is\noutside it, as it is for any advisory lock. It adds no thread safety: the\ncontract of one mutating operation at a time is unchanged.\n\nThe claim is a lock on an open descriptor, not the existence of a file. The\nkernel releases it when the last copy of that descriptor closes, which\nincludes a process dying however it dies; a lock file would outlive a crash\nand shut the database out until someone deleted it by hand. That is also\nwhy the descriptor is close-on-exec and the handle non-inheritable — a copy\ninherited across an exec would hold the claim in a process that never asked\nfor it.\n\nTwo failures are kept apart because they send an operator to different\nplaces: database_in_use means the lock was attempted and someone else has\nit, and nothing else maps to it; database_lock_unavailable means the\nattempt could not be made at all.\n\nIt is taken before recovery, which unlinks files and which two processes\nmust never run at once over one directory, and before the mode check, so a\ndatabase that is both in use and of the wrong mode reports the one that\nstops you either way. remove_existing empties the directory's children\nrather than the directory: removing it would unlink the inode the claim is\nheld on, leaving the holder locked to a name nobody can reach while a\nsecond process created a new lock file, claimed a different inode, and both\nbelieved they were alone.\n\nThe holder's pid and the time it took the claim are written into the file\nafterwards, for whoever reads a log. They are diagnostic: never consulted\nto decide ownership, never read by recovery, and failing to write them does\nnot weaken a claim already held.\n\nThat text is readable while the claim is held on POSIX only. flock leaves\nthe contents readable; LockFileEx locks them, so on Windows nothing can\nread it until the holder lets go — which is when it would have been useful.\nThe limitation is documented rather than worked around: a readable holder\nthere means locking one region and keeping the text in another, which is a\ncontract to design rather than a detail. The claim itself, and the\nexclusion it provides, are identical on both.\n\nUnder Emscripten the filesystem is virtual and there is no second process,\nso the claim succeeds and is documented as vacuous. Refusing would break\nthe one build where the hazard cannot arise.\n\nCloses #71\n\nBREAKING CHANGE: opening a database that another instance holds now fails\nwith error_code::database_in_use rather than succeeding and corrupting it.\nerror_code also gains database_lock_unavailable. A database directory\nacquires a permanent .utxoz.lock file, which must not be removed, renamed\nor replaced while an instance exists.",
          "timestamp": "2026-08-07T18:20:04+02:00",
          "tree_id": "4e3da1a0c912903aada62ee6a0f45e36f51eb903",
          "url": "https://github.com/utxo-z/utxo-z/commit/c2954f4b85a169e911ad562c6cf486526437a4d4"
        },
        "date": 1786119749962,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 409345.68,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 363657.08,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 545628.88,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 621620.61,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 446.92,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 499.55,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 13320117.34,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13233436.41,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13003112.78,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12988.89,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 9497482.85,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14126800.15,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 139548.8,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13079.27,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2641.91,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3446.87,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2758.42,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.4,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.32,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.8,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.89,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.16,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.13,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "94bf695b6d8251532f3f47eaf04c69fa95b6e2c2",
          "message": "test: run the crash matrix against compact mode as well (#78)\n\nThe crash-atomic merge protocol is written twice, once per storage mode,\nand the eight-point matrix only ever ran the full one. Two copies of a\nsequence whose correctness is entirely in its ordering is the shape that\ndrifts, and a matrix watching one of them would not notice the other\ndrifting.\n\nCompact mode had the latch, the uncertain-record and the name-barrier\ncases, but nothing that cut the protocol at each barrier and checked what\nreopening made of it. It does now: the same eight points, the same row per\npoint, the same requirements — every key exactly once, the merge finished\nor abandoned according to whether the target was published, no reserved\nnames left, the database serving again, and a second recovery that changes\nnothing.\n\nThis also puts a net under #72. Once the two implementations are one, both\nmatrices exercise the same code, and a barrier moved in it fails on both\nsides rather than silently in the mode nobody was watching.",
          "timestamp": "2026-08-07T19:25:47+02:00",
          "tree_id": "4e0ed392063b7a845d90fc172486f3f8e2ea539f",
          "url": "https://github.com/utxo-z/utxo-z/commit/94bf695b6d8251532f3f47eaf04c69fa95b6e2c2"
        },
        "date": 1786123726607,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 15739186.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 15292932.32,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 8361030.23,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 5354491.89,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 638.55,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 1030.99,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 16853472.87,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11926273.46,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 15626481.26,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13984.84,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 17149736.35,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 10594795.09,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 184487.59,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 15455.94,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 3483.06,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 6562.58,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3174.99,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 5.75,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 5.78,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 5.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 5.76,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 5.75,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 5.71,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "042c67f44fcec69700bd0e901f68b392537a7ead",
          "message": "feat: add sync(), which puts what has been written on stable storage (#80)\n\nEvery write until now has been buffered. Inserts and erases land in mapped\nfiles, the kernel writes those pages back when it chooses, and a power cut\nloses whatever it had not got to — including writes that every call\nreported as successful. That is not a defect, it is what buffered I/O is,\nand there was no way to end it.\n\nsync() ends it, and covers three things because all three carry writes. The\nactive container of each size class, obviously. Every historical generation\nthis instance has written to, which is the part worth being careful about.\nAnd the directory, because a file that exists only in an unflushed directory\nentry is a file that may not be there.\n\nA version owes a barrier from any of three directions, and all three are\nrecorded: written to by a deferred deletion sweep; written to directly in a\ncached historical file, outside the sweep; or active, receiving inserts, and\nthen rotating. The rotation is the one worth spelling out: a\nversion that stops being active stops being covered by the active-container\nbarriers, and closing it only calls the asynchronous flush, which schedules\nwriteback and promises nothing. It is recorded at the rotation rather than\nat each insert because the active version is covered for as long as it is\nactive, so the only moment coverage could be lost is the moment it stops\nbeing one; marking per insert would put a lookup on the hot path to record\nsomething already known.\n\nThe historical generations are tracked by identity, not by what the file\ncache is holding. The cache is an LRU of one mapping by default, so a sweep\nthat deletes from three generations has evicted the first two before it\nends — and unmapping is not a barrier: those pages are still dirty and\nnothing has asked the kernel to write them. A sync driven by the cache would\nflush the one survivor and report the database durable. So a version is\nrecorded as dirty when it is written to, the record outlives the mapping,\nand it is discharged only by a sync in which every barrier owed returned.\nA partial failure discharges nothing. Compaction retiring a source discharges\nthat source, since its entries are in a target made durable before it was\npublished.\n\nThe config file is published rather than written in place — a temp, a\nbarrier, an atomic replace, a directory barrier — and made durable there\nrather than by sync(). It says whether a database is full or compact, so a\nreader finding half of it reads the wrong answer about the whole store.\nWriting it now happens only when the database is created — the content\nnever changes, so rewriting it on every open put a barrier, and a way to\nfail, on a path with nothing to say. It says whether a database is full or\ncompact, so failing to make it durable is reported rather than warned about:\nsync() leaves it out of its promise on the grounds that this happened, and\nif it did not, open is the only place anyone finds out. A directory barrier\nis not a substitute for flushing its contents.\n\nCallers that need to know what a platform can promise ask\nplatform_durability(). Success under contents_only means the entries reached\nthe disk and the directory entries naming them did not, which is not the\nsame guarantee — and a caller recording a checkpoint on the strength of a\nsync should be able to tell them apart rather than infer it.\n\nEach mapping is flushed before the file behind it. The page barrier covers\nthe dirty pages, the file barrier covers the inode, and neither covers the\nother.\n\nHow often is not decided here. The store has no idea what a caller is\nwilling to lose, and a policy baked into it would be wrong for everyone: a\nnode syncing per block pays for durability it may not need below a\ncheckpoint, one syncing per hour may not be able to say what it has. The\ncaller knows and calls this when it wants the guarantee.\n\nDerived metadata is deliberately outside the promise. Losing a record costs\na rescan and nothing else, since an absent or damaged one degrades to\nunknown, so a barrier per record would buy nothing a caller could use.\n\nPer platform, stated rather than implied: POSIX has both barriers and this\nis a full guarantee; Windows flushes file contents and has no directory\nbarrier, so the ordering between a rotation and the data it publishes is\nweaker; under Emscripten the filesystem is virtual and there is no stable\nstorage to reach, so it returns sync_unsupported rather than a success it\ncannot honour.\n\nA failure is not retryable in any useful sense: the platform refused to\nflush, and asking again asks the same disk the same question.\n\nThe case that matters most is coverage of the evicted generations, and it is\ncounted rather than provoked: a failing barrier comes back whether or not\nthose files are reached, because the active containers are flushed first. It\nrequires more file barriers than a cache-driven sync could perform at all,\nwhich is the one thing separating the two designs. Driven by the cache\ninstead it reports six barriers against five active containers and one\nresident mapping, and fails.",
          "timestamp": "2026-08-07T20:01:59+02:00",
          "tree_id": "b2df01d904c689455931d3a2320d1ec80ea44902",
          "url": "https://github.com/utxo-z/utxo-z/commit/042c67f44fcec69700bd0e901f68b392537a7ead"
        },
        "date": 1786125872887,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 191847.2,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 300857.27,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 257565,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 261242.43,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 386.7,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 371.16,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12034721.25,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11997338.55,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12049157.47,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12454.45,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 8989365.23,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13237629.85,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 96723.58,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 9385.81,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2210.74,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2861.64,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2864.32,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.11,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.39,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.96,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 55.87,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 55.83,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.04,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "934954845abad7453fc6e49f49277651580b6d6b",
          "message": "refactor: write the merge protocol once instead of once per storage mode (#81)\n\nThe crash-atomic merge was implemented twice, once per storage mode, and\nthe two were identical step for step — twenty-eight of them in the same\norder, down to which failpoint sits between which barriers. Only six things\ndiffered: the map type, the container's identity, the file size, the bucket\ncount, which catalogue holds the versions, and where the block height lives\nin a stored value.\n\nTwo copies of a sequence whose correctness is entirely in its ordering is\nthe shape that drifts, and this one already had. The compact twin was left\non the previous protocol when the full one was rewritten, and every\ncorrection since — the merge identifier, the withdrawal of a record whose\npublication failed, the barrier before any source is retired — had to be\napplied twice by hand, with nothing checking that both landed.\n\nThe six differences are named in a policy and the protocol is written once.\nNothing else changes: not the order, not a failpoint, not an error, not a\nstate. The only edits inside the body are the mechanical ones the policy\nexists to make, plus log lines that now name a version through the policy\nrather than formatting a container index that no longer exists there.\n\nProven rather than asserted. Extracting the sequence of barriers, crash\npoints, catalogue mutations and latches from all three versions — the two\nthat were replaced and the one that replaces them — gives twenty-eight\nsteps, and the unified sequence is identical to both. Both crash matrices\npass, which is what #78 was written to make possible: they now exercise the\nsame code, so a barrier moved in it fails on both sides rather than\nsilently in the mode nobody was watching.\n\nCloses #72",
          "timestamp": "2026-08-08T00:58:02+02:00",
          "tree_id": "68e44bd6e020794706febb927dcc9da4024a701f",
          "url": "https://github.com/utxo-z/utxo-z/commit/934954845abad7453fc6e49f49277651580b6d6b"
        },
        "date": 1786143651068,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 277233.21,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 321641.66,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 314415.38,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 272168.09,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 362.91,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 432.48,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12493991.84,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 9932715.75,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12165787.52,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13215.32,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 9327859.97,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 10372712.62,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 109585.52,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 7966.08,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2110.34,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2983.43,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2884.43,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 52.68,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 52.58,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 52.7,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 52.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 52.1,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 52.78,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "1894bb68501db9ea03c31b5703bc7130beb6d2ca",
          "message": "refactor!: name the storage mode reference, not compact (#82)\n\nOne word meant two unrelated things. `compact` was the storage mode that\nkeeps a fixed-size pointer to where an output lives, and `compact` was\nalso the operation that merges version files and reclaims their space.\n\"Compact the compact database\" is not a joke about naming; it is what the\ncode said, and `compact_compact_container()` was a real function.\n\nThe mode is now `reference`, because a reference is what it stores: a\nfile number and an offset naming the output, not the output itself. The\noperation keeps its name — `compact_all()`, `compact_container()`,\ncompaction — and does exactly what it did before.\n\nNothing on disk changed, which is the part worth stating plainly. The\nfile names are the ones they always were (`compact_v00000.dat`,\n`meta_compact_v00000.dat`, `compact_v00000.merge`), and the mode byte in\n`utxoz_config.dat` is still 1. A database written before this commit\nopens after it, and one written after opens before. Renaming either would\nbreak that, silently in the second case, since that byte is what tells a\nreopen which kind of database it is looking at — so a test now asserts\nboth, and it fails if the enumerator is renumbered or the prefix moves.\n\nBREAKING CHANGE: `compact_db` is now `reference_db`,\n`compact_find_result` is `reference_find_result`, `storage_mode::compact`\nis `storage_mode::reference`, and `compact_file_size` and\n`compact_test_file_size` are `reference_file_size` and\n`reference_test_file_size`. Renaming at the call site is the whole\nmigration: the enumerator's value, the on-disk format and every semantic\nare unchanged. Targeted at 0.9.0.\n\nCloses #79",
          "timestamp": "2026-08-08T14:07:01+02:00",
          "tree_id": "640e3b67d46ddd86841249e481b646f829490a1c",
          "url": "https://github.com/utxo-z/utxo-z/commit/1894bb68501db9ea03c31b5703bc7130beb6d2ca"
        },
        "date": 1786190958964,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 273393.36,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 316188.87,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 265861.65,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 541350.04,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 438.41,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 425.25,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12273530.4,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12438708.7,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11950461.25,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12349.15,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12275229.99,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15641551.14,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 136891.91,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12200.44,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2526.2,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3088.69,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2865.97,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.7,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.57,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 57.02,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.75,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.44,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.65,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "77a72e30ec006a5ef9307a116b93e9dda9eeece4",
          "message": "fix: generate config.hpp and version.hpp into the build tree (#85)\n\n`configure_file` wrote both headers into `include/utxoz/`, which every build\ndirectory shares. That made them shared mutable state: configuring a second\nbuild overwrote the headers the first one was using, and the first then went on\ncompiling against options it had never been given. No error, no warning, no\nreconfiguration — its own `CMakeCache.txt` still said `statistics=True` while\nthe binary it produced had statistics compiled out.\n\nIt is not a hypothetical. Two directories, `-o statistics=True` then\n`-o statistics=False`, rebuild the first: `Reference: statistics` fails in a\nbuild configured with statistics enabled. And `cmake --install` from that same\nbuild directory shipped a `config.hpp` saying statistics was disabled — a wrong\npackage, produced by a successful install.\n\n`version.hpp` was worse, because it is tracked. Every build rewrote a file under\nversion control with whatever version that build happened to use, so it showed\nup as a modified file to be reverted by hand, or committed by accident.\n\nBoth headers now go to `${CMAKE_CURRENT_BINARY_DIR}/include/utxoz/`, and three\nthings follow from that:\n\n- The generated include directory precedes the source one. Working trees that\n  predate this change still hold the old files, ignored by git and regenerated\n  by nobody; searched first, the source tree would serve those instead.\n- `install()` takes them from the build tree, and the source-tree install\n  excludes both names so a leftover copy cannot ship. Without this the package\n  loses the headers entirely, which is not an error here — it is a broken\n  `#include` in whoever consumes it.\n- `version.hpp` is untracked, and both are ignored.\n\n`ci/check_generated_headers.sh` is the regression: two configurations that must\nnot disturb each other, a source tree that must stay clean, a stale header\nplanted mid-build that must not shadow the generated one, and an install that\nmust carry both with the right contents. Every check fails on the previous\ncode. A new CI job runs it.\n\n`conan create` gained a `test_package`. It only built the package before, which\ncannot tell whether the package is usable — a consumer that includes both\ngenerated headers and opens a database can. Removing the generated-header\ninstall makes it fail with `utxoz/config.hpp: No such file or directory`, which\nis the failure that used to reach Knuth instead of CI.\n\nCloses #84",
          "timestamp": "2026-08-08T16:03:22+02:00",
          "tree_id": "eb93e205a0fc411772f06f60dc662cbb9a15210e",
          "url": "https://github.com/utxo-z/utxo-z/commit/77a72e30ec006a5ef9307a116b93e9dda9eeece4"
        },
        "date": 1786197952254,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 338254.02,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 449870.54,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 358585.74,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 653516.38,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 489.8,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 481.94,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12093011.92,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12948379.58,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12805303,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12254.02,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 15246159.38,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14813595.87,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 142071.83,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13466.34,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2904.12,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3598.75,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2928.19,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 53.99,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.1,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.18,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.97,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.07,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5b6876618066a529b789baa57beda7ede7ae63d2",
          "message": "ci: pin and verify build toolchains (#86)",
          "timestamp": "2026-08-08T18:28:45+02:00",
          "tree_id": "c2e7d2e88abcc34507daeb39c96499166f8ca382",
          "url": "https://github.com/utxo-z/utxo-z/commit/5b6876618066a529b789baa57beda7ede7ae63d2"
        },
        "date": 1786206745790,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 289431.41,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 538334.35,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 383655.4,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 547820.71,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 418.48,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 483.84,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12661700.27,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13402442.04,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12516361.19,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12952.13,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 14581173.07,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14614262.02,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 160044.46,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12461.21,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2693.83,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2816.06,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2958.84,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 53.84,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 54.18,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.29,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.68,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.99,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "914bbc287cdc30d238d783151371412bb999ca96",
          "message": "fix: refuse a build version that is not a version (#90)\n\nPart of the build version comes from a branch name, and on a pull request\nthe branch name is written by whoever opened it. `determine-version` takes\nthe last path component of a `release/*` or `hotfix/*` ref verbatim, so a\nbranch called `release/0.9.0; whoami` yields exactly that string — verified\nagainst the previous code, which produces `0.9.0; whoami`.\n\nThat string was then expanded into six `run:` blocks. An expression in\n`run:` is substituted into the script before the shell sees it, so quoting\nat the point of use would not have helped: it was not an argument, it was\nshell source, in `conan install`, `conan create` and `conan upload`.\n\nThe first injection point was inside the action itself:\n\n    GITHUB_REF=\"${{ inputs.github-ref }}\"\n\nwhich is command substitution in the assignment, before any output exists\nto validate. So the inputs now arrive through `env:`, and so does every\nconsumer's copy of the version, quoted at each use.\n\nThe check runs once, in the derivation, before the value can become a step\noutput. No consumer can forget it, and the per-job guard that had been\nadded to one job is gone — one check nobody can skip beats six that\nsomebody eventually will. A ref that does not yield a version fails\n`setup`, so no job runs at all.\n\nThe derivation moved to `ci/determine_version.sh` so that the thing under\ntest is the thing that runs, and `ci/test_determine_version.sh` is the\ntest: nine refs that must be accepted, and sixteen that must be refused —\na space, `;`, `$()`, backquotes, an embedded newline, a leading `v`, an\nempty component, two components, path traversal, `&&`, `|`, `--version`,\nthe hostile names again as bare branch names, and a newest tag that is\nnot itself a version. Removing the pattern turns all sixteen red.\n\nReading the newest tag is one checkable command rather than\n`git tag -l | head -n 1`. A pipeline reports the status of its last stage, so\nhead succeeding hid git failing and left the tag empty — indistinguishable\nfrom a repository that simply has no tags, and it became 0.0.0-commit.<run>\nas though that were an answer. `git for-each-ref --count=1` separates them:\na query that finds nothing still falls back to 0.0.0, a query that fails\nwrites to stderr, exits non-zero and puts nothing on stdout. Both are tested,\nand the failing-git case is accepted as 0.0.0-commit.42 by the previous\npipeline.\n\nBehaviour is otherwise unchanged, deliberately. A tag still publishes\n`<last-tag>-commit.<run>` rather than its own version — surprising, and\nhow every release so far was cut, so changing it belongs to whoever owns\nreleases rather than to a fix about quoting.\n\nCloses #87",
          "timestamp": "2026-08-09T01:52:02+02:00",
          "tree_id": "e3a564faa3212181b82768498973aa345757f8be",
          "url": "https://github.com/utxo-z/utxo-z/commit/914bbc287cdc30d238d783151371412bb999ca96"
        },
        "date": 1786233330259,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 276612.93,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 274306.09,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 326786.79,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 281000.83,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 456.82,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 454.34,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 13298614.07,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12921604.5,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13196233.69,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13482.06,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 7867640.67,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14013852.56,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 119604.68,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 10072.25,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1799.89,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3049.18,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2763.99,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 53.07,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 52.93,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.48,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.51,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.45,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.24,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "29221b2ea3f83500179ed6b8e96d20558938350f",
          "message": "test: keep the reference statistics test honest with counters compiled out (#91)\n\n`Reference: statistics` asserted\n\n    CHECK(stats.total_inserts >= 50);\n\nunconditionally, against a counter that does not exist when\nUTXOZ_STATISTICS_ENABLED is undefined. With `-o statistics=False` it read 0\nand the test failed — the only failure in that configuration, and one that\npredates this branch: it fails the same way on master.\n\nThe full-mode test in test_database.cpp already had the right shape and the\ncomment explaining it. This one was left behind when that convention was\napplied, so it gets the same split: the mode and the entry count are kept by\nthe database itself and are asserted either way; only the counter goes behind\nthe guard. Nothing is deleted — with statistics on, `total_inserts` is still\nchecked.\n\nNobody noticed because nobody built it. No job set the `statistics` option, so\nevery run used the default. A configuration the library supports, and offers as\na Conan option, had no coverage at all.\n\nSo there is a job for it now, and it runs the tests rather than only building.\nThat distinction is the whole point: `statistics=False` has always compiled\nclean, and what was broken only appeared when the suite ran. A job that stopped\nat compilation would have gone green over this exact failure. It also asserts\nthat the configuration under test is the one it meant to build, by reading the\ngenerated config.hpp — a job named \"statistics disabled\" that quietly built with\nthem enabled would prove nothing.\n\nVerified in both directions: with `-o statistics=False`, master fails\n`Reference: statistics` and this commit passes 102/102 — five test cases are\nthemselves compiled out. With statistics enabled, 107/107.\n\nCloses #83",
          "timestamp": "2026-08-09T09:51:18+02:00",
          "tree_id": "b5f3de190949825fb2ce81bfbdfdd5ce59ae02db",
          "url": "https://github.com/utxo-z/utxo-z/commit/29221b2ea3f83500179ed6b8e96d20558938350f"
        },
        "date": 1786262074084,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 206119.27,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 268558.71,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 128542.97,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 330397.61,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 376.41,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 407.19,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11907377.51,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12938100.86,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11801718.2,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12075.71,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 7109256.65,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15071337.17,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 81430.22,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 7036.95,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1885.25,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2829.17,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2778.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.34,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.58,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.73,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 55.89,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 55.76,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 55.56,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7c62f04a1eccc2f9cd320b78dca8f30c4f56ed39",
          "message": "fix: run every step in bash, and check the version the build actually used (#97)\n\nSince #90, Windows built the library as 0.0.0-dev.\n\nThat change moved the build version out of `${{ }}` and into `$UTXOZ_BUILD_VERSION`,\nwhich is right — an expression in `run:` is substituted before the shell sees\nit, so it is shell source rather than an argument. What it missed is that the\ndefault shell on Windows runners is PowerShell, where `$UTXOZ_BUILD_VERSION` is\nan undefined PowerShell variable and not the environment. Conan therefore got\n`--version=\"\"`, resolved the recipe as `utxoz/None`, and the conanfile's own\nfallback turned that into 0.0.0-dev.\n\nCompare the same commit across two runs:\n\n    #86, before  Windows  conanfile.py (utxoz/0.8.1-commit.266)\n    #96, after   Windows  utxoz/None, \"utxoz version: 0.0.0-dev\"\n    #96, after   Linux    conanfile.py (utxoz/0.8.1-commit.274)\n\nEvery Windows job stayed green, because a version nothing compares against is a\nversion nothing notices: the conanfile falls back, the tests do not look at it,\nand the job that would really suffer — publish — runs for tags and release\nbranches only, so no pull request has ever executed it.\n\n`defaults.run.shell: bash` makes every step read the environment the same way\neverywhere. Audited before setting it rather than after: only `build` and\n`publish` run on Windows at all, and every step in them that is genuinely\nPowerShell — locating MSVC, entering the developer shell — already declares\n`shell: pwsh` and keeps it. What is left are plain commands that behave the\nsame under either shell.\n\nThe default alone would leave the same class of defect free to return, so the\ntoolchain verifier now compares the version the job was handed against the\nversion the build used, in two places: `UTXOZ_VERSION` in CMakeCache.txt, which\nis what CMake was configured with, and the generated version.hpp, which is what\ngets compiled into the library and shipped. A build configured correctly whose\nheader disagrees is still a broken package.\n\nIt runs wherever there is a configured build to compare against — build,\nbuild-wasm, statistics-disabled and benchmark — and it fails on exactly the\nmismatch above:\n\n    expected build version 0.8.1-commit.274, CMake configured 0.0.0-dev\n\nPublishing had the worse end of this. A release cut today would have had the\nWindows job run `conan create --version=\"\"` and `conan upload \"utxoz/\"`.",
          "timestamp": "2026-08-09T10:33:33+02:00",
          "tree_id": "d2345161f3c2c4df6ce86c30b54d7b83ef6f46f9",
          "url": "https://github.com/utxo-z/utxo-z/commit/7c62f04a1eccc2f9cd320b78dca8f30c4f56ed39"
        },
        "date": 1786264608280,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 293319.65,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 694375.36,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 272129.67,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 483867.85,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 462.07,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 455.51,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 13407899.18,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13571147.29,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12730690.96,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13193.05,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12038424.67,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 9684245.22,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 138626.02,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 9967.92,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2711.24,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3322.55,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2773.59,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.99,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.74,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.73,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.6,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e32eb0a2323947c951f653af6a2c1f4cbadf46ac",
          "message": "fix: pin line endings, so the recipe hashes the same on every platform (#95)\n\nConan derives the recipe revision from the bytes of the exported sources,\nwhich makes line endings part of the package's identity. Nothing pinned\nthem: there is no .gitattributes, so the working tree got whatever each\nplatform's git defaults produced.\n\nThe result is that one release publishes several recipes. `release/0.8.1`\npublished `9fbdb476953e2efe5dcbe0038ce382a1` from Linux and macOS and\n`cd1d5b960d68182d36644a5cb5ef1005` from Windows, on the same commit; the\nversions still on the remote each carry exactly two revisions, one second\napart, which is the same split. A consumer resolves whichever the server\nconsiders latest, so which recipe Knuth gets depends on which publish job\nfinished last.\n\nReproduced before fixing: the same tree exported twice, differing only in\nline endings, gives two revisions, and converting the CRLF copy back to LF\nrestores the first one exactly. Ruled out as contributors — every exported\nfile is mode 100644, and #85 already moved the generated headers out of the\nsource tree, so what is exported is tracked source and two .in templates.\n\n`* text=auto eol=lf` normalises the working tree rather than only the index,\nand the working tree is what gets exported. Nothing in the repository has\nCRLF today, so this changes no file; it stops a checkout from introducing\none.\n\n`ci/check_recipe_reproducible.sh` runs on each operating system, because\nwhat it catches is invisible from any other. It refuses a working tree with\nCRLF in the exported sources — the state that produced the second revision —\nand then shows that line endings really do change the revision, so that if\nthat ever stops being true the first check is revisited rather than trusted.\nChecked against its negation: giving one exported file CRLF endings makes it\nfail and name the file.\n\nWhat this does not do is make the three platforms stop each exporting and\nuploading a recipe. Byte-identical sources mean they now agree, but the\nidentity of a release is still decided by whichever job wins rather than in\none place. Publishing the recipe once and having each platform upload only\nits binaries against that revision is the shape that fixes it, and it\nchanges the release path — which no pull request exercises until #89 lands,\nso it is not something to change blind.\n\nThe checker runs under `set -euo pipefail`, and refuses to answer when it\ncannot look. `git ls-files | xargs grep`\nreports the status of its last stage, so git failing left the result empty —\nwhich reads exactly like \"no CRLF anywhere\" and passed. Verified: outside a\nrepository that pipeline yields an empty result and would have reported the\ntree clean; the loop that replaces it fails with \"cannot list the recipe's\nfiles\". `conan export | grep` had the same shape and is checked the same way,\nand the revisions are assigned before being made readonly, since `readonly\nVAR=$(...)` reports the status of readonly rather than of the substitution.\n\nThe fixture converts exactly what the recipe is made of — conanfile.py, which\nis the recipe, and the paths it lists in exports_sources — listed by git rather\nthan matched by extension, so the fixture and the property cannot drift apart.\nConverting the whole tree also changed scripts/ and ci/, which are not part of\nthe recipe, and demonstrated \"changing many files changes the revision\": true,\nand not the claim. Verified in both directions: CRLF in src/database.cpp fails\nand names it, CRLF in scripts/benchmark_compare.py does not.\n\nWhat it matches is a carriage return at end of line, not any carriage return.\nA bare \\r inside a string literal, or a file using old Mac line endings, is\nneither CRLF nor the thing this guards against, and a check that fires on the\nwrong thing is a check nobody trusts. Verified with a lone \\r placed\nmid-line: not reported, while a genuinely CRLF file still is.\n\nEvery external command has its status inspected, and every three-way outcome\nstays three-way. grep's 0, 1 and 2 are matched, did not match, and could not\nread — the last is neither \"no match\" nor \"binary\". conan export's output is\nparsed in bash rather than piped through grep, and distinguishes exactly one\nrevision from none and from several different ones, since taking the first\nmatch would hide an output that named two. Each failure was induced and\nobserved: an unreadable file, a failing git archive, a failing conan export,\nan export naming no revision, and an export naming two.\n\nCloses #94",
          "timestamp": "2026-08-09T13:30:37+02:00",
          "tree_id": "685e4191f08ae542ad037e208a53a446444ac0a3",
          "url": "https://github.com/utxo-z/utxo-z/commit/e32eb0a2323947c951f653af6a2c1f4cbadf46ac"
        },
        "date": 1786275233544,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 136627.29,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 323353.53,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 265283.77,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 269350.86,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 360.57,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 420.26,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12091977.98,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12687393.26,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11714519.91,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12272.36,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 8695559.56,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13211940.94,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 126817.57,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 7034.76,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2003.58,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2905.65,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2752.12,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.27,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.11,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 56.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.09,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 55.71,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.17,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "be9844718347a040b8247c5155025dff42d6b518",
          "message": "fix: publish the version a tag names (#96)\n\nPushing `v0.8.1` published `utxoz/0.8.1-commit.179`. The tag said one thing\nand the package said another.\n\n`refs/tags/*` fell through to the branch that derives a development version\nfrom the newest tag, so the earlier\n\n    [[ \"$GITHUB_REF\" == \"refs/tags/\"* ]] && VERSION=$(… 's/^v//')\n\nwas dead code: its result was always overwritten. The intent was written\ninto the derivation and the derivation did not do it. A tag now names its own\nversion, and only branches take the `<last-tag>-commit.<run>` form.\n\nTwo consequences worth stating, because they are the point rather than side\neffects.\n\nA tag that is not a version is now refused instead of laundered. Before this,\n`refs/tags/latest` produced `0.8.1-commit.42` — a green build publishing a\npackage named after a tag nobody pushed. It fails now, as do `v`, `v0.9`\nand a tag carrying a semicolon.\n\nAnd `LAST_TAG` stops deciding a tag's version. A tag ref names the version\noutright, so `refs/tags/v9.9.9` is `9.9.9` whatever the newest tag happens to\nbe; the test pins that with a deliberately unrelated `LAST_TAG=0.1.0`.\n\nRefs are hierarchical, so the version is the whole suffix after the prefix\nrather than the last path component. `archive/v0.9.0` is a different tag from\n`v0.9.0`, and taking the last component published both as 0.9.0 — two tags, one\npackage version, whichever built last winning. Keeping the slash makes the\npattern refuse it, and the same applies to release/foo/0.9.0.\n\nAnd a tag needs no tag list: reading the newest tag now happens only on the\nbranch that uses it. A tag names its own version outright, so a release must not\nstop because `git for-each-ref` failed on something it does not consult. Tested\nfrom a directory that is not a repository, with LAST_TAG unset — git would fail\nif it were asked, and refs/tags/v0.9.0 still yields 0.9.0.\n\nTen tag cases and six branch cases in ci/test_determine_version.sh, including\n`refs/heads/tags/v0.9.0` — a branch that merely looks like a tag ref is still\na branch. Removing the new case turns all ten tag cases red and leaves every\nbranch case green, so the two halves are pinned independently.\n\nCloses #93",
          "timestamp": "2026-08-09T20:27:07+02:00",
          "tree_id": "95ef83e2f677cb51ba67bfb2880aa308796d1fa3",
          "url": "https://github.com/utxo-z/utxo-z/commit/be9844718347a040b8247c5155025dff42d6b518"
        },
        "date": 1786300290085,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 1227741.97,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 13514876.45,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 7444683.84,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 5667448.46,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 519.82,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 656.71,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 15015985.99,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 9462540.09,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 14059651.03,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 14818.05,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 16371751.39,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13553676.84,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 194974.81,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14602.94,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 3223.31,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 6083,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3177.12,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 5.16,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 5.1,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 5.14,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 5.14,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 5.15,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 5.15,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "79668a949759610468b87e2073c44e096c09f6fd",
          "message": "feat: exercise the publish path from pull requests, and check what it published (#98)\n\nThe publish job runs for tags and release branches only, so no pull request\nhas ever executed it. Its profile, its options, its consumer check and its\ncontrol flow reach a release untested — which is how a Windows publish came\nto build `utxoz/None` without anything noticing.\n\nTwo scripts, and both halves of the release now run somewhere a pull request\ncan see them.\n\n`ci/create_package.sh` is everything the real publish does before it touches\nthe network. The dry-run job and the publish job both call it, so the two\ncannot drift into separate implementations: whatever a release does there, a\npull request already did. It cannot upload — not by a flag that could be\npassed wrongly, but because the dry run gives it a cache with no publishing\nremote in it, which the job then asserts rather than assumes.\n\nIt also resolves the package again as an ordinary dependency with\n--build=never. That is not the same check as test_package: test_package runs\nagainst a package the command just built, while this one refuses to rebuild.\nA package that only works when it can be rebuilt from the recipe is not a\npackage.\n\n`ci/verify_published.sh` runs after the upload, because a successful upload\nis not evidence that anything can be retrieved — utxoz/0.8.1 was uploaded by\nthree jobs that each printed a complete summary and is not on the remote\ntoday (#92). It asks from an empty cache, so nothing can be answered by what\n`conan create` left behind, and it reads the remote anonymously, so what it\nverifies is what a consumer sees rather than what the session that just\nuploaded can reach. It requires the exact recipe revision that was created,\nwhich the create step reports, so \"something with that name is there\" does\nnot pass.\n\nSeparating \"it is not there\" from \"we could not ask\" needed its own program.\n`conan list` exits 0 whether the reference exists, is absent, or the remote\nis unreachable — the outcome is a field in its JSON. Reading that with\n`if ! conan list` turns a network failure into a missing package, which is\nthe same mistake as `git tag | head`. `ci/remote_revisions.py` exits 0, 3 and\n4 for the three cases, and claims absence only on the remote's own words:\nasked for a reference it does not have, this remote answers `Recipe 'x' not\nfound`, while asking for that reference's revisions answers with an empty\nerror, which proves nothing. Hence two queries rather than one.\n\ntest_package now opens a database in both storage modes and checks the typed\nfields come back, because a package whose full_db links while reference_db\ndoes not is broken for half its users, and that failure belonged downstream.\n\nVerified against the real remote, uploading and deleting nothing:\n\n    absent (0.8.1)          the upload reported success and utxoz/0.8.1 is not on kth-verify\n    unreachable             could not establish whether utxoz/0.8.0 is on kth-verify; that is not a pass\n    wrong revision          utxoz/0.8.0 is on kth-verify, but not the revision just built\n    correct revision        reaches the install\n\nThe second line is the one that matters most: not knowing is its own\noutcome, and it never becomes \"missing\".\n\nA query that never returns is not an answer either, so it is bounded. Without\na timeout a stalled Conan holds the publish job open until the workflow kills\nit, which reports a timed-out job rather than \"we could not establish whether\nthe package is there\" — and loses the distinction the whole program exists to\nmake. Verified against a conan that sleeps: refused after the bound, as\n\"could not determine\", not as absence.\n\nThe dry run also points that verification at a version the remote cannot\nhave and requires it to say so in those words. Asserting the message rather\nthan a non-zero exit is deliberate — an unreachable remote fails too, and a\ncontrol that accepted any failure would pass while proving nothing.\n\nBoth scripts follow the discipline #95 settled on: set -euo pipefail,\nassignment before readonly, every captured command's status inspected, and\noutcomes that are three-way kept three-way. Reading the created revision back\nno longer pipes `conan list` into python — a pipeline reports python's status,\nso a failed query arrived as an empty revision and could not be told apart from\na package that has none. The query and the parse are separate failures now, and\nci/one_revision.py requires exactly one revision: none and several are both\nrefused, since taking the first of several would pin the publish check to\nwhichever iteration order produced. Checking the revision against the remote's\nlist separates grep's three outcomes, and the consumer executable is found\nwithout `find | head`, which reported head's status and made a failed search\nlook like nothing was built.\n\nci/test_remote_revisions.sh is twelve cases against a conan that answers what\neach one needs: present with the exact revision, absent, an error that is not a\ndefinite answer, conan exiting non-zero, a reply that is not JSON, a reply\nnaming no remote, a query that never returns, a revisions query that fails after\nexistence is established, a reference listed with no revision, several revisions\nall of which must come out, and a remote answering about something else. Each is\nchecked on both its exit code and its message. A real remote cannot be asked to\nfail on cue, and these are the failures that decide whether a release is\nbelieved. Treating any error as absence turns exactly one case red; removing the\ntimeout makes one case never return.\n\njson.loads accepts any valid JSON, not only the shape Conan documents, and the\nwrong shape failed in ways that looked like answers rather than like errors.\n`{\"kth\": \"invalid\"}` made `\"error\" in reply` and `reference not in reply` two\nsubstring tests on a string, and the reply came back as *absence* — the one\nconclusion this program exists to never reach by accident. A string under\n\"revisions\" was worse: it yielded its characters as revisions and exited zero,\nso a malformed listing would have published a revision that never existed.\nEvery level is checked before it is read, in both programs.\n\nA reply that does not mention the reference is no longer absence either. Only\nthe remote's own \"not found\" proves that; a listing about something else proves\nnothing, and treating silence as denial is the same mistake one level up.\n\nci/test_one_revision.sh covers the other program the same way — one revision,\nnone, several, an error, invalid JSON, empty input, a missing reference, no\nLocal Cache, and four shapes that are JSON but not a listing.\n\nDepends on #97: the publish job now runs a shell script, which needs the\ndefault shell to be bash on Windows.\n\nCloses #89",
          "timestamp": "2026-08-09T21:00:04+02:00",
          "tree_id": "9429c051d7119b691c7b902c11ad5439edb07b86",
          "url": "https://github.com/utxo-z/utxo-z/commit/79668a949759610468b87e2073c44e096c09f6fd"
        },
        "date": 1786302193793,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 873221.76,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 1108746.26,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 1166552.75,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1530796.53,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 382.26,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 411.55,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11692642.95,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 9131152.42,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 10979428.09,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11825.36,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12647637.75,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 12376380.86,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 146934.79,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13490.19,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2276.64,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3314.96,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2769.53,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 104.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 105.2,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 105.63,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 106.04,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 105.36,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 106.8,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "6c4a957bb8a1cb46f93db763a2db5223aa3fe0e1",
          "message": "release: 0.9.0",
          "timestamp": "2026-08-09T21:23:56+02:00",
          "tree_id": "4f35a9814bc7e91c01c624338264d106d8883db4",
          "url": "https://github.com/utxo-z/utxo-z/commit/6c4a957bb8a1cb46f93db763a2db5223aa3fe0e1"
        },
        "date": 1786303685545,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 11458488.6,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 12023426.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 1296427.64,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 8051189.99,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 535.66,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 793.07,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12666814.76,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 8108964.66,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11460307.06,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11179.4,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12062977.67,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 11656308.86,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 162163,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 11930.34,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 3208.96,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 5795.54,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2791.42,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 4.49,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 4.49,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 4.56,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 4.61,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 4.57,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 4.59,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f9b84ef3fb174253def56f2d60c3f48c9f06bf72",
          "message": "fix: link bcrypt so consumers of the Windows package can link too (#103)\n\nThe v0.9.0 release failed on Windows before it could upload anything:\n\n    utxoz.lib(database_impl.cpp.obj) : error LNK2019: unresolved external\n      symbol BCryptGenRandom referenced in function generate_merge_id\n    test_package.exe : fatal error LNK1120: 1 unresolved externals\n\n`target_link_libraries(utxoz PRIVATE bcrypt)` resolves nothing for a static\nlibrary. Linking bcrypt while building utxoz does not put BCryptGenRandom into\nutxoz.lib; the symbol stays undefined and whoever links the archive has to\nsupply bcrypt themselves. PRIVATE says they do not have to.\n\nAnd nothing told them otherwise: package_info() declared system_libs for Linux\nand FreeBSD and never mentioned bcrypt, so a Conan consumer had no way to know.\nBoth are fixed, because they are two different audiences — PUBLIC covers anyone\nlinking the CMake target, system_libs covers anyone consuming the package.\n\nThis has been true since generate_merge_id was added. What is new is that a\nconsumer now exists to prove it: nobody had ever linked against the installed\nWindows library.\n\nThe reason it survived to a release is the second half of this change.\n`conan create` — and with it test_package, the only consumer there is — runs on\nall three platforms only inside `publish`, which no pull request executes. The\ndry run added in #98 was Linux-only, so it reported the package consumable\nwhile the Windows one could not link at all. It now runs the same three\nplatforms publish does, with the same compiler selection and the same\nverification, which is what makes this fix demonstrable rather than merely\nplausible: the Windows dry run is red without it.\n\nThe selftests that job runs stay on Linux. Both programs they exercise are\nPython with no platform-specific behaviour, so once is enough — and the harness\nis POSIX by construction: it replaces conan with an extensionless shell script\non PATH, which Windows does not resolve, so every case there asked the real\nremote and reported \"could not determine\". Found by running the job on Windows\nfor the first time.\n\nNothing was retried and nothing was published by hand. The Windows package was\nnever uploaded, which is the correct outcome — a package no consumer can link\ndid not reach the remote.",
          "timestamp": "2026-08-10T00:41:55+02:00",
          "tree_id": "4a6931c30e0dec2d4bfc7636e222aeb9e6af427f",
          "url": "https://github.com/utxo-z/utxo-z/commit/f9b84ef3fb174253def56f2d60c3f48c9f06bf72"
        },
        "date": 1786315505123,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 266598.54,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 329976.08,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 338360.21,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 272257.01,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 378.07,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 429.19,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12106619.85,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12558336.34,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11832419.48,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12102.14,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 11240531.84,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14655741.61,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 140911.89,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 10940.96,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2443.08,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2958.94,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2768.66,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.88,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.8,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 56.44,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.73,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.94,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.89,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "9db0c58271913059ad748d243b137bf5bf57ef75",
          "message": "fix: wait for the remote to publish before deciding it did not (#105)\n\nThe publish job asked whether utxoz/0.9.0 was on the remote 1.3 seconds after\n`conan upload` returned, and failed the release when the answer was no. But an\nupload's 200 is not publication on that remote: the files land in a staging area\nand are published by a later, server-side step. The first answer to this\nquestion is routinely \"not found\" on a release that is perfectly healthy, so the\ncheck was failing for the delay rather than for the fault.\n\nci/await_published.sh now asks repeatedly, with a gap that doubles from 2s to a\ncap of 30s, until either the exact recipe revision appears or a 300s deadline\npasses. Four things make that deadline real rather than decorative:\n\nThe window covers the queries, not merely the gaps between them. Each attempt is\ngiven the smaller of remote_revisions.py's timeout and what is left, so a remote\nthat accepts connections and never answers cannot stretch 300s into 420s.\n\nThe window is never spent on a question it cannot afford. Clamping each query to\nwhat remains makes the last attempt the most starved one, and a query given a\nsecond times out against a remote that is answering perfectly well — a timeout\nthat reads as Unknown and, arriving last, discards every definite answer before\nit. So an attempt that cannot be given time to answer is not made: the run ends\non the last real answer instead, with the unusable remainder deliberately\nunspent.\n\nremote_revisions.py spends one budget on the whole answer. It runs two\n`conan list` calls — does the remote have it, which revisions — and they used to\nget the full timeout each, so a caller that allowed N seconds could wait nearly\n2N. It now takes a monotonic deadline once and hands each call only the\nremainder.\n\nThe clock is monotonic. `date +%s` is the civil clock and it steps: NTP\ncorrecting a drifting runner moves it in either direction, and a deadline built\nfrom two of its readings lengthens or ends early with no sign that it happened.\n\nEvery way the wait can end resolves to one of three outcomes, and the exit code\ncarries which: published, absent after the window, could-not-establish. A\nfailing `sleep` is one of those ways — under `set -e` it used to leave carrying\nits own status, 127 for a command that is not installed, and a fourth value is\nnot a worse answer than Unknown but an answer with no meaning.\n\nAbsence and not-knowing stay separate. A release blocked by the first has a\nfault to fix; one blocked by the second has a question to re-ask, and treating a\nnetwork error as a missing package is how the wrong one gets acted on.\n\nThe delay is not reproducible on demand, so the tests replace conan with one\nthat answers a scripted sequence and replace the clock and the sleep with\nshims — waiting still consumes the deadline, only the real seconds are skipped.\n38 assertions here and two more in test_remote_revisions.sh, each written so\nthat reverting its fix turns it red: two queries sharing one budget rather than\none each, a query that would outlast the window, a window too short to ask\nagain, a wait that fails, a clock that stops and a clock that runs backwards.\n\nThe settings that would disable a guard are refused rather than accepted as\nsmaller values. A floor of zero on the query time is not a shorter floor, it is\nthe starved final query back again; a gap of zero is a busy loop against\nsomebody else's server. Both exit as Unknown with a diagnosis.\n\nThe harness bounds its own runs with `kill -0` and a shell loop rather than with\n`timeout`, which is coreutils and is not present on every platform this has to\npass on. Environments are passed through env(1): assignments prefixed to a bash\nfunction persist after the call, so they would have leaked between cases.\n\nThe dry-run control now asserts the exit code rather than the wording, and runs\nwith a short window: at the release default it would have spent five minutes per\nplatform on every pull request waiting for a version that cannot arrive.\n\nRefs #92",
          "timestamp": "2026-08-10T11:55:46+02:00",
          "tree_id": "8d57040a2915e8660516441b83f840ef802ec8b2",
          "url": "https://github.com/utxo-z/utxo-z/commit/9db0c58271913059ad748d243b137bf5bf57ef75"
        },
        "date": 1786355954050,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 268957.14,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 279991.95,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 291423.96,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 266954.16,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 371.75,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 397.99,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11812011.06,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 10784543.61,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11291461.51,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12003.85,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 9324847.57,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13977477.18,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 73039.06,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 9610.74,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2176.33,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3108.99,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2720.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.74,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.48,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 56.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.58,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.49,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.27,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "9fa6d3e075a467e641b686ee1b850f6ddf0e1a17",
          "message": "release: 0.9.0",
          "timestamp": "2026-08-10T13:18:53+02:00",
          "tree_id": "8d57040a2915e8660516441b83f840ef802ec8b2",
          "url": "https://github.com/utxo-z/utxo-z/commit/9fa6d3e075a467e641b686ee1b850f6ddf0e1a17"
        },
        "date": 1786360919699,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 584161.44,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 865960.26,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 1151410.48,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1404901.55,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 340.96,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 388.16,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12579604.55,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 10285059.4,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11909392.39,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12658.14,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 14225450.52,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14956266.18,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 156456.39,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14945.95,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2222.16,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2996.84,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2748.01,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 110.48,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 113.05,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 113.16,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 110.31,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 113.57,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 112.35,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "a039c2edacb124ea57079551b777f3f178aabd07",
          "message": "docs: update release notes for v0.9.0",
          "timestamp": "2026-08-10T14:26:03+02:00",
          "tree_id": "2c503715392774dc859a8647eb70eb817352395f",
          "url": "https://github.com/utxo-z/utxo-z/commit/a039c2edacb124ea57079551b777f3f178aabd07"
        },
        "date": 1786364952472,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 271520.86,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 325533.6,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 366231.77,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 431470.31,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 388.62,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 485.93,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12096279.75,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 9768747.28,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 7915575.41,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11844.78,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13534415.22,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13113851.85,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 146350.29,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14493.29,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2754.66,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3275.65,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2769.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 57.01,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 56.95,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.78,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.81,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.71,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "a039c2edacb124ea57079551b777f3f178aabd07",
          "message": "docs: update release notes for v0.9.0",
          "timestamp": "2026-08-10T14:26:03+02:00",
          "tree_id": "2c503715392774dc859a8647eb70eb817352395f",
          "url": "https://github.com/utxo-z/utxo-z/commit/a039c2edacb124ea57079551b777f3f178aabd07"
        },
        "date": 1786364979856,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 322489.46,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 365431.81,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 359115.55,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 278545.78,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 365.87,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 437.57,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 13298482.92,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13127063.19,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12729704.99,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13449.37,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 7181530.12,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 11951336.58,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 96034.49,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 7747.8,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2294.59,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3115.15,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2789.49,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 52.7,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 52.91,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 52.77,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 52.88,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 52.8,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a3aeeb52ab2607d840b638c27d080576756a3f48",
          "message": "fix: resolve public dependencies while verifying a published package (#108)",
          "timestamp": "2026-08-10T14:32:32+02:00",
          "tree_id": "ba3993357a9d5fe7c61e684dbad2fe8135dc0ad0",
          "url": "https://github.com/utxo-z/utxo-z/commit/a3aeeb52ab2607d840b638c27d080576756a3f48"
        },
        "date": 1786365784400,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 272844.82,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 364611.17,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 389368.11,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 562199.48,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 373.34,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 457.69,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11997491.05,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12163884.06,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11422955.47,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11969.51,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13669835.49,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14753854.6,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 156659.46,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13634.98,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2499.62,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3152.35,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2732.78,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.94,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 57.62,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 57.66,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.89,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.81,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f8e1666178d290a43faf2665b332dc8201414079",
          "message": "fix: generate the release notes once, before the release exists (#107)\n\nThe notes were generated twice and the two disagreed.\n\npost-release.sh created a `temp-v${VERSION}` tag, made a prerelease from it with\n`--generate-notes`, scraped the body back out, rewrote `temp-v` to `v` with sed,\nwrote that into doc/release-notes/release-notes.md, committed it — and then\ncreated the real release with `--generate-notes` a second time. By that second\ncall the release pull request had merged and the docs commit existed, so what\nGitHub produced was not what the file said. The release also cited itself: the\nmarker pull request was inside its own range, as was the commit that wrote the\nnotes.\n\nNow the notes are a photograph, taken once, before anything about the release\nexists. release.sh asks GitHub for them against the master commit it is about to\nrelease — via `releases/generate-notes`, which computes them without creating a\ntag, a draft or a prerelease — and embeds them in the pull request body between\nmarkers. post-release.sh reads them back out of that same body and uses them,\nbyte for byte, for both the file and the release.\n\nGenerating them first also means the release stops for free when it cannot be\ndone. Everything after that point creates something, so a GitHub failure now\naborts with no state to unwind. There is deliberately no fallback body, and a\npull request this run did not create aborts too: its body carries notes nobody\ngenerated for this commit. The notes are read and validated before\n`gh pr merge`, not after — a body whose markers were lost would otherwise be\nmerged first and refused second, leaving master with a release commit on it, no\ntag and no release.\n\nA failed release leaves nothing behind and can be re-run. The tag must be pushed\nbefore `gh release create` can reference it, so a failure there used to leave the\nremote carrying a v${VERSION} pointing at nothing published; the tag is now\nwithdrawn, and the status reported is gh's own. Withdrawing it only helps if the\nrun can be repeated, and by then the notes commit is already on master — so\nrecording the notes is conditional, and the condition is the text and not just\nthe heading. An entry for this version that says something else is a conflict and\nstops the release: publishing from the pull request while the file kept different\nwords is the divergence this change exists to remove. Identical is the retry\ncase and does nothing; duplicated or truncated headings refuse rather than guess.\n\nprevious_stable_tag distinguishes a repository with no tags from a git that could\nnot answer — returning success with no output for the second would generate notes\nover the whole history and present every version this project ever shipped as the\ncontents of this one. It also no longer pipes git into a `while` that breaks:\nonce the tag list outgrows the pipe buffer git takes SIGPIPE and a caller under\n`pipefail` aborts on the one path that found what it was looking for. Only\n`vMAJOR.MINOR.PATCH` counts as a previous release.\n\nThe markers tell unset from set: unset is defined and frozen, the expected value\nis frozen too rather than merely accepted, and anything else is refused — the\nbody would be written with one marker and read back with another.\n\nGitHub's Full Changelog link points at the tag it was asked about, which does not\nexist while the pull request is open; those links are pointed at the commit\ninstead. `gh api`'s stdout and stderr are kept apart. Markers are counted by\noccurrence rather than by line. Tags are synced from origin before the previous\nrelease is chosen, without --force.\n\nThe existing guarantees are unchanged: no tagging unless the pull request is\nMERGED and its merge commit is an ancestor of the checked-out master, no deleting\nthe branch until the release exists, and nothing touches Conan packages.\n\n99 assertions in scripts/test_release_lib.sh, run on Ubuntu and again on macOS.\nEach fix was checked by reverting it, and four of those checks had to be\nsharpened before they discriminated at all: comparing only the heading, a\nduplicate heading that failed for the wrong reason, a SIGPIPE that needs nine\nthousand tags to appear, and the suite's own line lookups, which recorded misses\nin a counter incremented inside a command substitution and therefore in a\nsubshell that discarded it. The retry cycle runs against a real repository with a\nreal remote and checks that the persisted entry is exactly the text handed to\npublish_release.\n\nRefs #92",
          "timestamp": "2026-08-10T16:33:09+02:00",
          "tree_id": "64b6f7550dfef7d25aed3443c78daad492b5f126",
          "url": "https://github.com/utxo-z/utxo-z/commit/f8e1666178d290a43faf2665b332dc8201414079"
        },
        "date": 1786372588744,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 289435.6,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 325811.56,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 135687.15,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 397282.26,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 371.28,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 429.27,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12147174.41,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12576084.98,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11894781.78,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12395.78,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12709494.72,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14582856.43,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 148887.36,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14210.79,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2576.76,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3319.09,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2764.17,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.01,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.34,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.74,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 55.97,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d45e55c8d4fb5458dc5900d305c40141a40def4b",
          "message": "fix: tell a proven absence from a lookup that could not be made (#110)\n\n* fix: tell a proven absence from a lookup that could not be made\n\nprocess_pending_lookups() returned two lists, and the second one mixed keys that\nexist nowhere with keys nobody managed to look up. A version file that would not\nopen was logged, skipped, and every key still pending came back in that list —\nindistinguishable from a genuine absence.\n\nA consumer decides whether a block is valid by asking whether its inputs exist.\nHanded that list, it reports missing_previous_output and rejects a block that may\nbe perfectly valid, and the cause is a local storage fault it never hears about.\n\nNow the sweep is fail-closed. Every version below the current one is read, or the\ncall returns an error and no lists at all. What survives into the second list was\nlooked for everywhere it could have been, so absence is a fact about the database\nrather than about how the sweep happened to go.\n\nThe cause is carried rather than flattened. A version file that will not open is\nversion_unreadable; a catalogue that cannot be listed is catalog_unreadable,\nwhich already existed and says exactly that. They send an operator to different\nplaces.\n\nNothing is consumed on the error path. The entries the sweep resolved before it\nfailed are kept and put back, so the pending set is left exactly as it was and\nthe call can simply be made again once the storage fault is dealt with. Without\nthat, a retry would have found those keys neither resolved nor pending.\n\nBoth modes, full and reference, get the same treatment: the same catch that\nswallowed the failure, the same unguarded catalogue enumeration.\n\nThe seam that makes this testable is one failpoint on the existing mechanism in\ndetail/durability.hpp, failing the sweep's attempt to open one specific version.\nA specific version and not a blanket switch, because what has to be shown is a\nsweep that reads some files and then cannot read one — the case where a partial\nresult exists and must not be handed back as absence. A blanket failure stops at\nthe first file and never reaches it.\n\nThe deterministic tests are not in this commit. They are written against this\nseam and they have not been run, so they are not here.\n\nRefs #92\n\n* test: pin the sweep's contract with a deterministic unreadable version\n\nEight cases across both modes, each with one outcome. The failure is injected\nthrough the existing failpoint mechanism rather than arranged on disk: a\ncorrupted file produces a test that passes for reasons nobody chose, while\nnaming one version fails exactly that open.\n\nThe fixture rotates container 0 twice and container 1 once, so the sweep has\nseveral previous versions to walk and the failed open lands after entries have\nalready been consumed. That position is not assumed — reverting the restore\nturns the retry red with `deferred_lookups_size() == 1` against an expected 3,\nwhich is the two consumed entries not coming back.\n\nThe retry is what carries the weight. It is not enough that the second sweep\nsucceeds: it has to return both resolvable keys and the one genuine absence,\nbecause that is the only thing that shows the whole pending set was restored and\nnot just the entries the failure never reached.\n\nA second failpoint covers the catalogue, so a version that will not open and a\ncatalogue that cannot be listed cannot be shown to work by the same test. They\ncarry different codes and different tests assert each.\n\nOne existing test changes with the contract. \"a truncated version reached\nthrough the file cache is refused promptly\" required the sweep to succeed and\nhand the witness back in the unresolved list — it was pinning the ambiguity\nitself. Its real subject is the timing, and that is kept: the error still has to\narrive in under five seconds, and version_unreadable is only produced by the\nopen path, so receiving it is itself evidence the cache was reached rather than\nthe call failing early.\n\nVerified against all three negations. Removing the incomplete-sweep marking\nfails 4 cases on false absences. Not restoring the consumed entries fails the\nretries. Returning the wrong code fails the two cause controls and nothing else.\n\nFull suite: 115 cases, 25920790 assertions, green.\n\nRefs #92\n\n* fix: do not publish statistics from a sweep that was rolled back\n\nA failed sweep restores its queue, so the retry does the same work again. The\ncounters were being written as the sweep went, so an abandoned attempt was\ncounted alongside the one that finished: files visited twice, lookups resolved\ntwice, and a processing run that never produced a result. These are what an\noperator reads to decide whether the deferred path is behaving.\n\nThey are now accumulated locally and published only once the sweep is known to\nhave covered everything. Nothing is reset and nothing is rolled back\napproximately: an attempt that did not complete simply never appears.\n\nThe header also promised the wrong thing. It said a version file and an\nunreadable catalogue both give version_unreadable; the code has told them apart\nsince the previous commit, and the two send an operator to different places. The\ncontract now names both codes, in full and in reference.\n\nTwo cases, one per mode: snapshot, fail after a partial resolution, check every\nobservable counter is unchanged, clear the failpoint, retry, and check the\ncompleted sweep counted each lookup exactly once. Both run with statistics\nenabled and disabled — with them off the counters are all zero, which is what\nmakes the unchanged-on-failure half meaningful in that build too.\n\nprocess_pending_deletions() was audited and deliberately left alone. Its second\nlist carries the same ambiguity, but no caller in this repository reads it as\nabsence or as a discharged obligation: basic_usage.cpp prints the count, and\nblockchain_processing.cpp prints it and logs an error when it is not empty.\nUnlike the lookup sweep, the entry comes back with its key and height, so the\nobligation is handed to the caller rather than disguised as a fact. Changing it\nfor symmetry would be changing a contract nothing has been shown to misread.\n\nVerified against four negations. Removing the incomplete-sweep marking fails 6\ncases; not restoring the consumed entries fails 3; the wrong cause code fails\nexactly 2; publishing statistics during the sweep fails exactly 2.\n\nFull suite, statistics on:  117 cases, 25920844 assertions.\nFull suite, statistics off: 112 cases, 25812438 assertions.\n\nRefs #92\n\n* refactor: remove the untyped sweep and tidy two loose ends\n\nThe untyped database_impl::process_pending_lookups() and its\nprocess_deferred_lookups_in_file() helper had no callers: the public entry\npoints go through the typed full_ and reference_ implementations. Dead code\nwould be reason enough, but this block was also the last copy of the contract\nthis branch removed — it logged an unreadable version, carried on, and returned\nthe keys it never resolved beside the ones that are genuinely absent. Left in\nplace it is a route back to the defect, and it kept catching stray edits during\nthis work for exactly that reason.\n\nDeclarations, definitions and every reference are gone; the full and reference\nAPIs are untouched.\n\nThe example in the header dereferenced its result after the error branch, with\nnothing to stop control reaching it. Copied as written that is undefined\nbehaviour, so it now returns before it gets there and says why.\n\nThe failpoint sentinel is declared before the atomic it initialises, so the\nvalue is written once instead of twice.\n\nStatistics-disabled expectations are deliberately unchanged. record_unresolved()\ncompiles to an empty body when UTXOZ_STATISTICS_ENABLED is not defined and\nget_summary() returns a zeroed struct, so the counters stay at zero even though\nthe call sits outside the #ifdef — verified by asserting it before reverting the\nassertion. Wrapping the call sites would be working around a no-op interface\nthat exists precisely so they do not have to.\n\n[unresolved], statistics on : 10 cases, 210 assertions.\n[unresolved], statistics off: 10 cases, 206 assertions.\nFull suite,   statistics on : 117 cases, 25920844 assertions.\nFull suite,   statistics off: 112 cases, 25812438 assertions.\n\nRefs #92",
          "timestamp": "2026-08-10T18:04:30+02:00",
          "tree_id": "644fed45cc850088c2ed531e3fed84f063f71c6b",
          "url": "https://github.com/utxo-z/utxo-z/commit/d45e55c8d4fb5458dc5900d305c40141a40def4b"
        },
        "date": 1786378091936,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 285510.35,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 422291.08,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 253308.65,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 279211.28,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 398.14,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 452.44,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12699055.42,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11875082.84,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11956780.97,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12676.52,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 10493371.47,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 12802142.99,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 106581.71,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 8622.15,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2227.15,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3212.69,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2731.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 57.02,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.05,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.76,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.66,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "8398147c956b9dc23c0de2a65973d5ad314fd8db",
          "message": "release: 0.9.1",
          "timestamp": "2026-08-10T18:05:26+02:00",
          "tree_id": "644fed45cc850088c2ed531e3fed84f063f71c6b",
          "url": "https://github.com/utxo-z/utxo-z/commit/8398147c956b9dc23c0de2a65973d5ad314fd8db"
        },
        "date": 1786378125212,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 894924.38,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 941810.14,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 918090.98,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 898384.74,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 311.47,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 390.7,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12589713.94,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 10021577.37,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12079672.26,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12702.04,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12367647.11,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 11926809.49,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 113423.97,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 12648.23,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2464.15,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3476.56,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2416.27,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 109.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 110.87,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 111.01,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 110.66,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 110.33,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 110.75,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "210127610f8c1c472531441a08552afbe4e6bcf4",
          "message": "docs: update release notes for v0.9.1",
          "timestamp": "2026-08-10T18:28:28+02:00",
          "tree_id": "e0d93f2771c36efae29c7e82ccf1a4368540e0cb",
          "url": "https://github.com/utxo-z/utxo-z/commit/210127610f8c1c472531441a08552afbe4e6bcf4"
        },
        "date": 1786379506402,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 399643.11,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 709937.11,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 717681.89,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 877729.84,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 428.3,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 539.1,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12877691.89,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 13243944.09,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12458755.04,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12928.94,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 15126677.41,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 15424874.85,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 156182.55,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 14390.49,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2890.22,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3683.9,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2941.76,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.93,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.11,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.04,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.97,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 55.31,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b2e1514812fc1ce41a247055205603c438b561e4",
          "message": "release: 0.9.1 (#111)",
          "timestamp": "2026-08-10T18:28:23+02:00",
          "tree_id": "644fed45cc850088c2ed531e3fed84f063f71c6b",
          "url": "https://github.com/utxo-z/utxo-z/commit/b2e1514812fc1ce41a247055205603c438b561e4"
        },
        "date": 1786379521781,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 189381.98,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 213786.68,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 260041.95,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 459151.43,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 378.03,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 457.93,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12781350.56,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12643945.18,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12517452.6,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12757.83,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 7384298.37,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13367096.26,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 78207.2,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 7855.27,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1940.17,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2928.8,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2951.55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 52.59,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 52.55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 52.89,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 52.84,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 52.54,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 52.26,
            "unit": "ops/sec"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "committer": {
            "email": "fpelliccioni@gmail.com",
            "name": "Fernando Pelliccioni",
            "username": "fpelliccioni"
          },
          "distinct": true,
          "id": "210127610f8c1c472531441a08552afbe4e6bcf4",
          "message": "docs: update release notes for v0.9.1",
          "timestamp": "2026-08-10T18:28:28+02:00",
          "tree_id": "e0d93f2771c36efae29c7e82ccf1a4368540e0cb",
          "url": "https://github.com/utxo-z/utxo-z/commit/210127610f8c1c472531441a08552afbe4e6bcf4"
        },
        "date": 1786380278081,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 283350.85,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 279151.73,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 283955.76,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 152208.36,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 401.55,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 426.49,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11869931.21,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12712025.57,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11408168.6,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12240.56,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13737123.92,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14949084.28,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 130045.94,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 11714.95,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1793,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2821.91,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2809.76,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 55.55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.8,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.98,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.31,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.18,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.5,
            "unit": "ops/sec"
          }
        ]
      }
    ]
  }
}