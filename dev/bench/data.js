window.BENCHMARK_DATA = {
  "lastUpdate": 1786015618205,
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
      }
    ]
  }
}