window.BENCHMARK_DATA = {
  "lastUpdate": 1771180389998,
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
      }
    ]
  }
}