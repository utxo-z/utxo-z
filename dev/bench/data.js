window.BENCHMARK_DATA = {
  "lastUpdate": 1771106360242,
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
      }
    ]
  }
}