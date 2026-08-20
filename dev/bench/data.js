window.BENCHMARK_DATA = {
  "lastUpdate": 1787235704068,
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
          "id": "a127997d9417ffaa9d206b4a1bff1e2bee8628c4",
          "message": "fix: carry the database path to the filesystem in its native form (#115)\n\n* fix: carry the database path to the filesystem in its native form\n\nopen() and open_for_testing() took std::string_view. On POSIX that was exact —\na path is bytes and path::string() hands those bytes back — but on Windows a\nstd::filesystem::path holds wchar_t, and path::string() converts through the\nactive code page. A directory named outside that page came back replaced, or\nthe conversion threw, and the user saw a path that did not exist rather than an\nencoding error. Knuth passes path.string() today because the API left it nothing\nbetter to pass (#109).\n\nThe parameter is now std::filesystem::path, taken by value. A caller holding a\npath passes the path:\n\n    db.open(dir.string());   // before\n    db.open(dir);            // after\n\nThere is one parameter type rather than an overload set, and that is the whole\ndesign. A std::string_view overload beside a std::filesystem::path one makes\nopen(\"literal\") ambiguous — const char[N] reaches both by a user-defined\nconversion of equal rank — so adding an overload would have broken the callers\nthis is trying not to break. A single path parameter accepts every form that\ncompiled before: literals, const char*, std::string and std::string_view all\nconvert implicitly. The test file asserts each of those, unevaluated, and pins\nthe exact signature, so a revert or an added overload fails to compile on every\nplatform rather than only where it would misbehave.\n\nChanging the signature alone would have fixed nothing. The path was narrowed\nagain inside: twenty-three sites built file names with\nfmt::format(\"{}/cont_{}...\", db_path_.string(), ...), and handed the resulting\nnarrow string to Boost.Interprocess. Names are now composed as db_path_ / an\nASCII filename, so only the filename is ever formatted and the directory is\nnever converted. Boost.Interprocess has const wchar_t* overloads under\nBOOST_WINDOWS, so path::c_str() — wchar_t const* on Windows, char const* on\nPOSIX — reaches it natively on both. file_cache holds an fs::path, and\nopen_existing_segment takes one.\n\nDiagnostics go through path_display(), which renders UTF-8 via u8string()\ninstead of string(). This is not tidiness: string() throws when a component will\nnot fit the code page, so the line meant to explain a failure would have thrown\nfrom inside the handler for it and lost the original error. Four pre-existing\n.string() calls in config-file error paths are converted for that reason.\n\nOne bug this found in itself: the file cache was constructed from the configure\nparameter after that parameter had been moved from. fs::path converts implicitly\nto its native string type, so it compiled and produced an empty base path, and\nevery historical version file was looked for in the working directory. Fourteen\nexisting tests caught it.\n\ntests/test_path_encoding.cpp opens, writes, closes and reopens a database under\na directory named with U+00F1, U+0434 and U+65E5, in both storage modes, and\nchecks the files landed in that directory rather than somewhere a conversion put\nthem. The name is built from universal character names inside a u8 literal, not\nfrom raw bytes, so a compiler invoked without /utf-8 exercises the same name as\none invoked with it — the alternative would have been the same class of bug as\nthe one under test.\n\nThe case that reports the old conversion says which of two things it observed\nrather than claiming a proof it does not have: on Windows outside the code page\nit records that path::string() threw or came back different, and that the\nnarrowed name does not name the database that was just created; on POSIX, and on\nWindows with a UTF-8 code page, it warns that the run proved no-regression only.\nPOSIX never had this bug and no test here can pretend it did.\n\n* fix: stop passing paths through parameters only POSIX can convert them to\n\nThe Windows job would not compile. Three helpers still took std::string const&\nand were being handed the fs::path the new *_path() accessors return —\nremove_if_present(), recover_one() and the retire lambda in the compaction path.\n\nOn POSIX that compiles silently: fs::path::string_type is std::string, so\noperator string_type() converts implicitly and exactly. On Windows string_type\nis std::wstring, there is no implicit conversion to std::string, and the call is\nill-formed. A Linux build could not have caught it, and a Linux build is what it\ngot. They take fs::path const& now, which also removes the last narrowing those\nthree call sites were performing on POSIX.\n\nenumerate_versions() had the same conversion in a place that fails at run time\nrather than at compile time: it narrowed every directory entry with\nfilename().string() before matching it. The names it recognises are ASCII, but\nthe directory it reads may hold anything, and string() throws for a name the\ncode page cannot spell. Thrown from there it would leave a fail-closed,\nresult-typed enumeration by exception — over a file it was about to ignore. It\nmatches on path_display() now; the prefixes and suffixes are ASCII, so comparing\nUTF-8 answers the same question.\n\nNo .string() remains on a filesystem path anywhere in src/.",
          "timestamp": "2026-08-11T10:55:57+02:00",
          "tree_id": "0e8ff414224d9cdf743beea9f4f78d80e19536cc",
          "url": "https://github.com/utxo-z/utxo-z/commit/a127997d9417ffaa9d206b4a1bff1e2bee8628c4"
        },
        "date": 1786438804626,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 157030.55,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 260271.43,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 241615.93,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 276115.97,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 345.45,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 387.64,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11947432.47,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 11585492.98,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11327614.57,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 10763.72,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 7911582.37,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13438388.62,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 90321.5,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 7664.51,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2009.96,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2721.12,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2904.12,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 55.4,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.06,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.04,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.7,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.53,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.54,
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
          "id": "0e8958d4f3fdd9792c09d7e38cb138da9ad0eea0",
          "message": "fix: report the release gate apart from the checks below it (#114)\n\n* fix: report the release gate apart from the checks below it\n\nThe 0.9.1 Windows publish job failed a release that had published. The recipe\nrevision reached kth-verify, a clean cache downloaded it and the Windows binary\nwith --build=never, and then the verification consumer would not compile:\ntest_package.cpp includes <fmt/format.h> while its CMakeLists.txt declares one\npackage, utxoz. On Linux and macOS that include resolved anyway, through fmt\nreaching the compiler as a dependency of utxoz rather than as anything the\nconsumer asked for. On Windows it did not. The run read as a lost release.\n\nThree questions were being answered as one. A release is published when its\nexact recipe revision is on the remote and comes back from an empty cache. A\nprebuilt binary is an acceleration — a consumer without one builds from the\nrecipe. A consumer that compiles and runs is a diagnostic. They now have their\nown exit codes and their own line in a verdict that is printed on every path:\n3 and 4 keep meaning absence and not-knowing, 5 is a revision the remote lists\nand will not hand over, 6 is a missing binary and 7 a failed consumer. Every one\nof them is still red. What changed is that 6 and 7 say, in the same breath, that\nthe recipe published.\n\nThe required gate now retrieves rather than only asks. Listing a revision and\nserving it are different claims, and a release rests on the second, so the\nrecipe is downloaded into the empty cache and its presence there checked.\n\nThe consumer no longer reaches for a package it does not declare. Nothing under\ninclude/utxoz includes fmt, so it was never part of the contract this test\nexists to measure; the printing moved to <iostream>, which every toolchain that\ncan build the package already has. A diagnostic that fails because a compiler's\nC++23 library is incomplete would report utxoz as broken when it is not.\n\nThe consumer stage moved to ci/verify_consumer.sh so it can be run against a\nlocally created package, which is what makes the new control non-vacuous: the\npull-request job builds the same consumer twice against the same package, as it\nships and with its utxoz dependency stripped out, and fails if the second one\nsucceeds. Until now nothing had established that this check could fail at all.\n\nci/test_verify_published.sh drives the script against a conan and a cmake that\nanswer on cue, because a remote cannot be asked to serve a recipe and withhold a\nbinary. Every failing case asserts that the verdict does not say the recipe is\nabsent. A real unreachable remote is exercised too, and required to report not\nknowing rather than absence.\n\nThe Conan options are scoped to utxoz/*. Bare options are ambiguous once the\nroot is a requirement rather than the recipe in this directory, and Conan says\nso on every install; scoping them keeps the package id identical and the warning\ngone.\n\n* fix: build the verification consumer in the configuration the package has\n\nThe Windows dry run failed on the consumer check with\n\n    error C1083: Cannot open include file: 'utxoz/config.hpp'\n\nwhich reads as a package that ships no headers. It ships them. CMake picks a\nmulti-config generator by default on Windows, and a multi-config generator\nignores CMAKE_BUILD_TYPE and builds Debug when `cmake --build` is given no\n--config. The package installed for the check has Release binaries only, so\nCMakeDeps has no data file for the Debug configuration and the utxoz target\ncomes out with no include directories at all.\n\nSo the build asks for the configuration that was installed. Single-config\ngenerators, which is everything the Linux and macOS jobs use, ignore the flag —\nwhich is why this only ever surfaced on Windows, and only once the consumer\ncheck was run somewhere that reached the compiler.\n\n* fix: identify the consumer executable exactly, and keep the search's own status\n\nTwo ways the consumer check could report the wrong fault, both from review of\nthe post-upload verification.\n\n`find`'s status was collected inside a process substitution, which inherits\nerrexit. A find that exited non-zero — an unreadable directory, a bad predicate\n— terminated the subshell before the status was written; reading the missing\nfile then failed, set -e exited 1, and verify_published.sh maps 1 to \"the\nconsumer does not configure\". A search that could not run was reported as a\npackage that does not build, which is the conflation that block was written to\nprevent, reintroduced by the machinery meant to prevent it.\n\nThe results now go to a file and the status comes from find itself, so the\ncapture is `|| find_status=$?` in this shell rather than something that has to\nsurvive errexit in a subshell. Disabling errexit inside the substitution would\nalso work; not having the subshell is simpler to be sure about.\n\nThe name is now exact. `test_package*` also matches what the build leaves beside\nthe executable — test_package.cpp.o, test_package.lib, test_package.pdb, the\ntest_package.dir tree — and Git Bash on Windows reports ordinary files as\nexecutable, so `-perm -u+x` does not exclude them there. The check could pick an\nobject file and \"run\" it. The CMake target is test_package and the only platform\nvariation is the .exe suffix, so both are named and nothing else matches.\n\nTwo candidates is now its own refusal rather than a first-match. A build tree\nholding two things called test_package means the search is not identifying what\nit thinks it is, and picking one would hide that behind a pass.\n\nci/test_verify_consumer.sh covers what had no test: an object file, an import\nlibrary, a PDB and a build subdirectory are each rejected while the real\nexecutable beside them is chosen; the Windows name is accepted; two candidates\nare refused; a consumer that runs and fails keeps exit 4; and an unreadable\nbuild tree comes back as the search fault with its diagnostic, asserted not to\nbe either of the two codes that blame the package.\n\nBoth fixes were checked by reverting them: restoring the wildcard fails the five\nidentification cases, and restoring the process substitution fails the\nunreadable-tree case with exit 1 — exactly the reported conflation.\n\nThe workflow now greps for the success line rather than only checking the exit\nstatus, because on Windows \"it exited 0\" is not by itself evidence that what ran\nwas the consumer.\n\n* fix: resolve the consumer's directories before deriving anything from them\n\nBUILD_DIR is derived from INSTALL_DIR, and `find` prints paths that begin with\nthe directory it was given, so a relative INSTALL_DIR produced a relative\ncandidate. The run then happens from a scratch directory on purpose — the\nconsumer creates databases under its working directory — and a relative path\nresolved from there names nothing. The exec failed with 127 and arrived as\nEXIT_RAN_AND_FAILED: \"it built and then did not work\", reported about a consumer\nthat was never started.\n\nCI passes paths built from $RUNNER_TEMP and never hit this. A caller working\nfrom its own directory would have, and would have been told the package was\nbroken.\n\nBoth arguments are now made absolute before TOOLCHAIN or BUILD_DIR are derived,\nwith `cd && pwd` rather than `realpath`, which is GNU coreutils and not on a\nstock macOS. That needs the directories to exist, so both are checked first and\nthe install directory gets its own diagnostic instead of being discovered later\nthrough a missing toolchain file. The exit codes and every existing message are\nunchanged.\n\nThe new case calls the script from another directory with both arguments\nrelative, and asserts a token the emitted executable prints rather than its exit\nstatus. Exit 0 alone cannot tell \"the consumer ran\" from \"nothing ran and\nnothing noticed\", which is the failure being fixed — so every success case now\nasserts the token too.\n\nChecked by removing the normalisation: the case fails with exit 4, and the run\nunderneath it reports `install/consumer-build/test_package: No such file or\ndirectory` followed by \"the consumer built but exited 127\" — the reported\nmisattribution, reproduced exactly.",
          "timestamp": "2026-08-11T11:48:33+02:00",
          "tree_id": "c6d2761fce2af9b3e6a07c4c13466bd51bc0c953",
          "url": "https://github.com/utxo-z/utxo-z/commit/0e8958d4f3fdd9792c09d7e38cb138da9ad0eea0"
        },
        "date": 1786441926556,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 280532.04,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 350066.79,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 264019.87,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 438938.31,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 373.44,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 430.37,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11976115.24,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 12516439.09,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11312275.14,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12156.75,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 13751251.35,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 13592089.86,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 127848.19,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13517.63,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2448.5,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3083.7,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2837.9,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.42,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 55.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.98,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.11,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.35,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.35,
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
          "id": "923fc72752a348f40db6b701c5c3c4c169a56491",
          "message": "feat!: the caller owns its lookups, and hands them to resolve() (#118)\n\n* feat!: the caller owns its lookups, and hands them to resolve()\n\nBREAKING CHANGE: process_pending_lookups() is replaced by resolve(span).\n\nfind() wrote every miss into a database-wide set and process_pending_lookups()\nemptied all of it, so the answer to a lookup went to whoever swept next rather\nthan to whoever asked. Nothing in the queue recorded who had asked, and nothing\ncould: the requester's identity was discarded at the point of entry.\n\nWith one component doing both, that is invisible. With two it is a time-of-check\nto time-of-use race across two calls that were never atomic with respect to each\nother. A sweeps, B sweeps first and clears the queue, and A gets back a result\nset that does not mention the key it asked about — neither found nor absent,\nsimply missing. The mirror case is worse, because it is confident rather than\nempty: A's sweep returns keys B queued, including, in the second list, absences\nproven on B's behalf. KTH confirmed the race. The fail-closed work in #110 does\nnot cover it — an absence proven correctly and delivered to the wrong caller is\nstill an absence delivered to the wrong caller.\n\nThe documented mitigation was that exactly one component may own the sweep and\nmust route results back. That is not a fix. It rebuilds outside the library the\nownership the library threw away; it contradicts a class contract that invites\nconcurrent find() calls from N threads; the lookups-before-deletions ordering it\ndepends on is a property of the whole process rather than of one batch; and\nnothing can check it, so both the correct and the broken deployment compile and\npass.\n\nSo the queue is gone rather than guarded. resolve() reads the caller's span,\nanswers exactly the requests in it, and keeps nothing. Two components can each\nhold a batch without agreeing which of them is allowed to sweep, and neither can\nreceive or consume the other's requests — not because a rule forbids it, but\nbecause there is no shared container for it to happen in.\n\nfind() searches the active versions and returns error_code::not_resolved, which\nis what it means: not answerable here. It is no longer spelled not_found, which\ninvited exactly the reading that made it dangerous, and it registers nothing.\n\nThe failure paths get simpler rather than more careful. A resolution that cannot\nread a version still returns version_unreadable or catalog_unreadable and no\nlists at all, but there is no rollback to get right any more: nothing was\nconsumed because nothing was taken, so the caller retries the same vector.\n\nDuplicate keys collapse, so a batch naming one outpoint twice asks one question\nand gets one answer. The working set is indices into the caller's span, not\ncopies of it — measured at ~6 ns per request, flat in batch size, against 35-163\nns for the resolution itself.\n\nRemoved: db_base::deferred_lookups_size(), full_db/reference_db\n::process_pending_lookups(), deferred_lookup_entry (now lookup_request),\nstd::hash<deferred_lookup_entry>, container_stats::deferred_lookups (which\ncounted a queue and was never written), and the internal deferred_lookups_,\nadd_to_deferred_lookups(), full/reference_process_pending_lookups().\n\nDeferred deletions are untouched here. erase()/process_pending_deletions() keep\nthe same global queue and the same single-owner caveat; that is the same defect\nin the write path and belongs in its own change.\n\ntests/test_lookup_ownership.cpp pins the properties that the argument is what\nbuys: a resolution returns only its own batch however many finds ran first, two\nbatches with disjoint keys do not leak into each other, two threads each owning\na batch neither steal nor lose requests, and a failure leaves both batches\nintact. Both storage modes, case for case.\n\n* fix: keep lookup statistics out of the deletion counters\n\ndeferred_stats belongs to deletions, and a resolution was writing into it:\nprocessing_runs, successfully_processed, failed_to_delete, total_processing_time\nand a per-depth histogram in lookups_by_depth. An operator reading\nfailed_to_delete saw deletions that failed plus outpoints that were looked up\nand are legitimately not stored — unrelated events summed into one number — and\nsuccessfully_processed moved for both paths too. Neither described anything.\n\nThe resolvers now write resolution_stats_ and nothing else. That is enforced\nrather than agreed: resolve() is const and deferred_stats_ is no longer mutable,\nso a write across the boundary does not compile. Checked by putting one back —\n\"increment of member 'processing_runs' in read-only object\", both resolvers.\n\nlookups_by_depth goes with it. Nothing else ever wrote it, and the resolution\ndepth story is already told by resolution_summary::avg_depth.\n\nresolution_summary::unresolved becomes absent. It is published only by a\nresolution that covered every version it had to, where every remaining key was\nlooked for everywhere it could have been — so it always counted proven absences\nwhile its name and its doc comment said \"could not settle\". CodeRabbit read the\nname and proposed moving the call to the failed branch; the name was the thing\nthat was wrong. Moving it would have published statistics from an attempt that\nwas abandoned, which is what the enclosing design exists to prevent. Renamed\nthrough the field, the recorder, the docs and the tests.\n\nThe header example is a complete function now, so its returns agree and it\ncompiles as written, and it shows results being matched back by key.\n\nDuplicate handling is stated where the types are defined: the batch is\ndeduplicated by key keeping the first occurrence, found.size() + absent.size()\nis the number of distinct keys rather than requests.size(), and the lists are\nnot positional. A caller that sent one outpoint at two heights gets one entry\nback, carrying the first height.\n\nAlso from review: direct includes for what the ownership test uses rather than\nrelying on database.hpp to pull them in, [[nodiscard]] on both impl resolvers,\nsize_t for the working-set indices instead of uint32_t, and the queue, consume\nand restore wording removed from test_unresolved_vs_absent.cpp — resolve()\nborrows the batch, so there is nothing to consume and nothing to give back.\n\nThe new controls are discriminating rather than incidental: a resolution that\nfinds one key and proves one absent must move only the resolution family; a\ndeletion must move only its own; and an incomplete resolution must move neither.\nPutting the absent count back on the failed path fails five of them.\n\n* test: build the deletion case its own database, and merge a stray doc block\n\nThe deletion counterpart erased one of the shared fixture's filler keys. The\nfile's own header says nothing here mutates the database, and the cases above\ndepend on that: a fixture that one case writes to makes their independence a\nproperty of the order Catch2 happens to run them in.\n\nSo the fill loop comes out of full_fixture() as fill_until_rotations(), used\nboth to build the shared fixture and to give this case a database of its own.\nOne rotation is enough here and costs half of what the shared two do. The\nassertions are unchanged, and both halves of the premise are still pinned: the\nerase returns 0, so it really was deferred, and the sweep reports one applied,\nso it really did the work whose counters the case then reads.\n\nThe header claim is corrected rather than left to be read charitably. It now\nsays the shared fixtures are untouched and names the one case that writes.\n\nAlso merges a standalone comment on deferred_stats into the Doxygen block above\nit, which had ended up with two adjacent comments after the statistics split.",
          "timestamp": "2026-08-11T13:21:37+02:00",
          "tree_id": "f022085a66545bc8c214cb65fb5be7f52dbaf490",
          "url": "https://github.com/utxo-z/utxo-z/commit/923fc72752a348f40db6b701c5c3c4c169a56491"
        },
        "date": 1786447494236,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 267444.69,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 274843.25,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 263153.43,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 263047.77,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 409.37,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 439.53,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12146097.97,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 25913982.18,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11496046.43,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12099.19,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 12292401.6,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 14418406.27,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 130462.55,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 13248.34,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2413.19,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3169.34,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2759.97,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.67,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 56.59,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.02,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.41,
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
          "id": "58fa08d2ba41ae16692787d9fd4fefa12d86aa9d",
          "message": "fix: serialise resolutions, so resolve() can be called from two threads (#123)\n\n* fix: serialise resolutions, so resolve() can be called from two threads\n\nresolve() reads the LRU file cache, and the cache has no synchronisation. Two\nthreads resolving disjoint batches produced 90 ThreadSanitizer data races in\nfile_cache::get_or_open_file and exit 139 — a segfault, not a torn read (#120).\n\nTwo faults share that call site. The smaller one is bookkeeping: cache_,\naccess_frequency_, gets_, hits_ and the per-entry last_used and access_count are\nall mutated unguarded. The one that crashes is lifetime. The cache returns a\nreference into a managed_mapped_file it owns, and evict_lru() destroys that\nsegment; a second resolution evicting a mapping the first is still reading\nunmaps the memory underneath it. Locking the contents would not have helped, as\nthe cache's own header has said all along. max_cached_files_ defaults to 1, so\nnearly every miss evicts and two threads on different containers evict each\nother almost every file.\n\nSo the lock is held for the whole of full_resolve() and reference_resolve(),\nfrom before the first read to the return, rather than around the cache calls.\nAnything narrower protects the bookkeeping and leaves the references dangling,\nwhich is the half that segfaults. Being a function-scope scoped_lock, it also\ncovers the error returns and the empty-batch early return.\n\nThis is not a step towards a reader/writer cache. With a one-entry cache,\nreference-counted segments would let two threads overlap only to spend the\noverlap thrashing the LRU; that shape needs a much larger cache and a profile\nshowing contention is the bottleneck, and resolution is dominated by mmap page\nfaults rather than by CPU. Serialising is the whole fix until there is evidence\nasking for more.\n\nresolve-vs-resolve only. insert(), erase(), process_pending_deletions() and\ncompact_all() touch the same cache and stay the caller's to serialise.\n\nfind() needs no lock and gets none. It reads the active containers, which live\nin segments_ and are not the cache's to evict, and writes only its own sharded\nprobe counters; a resolution reads the older versions through the cache and\nwrites only the resolution counters. Disjoint mappings, disjoint counters.\n\nThe controls run both storage modes with no lock of the caller's, two disjoint\nbatches, forty barrier-synchronised rounds, plus find() and resolve() together\nfor two hundred rounds exercising both of find()'s paths. Clean under\nThreadSanitizer, and removing the two scoped_locks brings back 105 races —\nevict_lru among them — and exit 66. A sequential case resolves the same batches\nthe same number of times on one thread, so a disagreement between the two can\nonly be the concurrency.\n\nThe fixtures gain one key inserted after the fill, so it is in the active\nversion and find() has a hit path to exercise rather than only a miss path.\n\n* docs: say what the read path may now do concurrently, and what it may not\n\nThe threading contract still described the state before the lock: that resolve()\nbelongs on the \"one at a time\" list, that caller-owned batches make two\nresolutions \"independent, not concurrent\", and that find() is permitted only\nwhile no resolution can run. All three are now false, and a caller reading them\nwould build serialisation it does not need — or worse, conclude the pairing is\nunsafe and avoid the API.\n\nWhat it says instead is exactly what is true and no more:\n\n  - resolve() may be called concurrently. The library serialises resolutions\n    with a lock of its own, held for the whole call rather than around the cache\n    lookups, which is what covers the lifetime of every mapping reference the\n    call obtains. The caller arranges nothing.\n  - find() may run alongside resolve(), because they touch disjoint state: the\n    active containers and the probe counters on one side, the older versions\n    through the cache and the resolution counters on the other. Eviction cannot\n    reach the active containers; they are separate mappings. Stated as\n    demonstrated rather than reasoned, with a pointer to the ThreadSanitizer\n    cases that show it.\n  - none of that extends to insert(), a deletion, compaction or close(). A\n    rotation inside insert() unmaps the active segment outright, and\n    process_pending_deletions() writes through the very mappings a resolution\n    reads.\n  - the lock covers resolve-vs-resolve. It does not make the database\n    thread-safe, and the text says so in those words rather than leaving the\n    scope to be inferred from what is absent.\n\nThe cache bullet is corrected the same way: resolve() is safe against another\nresolve() because it holds that lock across its whole use of the references,\nwhile erase() and process_pending_deletions() touch the same cache and are not\ncovered.\n\nBoth the header and the README, which had drifted into saying opposite things\nabout the same call.\n\n* docs: fix a missing auxiliary in the threading note\n\n\"Callers need arrange nothing\" was missing its verb. Says \"Callers arrange\nnothing\", matching the register the README already uses for the same sentence.",
          "timestamp": "2026-08-11T14:03:16+02:00",
          "tree_id": "a92d6b98c1444ecb589f8881c92c0ebb2a425442",
          "url": "https://github.com/utxo-z/utxo-z/commit/58fa08d2ba41ae16692787d9fd4fefa12d86aa9d"
        },
        "date": 1786450059438,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 12579844.46,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 12525715.04,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 8066326.45,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1474693.55,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 566.21,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 770.71,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 14388115.47,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 33026695.72,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 14104173.47,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 14348.06,
            "unit": "ops/sec"
          },
          {
            "name": "erase hit",
            "value": 14796807.76,
            "unit": "ops/sec"
          },
          {
            "name": "erase miss",
            "value": 12169397.54,
            "unit": "ops/sec"
          },
          {
            "name": "erase + process_pending_deletions (100 entries)",
            "value": 113501.42,
            "unit": "ops/sec"
          },
          {
            "name": "batch erase 1K",
            "value": 15182.93,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2013.06,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 5816.88,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2621.59,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 4.86,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 4.9,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 4.83,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 4.77,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 4.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 4.75,
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
          "id": "ebe9abe083a4053e439da2f19df58d71ba3bca84",
          "message": "feat!: the caller owns its deletions, and gets back what was applied (#124)\n\n* feat!: the caller owns its deletions, and gets back what was applied\n\nBREAKING CHANGE: erase(), process_pending_deletions() and\ndeferred_deletions_size() are replaced by apply_deletes(span).\n\nThe deletion queue had the ownership defect #116 fixed for lookups: erase()\nwrote into a database-wide set and process_pending_deletions() emptied all of\nit, reporting everything to whoever called. It had not bitten yet only because\nof who the callers happen to be — connect and reorg are mutually excluded — and\nthe first writer outside that exclusion would have acquired it silently.\n\nReading the old drain settled a question KTH could not answer from its own side,\nand both of its beliefs about it were wrong. There was no failure path at all:\nprocess_deferred_deletions_in_file() caught every exception, logged it and\ncarried on, so an unreadable version was skipped rather than reported. The queue\nwas then cleared unconditionally, so keys that were never applied came back in\n`failed` beside keys genuinely not stored — the #110 ambiguity, still live in\nthe write path — and a caller re-querying the queue size after a failure always\nread zero.\n\nDeletions cannot borrow the lookup fix. A resolution is a read and can discard\npartial work; a deletion has already written to a mapped file by the time it\nmeets one it cannot open. So the result enumerates instead of pretending:\n\n  erased      applied during this call, including on the failure path\n  absent      not stored, established only after full coverage\n  unresolved  still owed; the only category that may be resent\n\nEvery distinct key of the span lands in exactly one of the three — on every\npath, including the ones that apply nothing. A closed or recovery-latched\ninstance returns the same deduplicated batch in `unresolved` rather than a\nverbatim copy of the span, which is what made a refusal the one place the\npartition did not hold: three requests naming one outpoint came back as three\nkeys still owed.\n\nThe walk consolidates each deletion as it happens. The map is changed, the\ndeletion is recorded — into a vector reserved before any mutation, so it cannot\nallocate there — and only then does the bookkeeping that can throw run. What is\nstill owed is finished by a scope guard that runs on the exception path too, so\nthere is no window where the map has changed and the working set still counts\nthe key as pending. Without that, an exception after an erase left the key in\nboth `erased` and `unresolved`, and a caller resending its unresolved would be\ntold the key is absent — fatal on the connect path, one retry after a deletion\nthat had actually succeeded.\n\nfailpoints::fail_delete_after_applied is the seam that reaches it: it throws\nbetween the map changing and the bookkeeping, after a chosen number of deletions\nhave been applied within the call. fail_lookup_open_version cannot, because it\nfires before a file is touched.\n\nStatistics count what happened: applied deletions always, processing_runs only\non a completed batch. Lookup resolution writes resolution_stats and never\ndeferred_stats.\n\nThe public surface is one mutating call per batch. There was no production\ncaller of the single-key erase(), so the immediate path became phase 0 of\napply_deletes(). Internally deferred_deletions_, add_to_deferred_deletions(),\nerase(), reference_erase(), erase_from_cached_files_only() and\nprocess_deferred_deletions_in_file() are gone.\n\nThe threading contract keeps what #123 established — resolve() serialised\ninternally, find() alongside it — and says where apply_deletes() sits: it\nmutates, the resolve lock does not cover it, and it needs exclusion from\nresolve(), find(), insert(), compaction and close() alike.\n\ntests/test_deletion_ownership.cpp pins the contract in both storage modes,\nincluding the two states that apply nothing and the exception raised after real\nprogress. The 61 call sites across tests, examples and benchmarks were migrated\nby intent: a test that used to compare erase()'s return now names the category\nit means.\n\n.gitignore grows the three test-output patterns the suite can leave behind when\na case fails before its cleanup — which is when somebody is most likely to run\n`git add -A` while debugging.\n\n* fix: charge a historical reference deletion to the reference catalogue\n\nThe reference walk reached the file through the same erase_in_file<Index> the\nfull containers use, via a switch whose `default` mapped reference_sentinel_index\nonto Index 0. The bookkeeping inside was guarded by\n`if constexpr (Index == SIZE_MAX)` — a condition that dispatch can never satisfy.\nSo a historical reference deletion erased the right entry, marked the right file\ndirty (note_dirty takes the runtime index), and then called\nupdate_metadata_on_delete(0, version): the metadata of full container 0. The\nreference catalogue never heard about the deletion, and container 0's was told an\nentry left a file it was never in.\n\nThe two walks are now separate functions. The reference one opens the reference\nfile and updates reference_catalog_; the full one is templated on a real\ncontainer index and updates catalogs_[Index]. Neither can be instantiated for a\nsentinel that is not a container index, so the shape that made this possible is\ngone rather than corrected. The per-file stepping they share takes the\nbookkeeping as a parameter: which catalogue a deletion belongs to is a property\nof the file being walked, and deciding it next to the code that opened the file\nis what keeps the two from drifting again.\n\n`default:` no longer walks an unknown index as container 0. Doing that silently\nis how the sentinel came to update the wrong catalogue, so an index nobody\nrecognises stops the batch instead.\n\nThe dirty identity and the erased/absent/unresolved partition are unchanged;\nthis only moves where the metadata decrement lands.\n\nCharged rather than observed, because there is nothing to observe. entry_count\nhas no runtime consumer today — it is serialised and nothing reads it back\nduring a run — and a historical version's metadata is not persisted after a\ndeletion, so the wrong catalogue produced identical answers, identical files and\nan identical database. The defect is real and silent, which is why the control\nreads a counter the two branches bump rather than a behaviour: the bookkeeping\nis wrong the moment anything starts trusting it, and nothing would have caught\nit before then. failpoints::reference_metadata_deletes and full_metadata_deletes\nfollow sync_file_calls, which exists for the same reason.\n\nTwo cases, one per mode, each asserting its own catalogue took every deletion\nand the other took none. Restoring the old dispatch makes the reference one\nreport zero reference updates and a full one instead.\n\n* fix: restore the statistics a historical deletion records, and four review findings\n\nThe rewrite of the per-file walk dropped every counter the queue-draining path\nused to keep: deletions_by_depth, current_size, total_deletes and the\nheight-range histogram. The active-version phase kept recording its own, so the\ntwo halves of one call disagreed — and by container. A deletion that reached an\nolder file was invisible to every per-container number while an identical\ndeletion in the active version was not.\n\nNothing caught it. Every batch-level assertion still passed, because `erased`\nwas right; it was only what an operator reads that was wrong. So the fix comes\nwith a case that compares the totals across containers, and the depth histogram,\nagainst the number of deletions actually applied — a control the old shape would\nhave failed, and the one that was missing when this was introduced.\n\ndeferred_deletes is not among the restored counters and is removed instead. It\ncounted how much was sitting in the queue, and there is no queue to sit in;\nnothing had written it since the queue went, exactly as deferred_lookups stopped\nbeing written in #118.\n\nFour more from the same review:\n\nbench_large_ibd cleared its batch after each call, discarding whatever the call\ncould not finish. It now carries `unresolved` forward, which is the one category\nthat has to be sent again — a benchmark that silently drops work measures the\nwrong thing and demonstrates the wrong usage.\n\nThe README still had a section heading and prose describing a deletion queue,\nincluding \"queued as a pending deletion. Deletions still use an internal queue\",\nwhich this change makes false. The section is now \"Batched reads and deletes\"\nand says that nothing is queued inside the database.\n\nThe sentinel comment on fail_delete_after_applied justified itself with \"zero is\na real count\", but the check is `== ++applied_in_call`, so zero can never match\nand was never a reachable setting. The comment said why a distinct sentinel was\nnecessary; it is not, and it now says so.\n\ntest_compaction_invariant compared erased.size() against batch.size() for a\nbatch built by concatenating two key lists. apply_deletes() partitions distinct\nkeys, so that asserted something the contract does not promise and would have\nbroken the day those lists overlapped. It counts the distinct keys now.\n\n* docs: finish removing the queue from the vocabulary, and the fields nobody writes\n\nThe earlier pass changed the section that described the deletion queue and left\nthe rest of the prose describing an API that no longer exists. CodeRabbit's\nreview listed the sites; this is the sweep rather than another partial one.\n\nerase() is gone from README and from database.hpp: it was still named as\nsomething that \"works on that mapped version\", still credited with writing the\nstatistics counters, and still listed among db_base's methods. The sentence that\nexplained the pair now explains the pair that exists — find() answers from the\nactive versions and hands back what it could not, apply_deletes() starts there\nand then walks the older versions itself.\n\nThe dead fields go with the words. Nothing has written container_stats\n::deferred_deletes, deferred_stats::total_deferred or max_queue_size since the\nqueue was removed — they counted queue occupancy — so they are removed rather\nthan left reporting zero forever, exactly as deferred_lookups was in #118. The\none assertion that read total_deferred goes with it. What survives in\ndeferred_stats is what the deletion path still writes.\n\nbench_mixed_workload cleared its batch after each call, like bench_large_ibd\ndid: both discarded whatever the call could not finish. Both carry `unresolved`\nforward now. The blockchain_processing example's final path did the same and now\nmatches its own periodic path.\n\nThe failpoints are renamed fail_historical_open_version and\nfail_historical_catalog. They were named for lookups and the deletion path uses\nthem too, so the names said something false about who they belong to; the\nbehaviour and the initialisers are unchanged.\n\napplied_in_call becomes applied_in_walk, because that is what it counts — the\nactive-version phase has no throw point, so including it would shift the\nnumbering by however many keys happened not to have rotated away, which is not\nsomething a case can know. The failpoint's documentation says so now instead of\nimplying the whole call.\n\ntest_sync required only that nothing was left owed, which a batch that found\nevery key absent would also satisfy. It requires a non-empty `erased` too, so\nthe sync failpoint below it operates on deletions that really happened.\n\nAlso: the empty \"Deferred lookup helpers\" heading in database_impl.hpp, and the\n\"Erase helpers\" one above a function only apply_deletes() calls.",
          "timestamp": "2026-08-11T16:16:56+02:00",
          "tree_id": "d729d34ba6236ebe26bc48f973a3dc3ad5c9672f",
          "url": "https://github.com/utxo-z/utxo-z/commit/ebe9abe083a4053e439da2f19df58d71ba3bca84"
        },
        "date": 1786458050569,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 244227.18,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 330193.18,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 264448.5,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 277417.33,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 409.84,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 421.64,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12032689.97,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 26317403.79,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11533145.51,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12182.81,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2244344.93,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2384743.18,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 117288.51,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 12398.81,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2321.49,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3280.15,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2731.2,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.88,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.36,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 56.94,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 57.14,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 57.3,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 56.91,
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
          "id": "ef055826b5fc789d36ef2030488112c8a77f0ee5",
          "message": "release: 0.10.0",
          "timestamp": "2026-08-11T16:17:51+02:00",
          "tree_id": "d729d34ba6236ebe26bc48f973a3dc3ad5c9672f",
          "url": "https://github.com/utxo-z/utxo-z/commit/ef055826b5fc789d36ef2030488112c8a77f0ee5"
        },
        "date": 1786458074949,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 250637.49,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 278776.17,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 265664.73,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 274977.95,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 368.33,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 430.98,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12407304.31,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 26311492.09,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11925264.84,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12180.93,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2047091.22,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2417943.09,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 113341.14,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 11329.97,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2313.25,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3131.61,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2751.45,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.07,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 55.83,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 55.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.15,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 55.77,
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
          "id": "846b9f18e21575e63f4a0316692a3cfad2ad72cd",
          "message": "release: 0.10.0 (#125)",
          "timestamp": "2026-08-11T16:47:10+02:00",
          "tree_id": "d729d34ba6236ebe26bc48f973a3dc3ad5c9672f",
          "url": "https://github.com/utxo-z/utxo-z/commit/846b9f18e21575e63f4a0316692a3cfad2ad72cd"
        },
        "date": 1786459859547,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 772743.73,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 1092779.66,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 1108919.44,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1467721.44,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 340.16,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 412.57,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12099009.73,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 28168664.8,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11386191.11,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12309.66,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2283733.38,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2515399.63,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 118965.17,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 13215.9,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1817.05,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3086.49,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2637.14,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 107.96,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 107.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 106.69,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 107.72,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 105.97,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 107.38,
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
          "id": "4fe1537471245f50e950250f72863533e861ac1b",
          "message": "docs: update release notes for v0.10.0",
          "timestamp": "2026-08-11T16:47:15+02:00",
          "tree_id": "a3a422ca6cf8576925f2fb3edeb8c737263185c5",
          "url": "https://github.com/utxo-z/utxo-z/commit/4fe1537471245f50e950250f72863533e861ac1b"
        },
        "date": 1786459869839,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 824866.55,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 909992.04,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 1004257.72,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1304919.31,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 329.46,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 379.01,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11456256.65,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 29169400.61,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11123652.31,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 11176.43,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2417031.81,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2605563.19,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 133407.32,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 13317.99,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2187.5,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3183.15,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2741.32,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 111.73,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 110.55,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 111.48,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 112,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 109.67,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 109.98,
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
          "id": "4fe1537471245f50e950250f72863533e861ac1b",
          "message": "docs: update release notes for v0.10.0",
          "timestamp": "2026-08-11T16:47:15+02:00",
          "tree_id": "a3a422ca6cf8576925f2fb3edeb8c737263185c5",
          "url": "https://github.com/utxo-z/utxo-z/commit/4fe1537471245f50e950250f72863533e861ac1b"
        },
        "date": 1786461738701,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 604093.19,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 952877.36,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 866390.71,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1024877.19,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 303.86,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 372.64,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12116057.57,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 28382331.4,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11266529.19,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12107.38,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2228305.14,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2499271.71,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 102727.6,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 10860.31,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1717.48,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 2754.78,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2658.01,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 105.46,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 105.11,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 105.11,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 106.15,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 106.31,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 106.35,
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
          "id": "36528b9b75e6b2e6f5f733ba8fc82856b4fa9196",
          "message": "fix: refuse a version file whose named object cannot be reached (#127)\n\nBoost.Interprocess resolves a named object by name alone. `priv_generic_find<T>`\nlooks the name up in the index, checks nothing about the type beyond a\n`BOOST_ASSERT` that vanishes in release builds, and returns `static_cast<T*>` of\nthe stored address. Eleven call sites read that pointer; every one of them\ntreated a null as an empty file and carried on.\n\nThat cost three different things:\n\n- an enumeration returned a subset and reported success;\n- open() published an entry count short by whatever the unreadable file held —\n  and that count is a running total, so insert() and apply_deletes() carried the\n  error for the life of the instance;\n- a compaction dropped a source's entries from the merge and then unlinked the\n  source, because sources are retired unconditionally once the target is\n  published. That one loses data permanently, and it is why this is a fix.\n\n`find_single_named()` is now the one way to reach an object inside a segment\nthat already exists, next to `open_existing_segment()`, which is already the one\nway to open one. It refuses an absent object, and it also checks the instance\ncount `find<T>()` returns and that nothing was reading: `value_bytes /\nsizeof(T)`, which is 1 unless `sizeof(T)` has changed since the file was\nwritten. `read_target_marker()` had written that pair of checks out by hand; it\nnow shares them.\n\nThe count is a cardinality check, not a layout check, and the header says so.\nInteger division leaves it reading 1 for any `sizeof(T)` in\n`(value_bytes / 2, value_bytes]` — with today's `sizeof(utxo_map<48>) == 56`,\nanything from 29 to 56 passes. A reordering that preserves the total passes too,\nand a change to the hash or its mixing relocates every key while leaving every\nsize alone. The barrier for those is a format epoch validated before any segment\nis touched, plus fixtures written by earlier builds; this closes only what was\nalready free.\n\nThe two counting loops in configure_internal() now propagate rather than skip.\nThe catalogue is built from the directory, so a version it lists is one this\nbuild knows is there: being unable to read it is not a smaller database, it is\none this instance cannot describe. This changes what open() does with a damaged\ndatabase — it refuses where it used to succeed — which is the fail-closed\nreading and the vocabulary the store already has, since recovery_failed does not\nopen either.\n\nBoth functions in segment_open.hpp report failure as a value. Mapping a file is\nthe one operation in this path that raises, so it is caught there, once, and the\nten call sites in database_impl.cpp read as `result<>` like the rest of the\nstore; their `try` blocks shrink to the caller-supplied callback, the only thing\nleft in them that can throw. file_cache keeps its documented throwing contract\nand adapts at its own boundary: resolve() and apply_deletes() depend on\nunwinding, including a scope_exit guard that keeps the working set consistent,\nand converting them belongs with the format work rather than here.\n\nAn open failure reports `file_open_failed`; a version file that opens but holds\nno usable object reports `version_unreadable`. Keeping them apart matters — the\ntwo faults send an operator looking in different places.\n\nsegment_open.hpp now static_asserts on\nBOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_INITIALIZE_TIMEOUT_SEC. The ten-second\ncap is a compile definition on the library target and is baked into an inline\nfunction in that header, so a target including it without the definition gives\nthe program a second, slower definition of the same function; the linker keeps\none, and whichever it keeps decides how long everybody waits. The test target\nnow carries the definition and the assertion makes the mistake a compile error\nrather than a five-minute wait on one platform.\n\nEvery case is checked by mutation. Restoring `if (!source_map) continue;` in the\nmerge makes compact_all() return success over a blanked source — the data-loss\npath, reproduced. Restoring the two `continue`s in configure makes the three\nrefusal-to-open cases go red. Restoring the skip in the traversals makes them\nreport success over a partial read. Dropping the instance-count check leaves the\nthree-instance case passing.\n\nFour existing recovery cases pinned open-succeeds-then-fails-late, though not on\npurpose: they damage a historical version, open, and measure that the refusal on\nthe path reaching it is prompt rather than a five-minute wait. The property is\nthe promptness. They now damage the file after opening, which keeps them\nexercising the file cache and resolve() rather than collapsing into \"the open\nrefused\".\n\nNot in scope: the two `find_or_construct` calls that open the active containers.\nSeparating \"open existing\" from \"create new\" belongs with the format barrier,\nwhere the config has to be validated before any segment is touched.",
          "timestamp": "2026-08-13T11:48:23+02:00",
          "tree_id": "e5b6beefda932a18a00ff4fd5943eb4113277b03",
          "url": "https://github.com/utxo-z/utxo-z/commit/36528b9b75e6b2e6f5f733ba8fc82856b4fa9196"
        },
        "date": 1786614786744,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 286788.6,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 403645.37,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 279561.19,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 257605.6,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 414.02,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 513.52,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 13560037.95,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 26991435.95,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 13024023.69,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13030.94,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2222942.45,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2233487.65,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 87316.98,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 11021,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2566.25,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3469.41,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2945.19,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 54.37,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 54.08,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.93,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 54.03,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 54.81,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 54.09,
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
          "id": "196d39ca47c72f043acb08c78e8df141fb311add",
          "message": "Merge pull request #128 from utxo-z/harden/persisted-format-barrier\n\nfeat!: a database records what it was written under, and is refused if it does not match",
          "timestamp": "2026-08-13T15:01:50+02:00",
          "tree_id": "b38ddbd5ce11b0f38ae1ecce850cc32517e6bf56",
          "url": "https://github.com/utxo-z/utxo-z/commit/196d39ca47c72f043acb08c78e8df141fb311add"
        },
        "date": 1786626369269,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 1068596.15,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 1308637.46,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 1341936.67,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1663735.32,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 413.75,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 374.62,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 11420613.61,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 26653767.52,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11444166.27,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12239.2,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2347508.82,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2432456.26,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 135363.37,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 14317.69,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2071.07,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3378.53,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2819.99,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 106.07,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 105.73,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 105.36,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 104.63,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 105.09,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 105.19,
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
          "id": "cc1278f5c0c7dc3263128a2c9ce0126ecd36b2c7",
          "message": "test: certify that this build still reads what an earlier one wrote (#130)\n\nThe format barrier refuses a database whose identities do not match. It cannot\ntell whether the identities are still *true* — whether a build that declares\nepoch 1 reads epoch 1 the way the build that wrote it did. That needs a database\nsomebody wrote earlier, kept, and opened again.\n\n`tests/fixtures/epoch1-lp64/` is that: two databases written under epoch 1, on a\n64-bit little-endian target with Boost 1.91, one per storage mode, each with a\nhistorical generation as well as an active one. They are artefacts, not outputs.\nNothing regenerates them, and a change to their bytes is a change somebody has to\nread and justify.\n\n## What is checked, and what a green run does not say\n\nPhysical first — it opens, so the config and every stamp were accepted before\nanything reached a map — and then logical: every entry, canonically ordered,\ndigested. Neither half alone is compatibility. `open()` succeeding says nothing\nabout the values, and a matching entry count stays green while every one of them\nis wrong.\n\nA fixture written under epoch 1 proves the *reader* still reads epoch 1. It does\nnot prove the *writer* still produces the same bytes for every state a database\ncan reach, because a fixture holds only the states it holds. That is why the\nmanifest carries a digest per segment as well as a global one, why the generator\nis a separate tool whose candidates a person compares, and why the Boost probe\nreports what it found rather than a colour. The asymmetry is written down in\n`doc/format-compatibility.md` and in the test's own header, not left implied.\n\n## Two digests, because they answer different questions\n\nThe global digest covers key, creation height and payload, sorted by key so the\nmap's internal order cannot reach it. It deliberately omits which container or\ngeneration an entry sits in: compaction moves entries between files without\nchanging what the database holds, and a digest that noticed would report a change\nwhere there was none.\n\nThe per-segment digest is the opposite and is what certifies a fixture — it names\nthe container and the generation and covers only that file. A generation that went\nmissing cannot hide inside a global total.\n\nBoth are SHA-256 via `boost::hash2`, which Boost 1.91 already provides. FNV-1a\nstays where it was, guarding torn records; it is not a golden.\n\n`logical_digest.hpp` lives under `tests/support/` rather than `src/`. Nothing the\nlibrary does at run time needs it, and a cryptographic digest has no business on\nthe production path because the fixtures wanted one.\n\n## The seam the fixtures are built with\n\nA natural rotation needs about a hundred thousand entries, which would make the\nfixtures megabytes of poorly compressible data. `failpoints::force_rotations` lets\nthe generator ask for one, and it answers the ordinary safety check so the real\nrotation path runs — `new_version()` and everything under it. A second generation\nassembled by hand would attest to our ability to produce plausible bytes rather\nthan to what the writer writes.\n\nIt is off by default, internal, and has its own case: that it rotates exactly as\nmany times as asked, that nothing rotates when it is not set, and that the entries\nsurvive. A fixture resting on a seam nobody tested would certify less than it\nclaims.\n\n## Sizes\n\nThe version files are 10 MiB each and almost entirely holes. The whole fixture\nset — eight version files across two databases — packs to 184 KB in git. Git LFS\nis not needed and is not introduced. A fresh checkout materialises 81 MB apparent\nand about 13 MB real.\n\n## Boost 1.92\n\nThe informative probe is a job of its own, never required, pinned by version and\nchecksum — a floating reference would leave a run that cannot say which bytes it\ntested. It builds the store against a Boost the project does not pin and reports\none of five verdicts. `BUILDS_ONLY` exists so that compiling is never reported as\ncompatibility.\n\nRun against Boost 1.92.0 it returns `LOGICALLY_COMPATIBLE`: the epoch-1 fixtures\nopen and all 420 entries come back, digest for digest. `unordered`'s core changed\nbetween the two releases; `mulx.hpp` and every interprocess header did not. That\nis evidence for leaving `map_layout_epoch` at 1, and no epoch is moved here.\n\n## wasm32\n\nThere are no fixtures for it, so there is no evidence of a physical round trip\nthere, and the suite says so rather than passing quietly: on an ABI the fixtures\nwere not written on, the case checks that they are *refused* with `abi_mismatch`.\nA fixture declares the ABI it was written under, so that refusal is a correct\nresult rather than a gap — unlike the hash vectors, which certify an algorithm per\nABI and must fail where none is pinned.\n\n## Verified by mutation\n\nFive, each confirming the suite notices:\n\n| mutation | result |\n|---|---|\n| one byte flipped inside a data file | 1 case red |\n| a version file removed | 4 cases red |\n| the manifest's expected digest altered | 2 cases red |\n| a reader declaring `map_layout_epoch = 2` | 4 cases red, refused by the barrier |\n| the rotation seam neutered | the seam's own case red |\n\nSuites: 206 cases with statistics, 201 without.\n\n## Keeping eighty megabytes out of the package\n\nThe CI's line-ending check went red on the fixtures, which was a symptom of\nsomething larger it was pointing at.\n\n`exports_sources` includes `tests/*`, so every recipe revision would have carried\nthe fixtures — eighty megabytes of evidence about what earlier builds wrote, which\nthis repository's CI needs and a consumer building from source does not. They are\nexcluded now, and the exported sources stay at about a megabyte.\n\nWhich leaves the question of what happens where they are absent. The cases are not\nbuilt at all, and CMake says so: a suite that silently skipped them would report\nthe same green as one that checked them. Verified by moving the directory aside —\nthe build succeeds, the message appears, and `[compat]` matches nothing.\n\nThe fixtures are also named binary in `.gitattributes`. `text=auto` does detect\nthem correctly today, and the committed bytes match what the generator wrote, but\nthe detection is a heuristic over the first few kilobytes and these files exist to\nbe byte-for-byte what an earlier build wrote. A normalisation applied to one would\nnot corrupt it visibly — it would change a digest the suite then reports as a\nformat change.\n\nAnd the line-ending check skips files git exports verbatim: a file marked binary\nhas no line endings to normalise, so asking finds carriage returns inside binary\ndata and reports them as a portability fault.\n\n## Second round\n\nThe manifest was prose. Only `platform_abi_id` was read from it, so its header\ncould say `map_layout_epoch: 27` and every case stayed green — verified before\nfixing. It is now held against the bytes it claims to describe: every fixture's\nbinary config must carry the identities the header declares, the ABI id must\ndecompose into the endianness and the three widths beside it, every declared\nsegment must exist with a stamp that agrees, and nothing on disk may be\nundeclared. A case also rewrites a config's `boost_version` to a release that has\nnever existed and requires the database to open anyway, which is what says the\nfield is recorded and not enforced.\n\nThe substring reader is gone. It accepted a truncated file, a duplicated key and a\nnumber where a string belongs, which stopped being tolerable once the manifest\nbecame part of the certificate. Boost.JSON is already available header-only and\nrejects all three.\n\nReference mode had a global digest and nothing else. It now has the same\nper-segment verification as full mode — each generation opened, stamped, read as\n`reference_map_t`, and checked against its declared count and digest — and its own\nwrite-and-reopen, which checks height, file number and offset separately, because\na reader returning the right height at the wrong position would be no use.\n\n## The probe was overdeclaring\n\nIt opened one fixture and compared one digest while the verdict said \"the\nfixtures\" and \"every entry came back\". It now builds and runs the compatibility\nsuite itself against the other Boost, so the verdict covers what the suite covers\n— both modes, per-segment digests, write and reopen — rather than whatever a\nsecond implementation remembered to check.\n\nThe writer side is measured too, against a baseline written by the *pinned* Boost\nrather than against the committed fixtures. That is the only comparison that\nisolates the variable, for a reason worth writing down.\n\n## The files are not byte-reproducible, and now we know why\n\nTwo runs of one binary, same inputs, same forced identity, do not agree:\n\n- A rotated-out generation holds **raw process addresses** in its segment header —\n  eleven bytes near offset 513 reading `0x00007ff…`. Disabling ASLR makes the two\n  runs identical, which is what identified them.\n- The unused tail of every stored value is **uninitialised memory**. `utxo_value`\n  is default-initialised and `set_data()` fills only `actual_size` bytes, so up to\n  6 KiB per entry in the largest container is whatever the writer's stack held,\n  copied into the file. Nothing reads it back, which is why nothing has noticed.\n\nNeither changes what a database holds and neither is a compatibility problem, but\nthey mean physical equality can never be the criterion. Both are documented.\n\nWith the baseline comparison, Boost 1.92.0 writes 14 of 18 files byte-identically.\nAll four differences are accounted for: two configs record which Boost wrote them,\nand the other two are the files above.\n\n## And the gate that could hide all of it\n\nThe compatibility cases are only compiled when the fixtures are present, so a\nmistake in that condition would remove them from the suite and leave CI green\nwithout them. The Linux job now asks how many were discovered and fails if there\nare too few.\n\nSuites: 210 cases with statistics, 205 without.\n\n## One platform disagreed about a library nobody links\n\nParsing the manifest with Boost.JSON pulls in Boost.Container's pmr headers, and\nthose ask MSVC for `libboost_container` through a `#pragma comment(lib, ...)`\nthat fires on inclusion, whether or not a symbol from it is ever wanted. This\nproject links `Boost::headers` and no Boost binary — the Linux link line carries\n`libutxoz.a` and Catch2 and nothing else — so the request arrived with no path to\nsatisfy it and the Windows link failed over a dependency that is not used.\n`BOOST_ALL_NO_LIB` on our own targets, private, because which Boost binaries a\nconsumer's linker goes looking for is theirs to decide.\n\n## Asking the right question about coverage\n\nThe gate that checks the compatibility cases were built asked ctest for a name\npattern. Names are a heuristic: it matched two tests that merely contain the word\n\"generation\" and missed three of the eleven it exists to protect, so its count was\npartly about tests that are not these. It now asks the suite for the tag.\n\nThe hazard it guards is real and measured: with the fixtures removed the suite\ndrops from 210 cases to 199 and stays green.\n\nThe probe had the same shape of gap at the other end. Without fixtures its cases\nfail on the missing files, and that would have been reported as `LOGICAL_MISMATCH`\n— the format having changed — which is the one thing this job must not say when it\nhas not looked. `NO_EVIDENCE` says what happened instead. It also covers a filter\nthat stopped selecting the reference cases: the run is asked how many cases it\nexecuted, because a verdict claiming both storage modes should not rest on the\nassumption that both were reached.\n\n## The fixtures contained process memory\n\nA `utxo_value` is stored inline in the mapping and stored whole — the container's\nsize class, not the size of the output in it. `set_data()` wrote `actual_size`\nbytes and left the rest as it found it, so everything past the payload was\nwhatever that storage last held. For a value built on the stack, the caller's\nstack, copied into a file. Up to 6 KiB per entry in the largest container.\n`get_data()` returns `actual_size` bytes, so nothing ever read it back, and\nnothing failed.\n\nThere is a second region no member names. `sizeof` rounds a size class up to the\nalignment, so a 94 container occupies 96, and those two bytes are copied and\nwritten like the rest.\n\n`set_data()` now defines both, and the two construction sites value-initialise.\nNo layout moves and no key moves, so no epoch does either; a database written by\nan earlier build still opens and still reads back exactly what it held. It keeps\nthe residue it already has, which is #131's problem and not something a format\nchange could fix.\n\nThe fixtures are regenerated by the corrected writer. The old ones are not in\nthis branch's history — one commit, amended — so they never reach master.\n\n### What the residue was hiding\n\nThis also corrects something reported earlier in this PR. The eleven differing\nbytes near offset 513 of a rotated generation were read as absolute process\naddresses in a segment header, which would have contradicted `offset_ptr` and\nneeded explaining. They are the same defect: 171990 bytes differed between two\nruns, in runs of six at a stride of eight — stack slots holding pointers whose\nupper bytes move with ASLR. There are no absolute pointers in the persisted image.\n\nWith the residue gone, two runs with the same identity produce all eighteen files\nbyte-identical, which is what makes physical equality usable as a criterion at\nall.\n\n## So the writer comparison can now say something\n\nIt could not before, and the previous round's version could not have said it\neither: it enumerated only the baseline, so a file the candidate wrote and the\nbaseline did not was never visited and every file that *was* visited matched. It\nnow walks both trees, and reports a file present on one side as a difference.\n\nThe config is normalised rather than skipped — four bytes at offset 44, the\n`boost_version` the reader never compares — so a difference anywhere else in it\nis still a difference. There is no other mask, because there is nothing left to\nmask.\n\nThe measurement checks itself first: the pinned build is compared against a second\nrun of itself, and if that disagrees the comparison reports `NO_EVIDENCE` rather\nthan attributing the noise to Boost. `--self-test` runs the comparison over six\ntrees whose answers are known, including the candidate-only file.\n\nAnd the classification is now separate from the logical verdict. \"It reads what we\nwrote\" and \"it writes what we write\" fail independently, and one word covering\nboth always overstates one of them.\n\nThe lane never built the generator, so the comparison it claimed to make had\nsilently never run. It builds it now, and its absence produces `NO_EVIDENCE`\nrather than a footnote under a compatible verdict.\n\n## The manifest is checked against a schema, not just parsed\n\nParsing was the smaller half. A well-formed document can still carry a key\nrepeated within one object — Boost.JSON accepts that and keeps the last, so a\nsecond `map_layout_epoch` further down would have won every check — or a negative\ncount arriving as a very large positive, or two fixtures under one name, or a\ndigest that is not a digest.\n\nThe enumerations are read by equality. `storage_mode` was `== \"full\" ? full :\nreference`, which is not a decision but a default: any other string at all, a typo\nincluded, selected reference mode and the suite went on to verify the wrong half\nof the format. Same for `endian`.\n\nValidation returns the first problem instead of asserting, so ten deliberately\nbroken documents can be pointed at it and required to be rejected. A validator\nnothing is ever seen to reject anything with is indistinguishable from one that\naccepts everything.\n\n## Two seams and a certificate\n\n`clear()` did not reach `force_rotations`, `force_database_id` or\n`forced_database_id`. A process-wide seam left armed does not fail where it was\nset; it follows the next test in. They are reset now, and arming is guarded by a\nscope object rather than a call at the end of a block — a failing REQUIRE leaves\nby throwing, and skips the call.\n\nThe generator announced \"wrote a manifest\" without ever asking whether the write\nsucceeded. It writes beside the final name and renames into place, checks the\nstream after closing it, and says nothing about success until both have held —\nwhich also keeps a failure from leaving a truncated manifest that the next run\nwould refuse to replace.\n\nSuites: 221 cases with statistics, 216 without.\n\n## The comparison found its own mistake first\n\nIts first real run reported two differing files, both `utxoz_config.dat`, \"in more\nthan the Boost it records\". They were clean: the config carries a checksum at\noffset 48 covering everything before it, so a config recording a different Boost\ndiffers twice — in `boost_version` and in the checksum of `boost_version` — and\nnormalising only the field left the second looking like a finding.\n\nBoth are normalised now. Nothing is lost by dropping the checksum: it is a\nfunction of bytes 0..47 and those are compared directly, so any difference it\ncould reveal is already visible in the field that caused it. The self-test case\nnow differs in both, which is what would have caught this.\n\nWorth recording that the classification refused to call it identical rather than\nabsorbing it into an expected-differences list. That is the whole point of not\nhaving one.\n\n## The fill crossed out of the array it started in\n\n`data.data() + actual_size` to `this + sizeof(*this)` walks past `data`'s\none-past-the-end, and the padding sitting immediately after it does not make that\npointer legal. The two regions are cleared separately now: the tail as part of the\narray it belongs to, and the padding through the object representation, which is\nwhat a standard-layout object may be viewed as. `offsetof` gives the boundary, and\ntwo `static_assert`s state the properties that access relies on.\n\nWorth recording what this cost to establish: at `-O3` GCC's vectorised fill\nalready clobbers the padding to zero, so removing the memset leaves every case\ngreen. At `-O0` the poison survives. The check is therefore right and required —\nno compiler owes us that — but on a release build it cannot say who wrote the\nzero. The sanitiser job builds Debug, which is where it can.\n\nThe test also read its result back out of the `std::array` it had constructed the\nvalue into. That array's lifetime had ended, and the compiler answered from what\nit last knew: with the padding fill deliberately removed, the padding still came\nback zero. It reads through the object's representation now.\n\n## Two platforms were not slow, they were stuck\n\nmacOS and Windows sat in \"Run tests\" for hours while every other job finished. The\nsuite takes 84 seconds here, so this is a block, not a load — and nothing in CI\nwas arranged to say so: no job carried a `timeout-minutes`, no test carried a\n`TIMEOUT`, and a stuck run reports six hours of silence naming nothing.\n\nThat is fixed first, because it is what turns the next run into evidence: every\njob now has a budget, and `catch_discover_tests` sets a per-test timeout of 300\nseconds — thirty times the slowest legitimate case, which is the one that waits\nout the mapped-file initialisation timeout. A test that blocks now fails by name.\n\nTwo causes were removed on the way, both plausible and both worth removing\nregardless:\n\n- Whole files were read a character at a time through `istreambuf_iterator`. The\n  suite reads eighteen 10 MiB fixtures several times over, and implementations\n  differ on that path by more than an order of magnitude — it is how a suite that\n  finishes in a second on one platform stops finishing on another. Block reads\n  now.\n- The generator proved its destination writable at the end, after building 180 MB\n  of databases. So the test for an unwritable manifest had to do all of that work\n  to reach the failure it was checking, twice per run. It probes up front, which\n  is better behaviour for the tool as well: a destination that cannot take the\n  manifest is worth discovering before several minutes of work rather than after.\n\nFixtures regenerated with the corrected writer. Two runs at a fixed identity:\neighteen files, eighteen identical.\n\nSuites: 221 cases with statistics, 216 without, 84 and 85 seconds.\n\n## The review comments\n\nFour of the older ones were already addressed and were verified as such rather\nthan assumed: the lane builds the generator, the comparison walks both trees,\n`clear()` reaches the new seams, and the manifest is checked before it is\nannounced. Two nitpicks were likewise already done — the object-representation\nclearing and the `scoped_reset` guard in the value test.\n\nThe rest were real, and several were the same kind of defect this branch keeps\nfinding: a check that cannot fail.\n\n`find | head -1` under `set -o pipefail` is one. head closes the pipe, find dies\nof SIGPIPE, the pipeline fails, and the assignment takes the script with it —\nintermittently, depending on how fast find is. `-print -quit` instead, in both\nplaces and in the workflow.\n\nThe compatibility-case count was read out of Catch2's summary wording. Asking for\none line per case with `--verbosity quiet` does not depend on how a version of\nCatch2 phrases itself, and neither should a gate whose whole job is to notice\nabsence. The probe's own count now accepts both wordings Catch2 has used, because\nfailing to recognise a summary would report NO_EVIDENCE for a run that went\nperfectly well.\n\nThe digests sorted by key alone. Keys are unique — but that is a property of the\nstore, not of the function, and `std::ranges::sort` is not stable, so two entries\nsharing a key would have ordered arbitrarily and the digest would have differed\nbetween runs for no reason a reader could see. A certificate has to be defined\nfor every input it can be handed. Total order now: key, height, payload.\n\nThe fixture gate is evaluated at configure time, so adding or removing fixtures\nleft an already-configured build deciding on what was true when it was first\nconfigured. It is a CMAKE_CONFIGURE_DEPENDS now.\n\nAlso: the forced-rotation seam is consumed in one place rather than two; the\ngenerator test's output directory cleans up through RAII rather than a line at\nthe end of a case that a failing REQUIRE skips; the full-payload boundary is\nasked of the type instead of recomputed from its layout; a static_assert pins the\ncontainer count the per-container checks enumerate by hand; and the generator's\nincludes name what it uses.\n\n## What the timeouts bought\n\nThey turned six hours of silence into eight names, and the names say something\nthe silence did not.\n\nEvery compatibility case that opens a fixture timed out on macOS and on Windows.\nEvery one that does not, passed. That is eight of twelve on both platforms, and\nthe four survivors are exactly the four that only read bytes or build their own\ndatabase. Linux runs the same cases in a second.\n\nTwo candidate explanations were checked and are wrong. It is not the\ninitialisation timeout: the compiler lines show the ten-second definition\nreaching all three platforms, and Boost throws when it expires rather than\nwaiting. It is not a page-size difference in the persisted layout either —\n`ManagedOpenOrCreateUserOffset` rounds to `alignof(max_align_t)`, sixteen bytes\neverywhere here, not to a page.\n\nSo the cause is not established, and this does not guess at it. What it adds is\nthe one thing missing: a case killed by a timeout reports nothing, because Catch2\nbuffers until it has a result and a killed case never has one. The compatibility\ncases now say where they are on stderr under UTXOZ_TEST_TRACE, which CI sets, so\nthe next run names the call rather than the case.\n\nTwo failures there were diagnosable and are fixed. The Windows generator cases\nfailed rather than timed out: cmd.exe strips the outermost pair of quotes from\nwhat it is given, so a command beginning with a quoted path lost that quote and\nwas parsed as something else. The whole line is wrapped in one more pair on\nWindows, which cmd removes instead.\n\nAnd the sanitizer job's timeout was mine being wrong: the slowest legitimate case\ntakes eleven seconds optimised and over three hundred under address and\nundefined-behaviour instrumentation. One budget for both would either fail honest\nwork or stop catching anything, so it is now per configuration.\n\nSuites: 221 cases with statistics, 216 without, 85 seconds each.\n\n## What the tracing found\n\nTwo runs of instrumentation settled it. The copy finishes in sixty-two\nmilliseconds — so volume was never the problem — the segment maps cleanly, and\nthen the first named-object lookup never returns.\n\nA managed segment keeps its allocator's mutex **inside the mapped file**, and\nBoost compiles a different type for it per platform: POSIX process-shared mutexes\nwhere they work, the Windows API where that is the choice, and a spinlock\nemulation otherwise. macOS takes the third road because Boost marks Apple's\nprocess-shared mutexes as broken — `BOOST_INTERPROCESS_BUGGY_POSIX_PROCESS_SHARED`\nin `detail/workaround.hpp`, with the reason written out. Three types, three sizes,\nat one offset, and everything after them moves.\n\nLinux x86_64 and macOS arm64 agree on every number the identity held: little\nendian, and eight bytes for `size_t`, a pointer and an `offset_ptr`. So the\nbarrier accepted a file it had to refuse, and what followed was not a wrong answer\nbut no answer — a lock taken on another platform's bytes, no diagnostic, no end.\nEight cases held until a timeout killed them, on both platforms, and the four that\npassed are exactly the four that never open a fixture.\n\nTwo explanations were checked and discarded first, rather than assumed: the\ninitialisation timeout reaches all three platforms and Boost throws when it\nexpires, and `ManagedOpenOrCreateUserOffset` rounds to `alignof(max_align_t)` —\nsixteen bytes everywhere here — not to a page.\n\n## So the identity was under-specified\n\nIt is now two things: the **data** ABI, which is what it always was, and the kind\nof interprocess mutex the build compiles, derived from the same macros Boost\nswitches on and packed into the nibble the endianness byte leaves free. Linux\nreads 0x11080808, Windows 0x21080808, macOS 0x31080808.\n\n`lp64_le_abi` keeps naming the data ABI, and the hash vectors stay pinned against\nthat: what a key hashes to does not depend on which mutex sits beside it.\n\nThis is the fixtures doing the job they were built for. The suite was not asking\nwhether a Linux database opens on macOS — it was asking whether *this* build still\nreads what an earlier one wrote, and the answer arrived as a hang on two\nplatforms. Refusing is the correct behaviour and the barrier can now produce it.\n\nFixtures regenerated. Two runs at a fixed identity: eighteen files, eighteen\nidentical. Suites: 221 cases with statistics, 216 without.\n\n## The identity, taken from Boost rather than restated\n\nThe first version of this classified the mutex *family* by re-deriving the\npredicate Boost switches on. Two problems, both real.\n\nA copy of someone else's condition is right until it is not, and the failure is\nsilent: a Boost that changed the condition would leave this describing the wrong\nthing, which is exactly what the identity exists to prevent. It now reads the\nmacros `interprocess_mutex.hpp` leaves behind — `BOOST_INTERPROCESS_MUTEX_USE_-\nPOSIX` and `..._WINAPI` — and treats the remaining case as the emulation.\n\nAnd the family is not the layout. Two targets can both take the POSIX branch and\nstill disagree about what a `pthread_mutex_t` is; glibc and musl do. \"Same branch\"\nnever proved \"same persisted ABI\", so it is not what is compared.\n\nWhat is compared is measurable: the size and alignment of the mutex **and of the\nsegment manager**, which is the header this format is written around, together\nwith the platform and C runtime. Linux/glibc reports a forty-byte mutex; a\nspinlock build reports four. These do not fit a byte each, so the identity is a\nfold rather than a packing, with `platform_abi_encoding` in it so that changing\nthe ingredients is itself a change of identity.\n\nThe ingredient list exists once. `compute_platform_abi_id(family, tag)` is what\nthis build calls for its own constant and what a test calls to ask what another\ntarget's would be — the first attempt had the list written out twice, and a\nmutation that removed the family from one of them left every case green. Two\n`static_assert`s now hold the property directly: change the family or the platform\nand the identity must change. Removing the family from the fold is a failed build,\nnot a failed test.\n\nThe manifest records every ingredient beside the result, and the suite recomputes\nthe fold from what is declared. A header that said one thing while the id meant\nanother would not survive it.\n\nAnd a config whose data ABI matches but whose machinery does not is refused with\n`abi_mismatch` — proven as an ordering rather than an outcome, with container\nopens forced to fail: the refusal arrives before anything is mapped. Nothing here\nis timed, because a fast answer is not an early one.\n\nDocumented, because it is the part that is easy to get wrong later:\ncross-platform compatibility exists only where it has been certified. Landing on\nthe same branch as another build does not make a file readable by it.\n\nFixtures regenerated; two runs at a fixed identity give eighteen identical files.\nSuites: 222 cases with statistics, 217 without.\n\n## A job that reports nothing is worse than one that fails\n\nThe probe fetched Boost with an unguarded `curl` and an unguarded `git clone`\nunder `set -e`. A DNS failure, a proxy, a rate limit or a cancelled run would end\nthe script where it happened — no verdict, no summary, an informative job red and\nsilent, which is the single outcome it exists to avoid. `curl` also lacked `-f`,\nso an HTTP error page was saved as an archive and reported as a checksum mismatch:\na finding, for something that was not one.\n\nEvery fetch step is guarded now and a transport failure produces `NO_EVIDENCE`\nnaming what could not be reached, with `-f`, retries and a finite timeout. The\nchecksum keeps its own verdict, because bytes that do not match the pin are a\nfinding and not a network. Proven by pointing it at a version that does not exist.\n\n## And the generator's own reads\n\n`read_file` fed the SHA-256 that goes into the manifest, through\n`istreambuf_iterator`, checking nothing. A read that stopped early would not\nproduce a wrong file — it would produce a correct checksum of the wrong contents,\nand the fixtures would certify that. It reads one block against the size the\nfilesystem reports, and treats a short read, a bad stream and a file longer than\nit measured as errors.\n\n## Smaller, and all real\n\nA destructor called the throwing `remove_all`, so a cleanup failure during\nunwinding would have replaced a failing test with a terminate. The default-off\nhalf of the rotation-seam test lacked the guard its sibling has — and it is the\nhalf with the most to lose from a seam left armed elsewhere, since it is the one\nthat proves the default. A `CHECK` compared a storage mode through a ternary that\nmapped each value to itself.\n\nAnd the documentation said `BOOST_VERSION` is \"never compared\", which stopped\nbeing true when the writer comparison began normalising it. It is the barrier that\nnever consults it; the comparison reads it precisely because it is the one\ndifference expected between two builds that are otherwise the same.\n\nSuites: 222 cases with statistics, 217 without. Two runs at a fixed identity:\neighteen files, eighteen identical.\n\n## A fallback that made unknown platforms compatible with each other\n\n`platform_tag` ended in `0u` for anything unrecognised. Two unrecognised targets\nwould both take it, agree on every other ingredient, and land on the same\nidentity — the original defect wearing a different number: each would open the\nother's files having established nothing. Grouping \"Linux with a libc that is not\nglibc\" repeated it one level down, on musl and bionic, which are precisely what\nthe tag was added to tell apart.\n\nThere is no catch-all now. An unrecognised platform does not compile, and says\nwhat to do about it. bionic gets its own tag, checked before glibc because it\ndefines neither. A build that knows what it is can say so with\n`UTXOZ_PLATFORM_TAG`, above a floor that keeps an explicit tag from colliding with\none this header assigns itself, and a `static_assert` refuses zero — unreachable\nthrough the branches, and exactly what an unset build definition looks like.\n\nLinux/glibc keeps tag 4 and the fixtures keep identity 3060504794, so nothing was\nregenerated.\n\n## Checking a refusal that happens before there is a program\n\nNone of that is observable from a running suite: a compile-time refusal is\ninvisible to a test binary that only exists if it compiled. So ctest builds a\ntranslation unit with a bad tag and requires the build to fail — 0 for the unset\ncase and 999 for the floor.\n\nWith a control, which is the part that matters. A `WILL_FAIL` test passes when the\nbuild fails for *any* reason — a mistyped target, a missing header, a broken\ntoolchain — so on its own it proves nothing about the assertion it is meant to be\nreaching. The same file compiled with a tag that is allowed has to succeed, and\nthat is what makes the other two mean something.\n\nSuites: 222 cases with statistics, 217 without, and three build-contract tests\nbeside them.",
          "timestamp": "2026-08-14T14:50:13+02:00",
          "tree_id": "660d260a9fdbf98677fd9313dd9491ade1b44792",
          "url": "https://github.com/utxo-z/utxo-z/commit/cc1278f5c0c7dc3263128a2c9ce0126ecd36b2c7"
        },
        "date": 1786712078068,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 319865.66,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 423603.95,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 430505.14,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 461711.26,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 439.38,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 493.95,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 13527251.08,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 26881649.2,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12949209.11,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12218.37,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2265179.65,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2285211.16,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 110372.28,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 11515.22,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2748.81,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3448.89,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2833.1,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 53.76,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 53.78,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 53.75,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 53.91,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.92,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.81,
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
          "id": "0764ce8a74e466e865d4d747d671ef1bc05036c9",
          "message": "feat!: the second container class is 96, and stops wasting the two bytes it always had (#132)\n\n`utxo_value<94>` never occupied 94 bytes. Four for the height, one for the length\nand eighty-nine of payload come to 94, which rounds up to the alignment — so the\nobject has always been 96, and the two bytes at the end were padding no caller\ncould reach. They were copied, written to disk, and, until recently, uninitialised.\n\nNaming the class 96 turns them into payload. Measured, and the whole of it:\n\n    sizeof(utxo_value<94>) = 96      sizeof(utxo_value<96>) = 96\n    sizeof(pair with 94)   = 132     sizeof(pair with 96)   = 132\n    payload usable  94 -> 89         96 -> 91\n\nSame object, same stored pair, same bytes per slot, same entries per file. Two more\nbytes a caller can use, at no cost anywhere the capacity profile measures.\n\n## It is still a format change\n\nCapacity 89 becomes 91, so a 90- or 91-byte output used to go to container 2 and\nnow fits in container 1. That is a value living somewhere else, which is what\n`geometry_id` is for: it moves from 1 to 2, and a `static_assert` on\n`container_sizes` makes forgetting impossible rather than merely unlikely.\n\n`map_layout_epoch` and `hash_epoch` do not move, and should not: the map layout is\nbyte-identical and no key changes bucket. Only where a value is stored changes.\n\nDatabases written under geometry 1 are refused with `geometry_mismatch` and have\nto be rebuilt from the chain. There is no migrator and deliberately will not be\none — a value's container is decided by its size, so migrating means reading every\nentry and writing it elsewhere, which is rebuilding with the added risk of a tool\nthat believes it is doing something safer.\n\n## The line, tested where it moved\n\nFour payloads, because the classes are coarse and almost every size sits in the\nmiddle of one and proves nothing: 89 fitted before and fits now; 90 and 91 have\ncrossed; 92 has not. Each is inserted, read back byte for byte — a value in the\nright container missing its last byte satisfies every count in the store — and\nthen the container file it should have landed in is required to exist, because\n\"it reads back\" is equally true of the wrong container.\n\nThe last size each of the five classes accepts is checked too, so a change to any\nof them has to come through this test.\n\n## What keeps the padding honest\n\nNo class in the geometry rounds up any more, so the branch in `set_data()` that\nclears padding now compiles to nothing. It stays, and 94 stays tested beside it as\na size that is not a container class: the guarantee is about the object, and a\nbranch that is only ever unreachable is a branch nobody has checked. The tail of\n`data` is still cleared for every class, which is where nearly all of it was.\n\nFixtures regenerated under geometry 2; two runs at a fixed identity produce\neighteen identical files. Suites: 225 cases with statistics, 220 without.\n\n## Two tests that were not testing what they said\n\nChecking that `cont_1_v00000.dat` and `cont_2_v00000.dat` exist proves nothing\nabout where any particular key went, because the same test fills both. And\n`find()` searches every container, so it answers \"the store has this\" and never\n\"it is here\". Between them, the placement half of these cases was unchecked.\n\nThey now open the segments and look in the named map with the type that container\nactually holds: each key is required to be in the container it belongs to, absent\nfrom every other one, and to read back byte for byte from there.\n\nThe upper edge was missing too. \"Every class boundary\" inserted only the capacity,\nwhich leaves the half that actually moves when a class changes untested. Each\nnon-final class now gets `capacity` and `capacity + 1`, and the largest class gets\none byte past it — which must be refused with `value_too_large` and must not\npersist the key anywhere.\n\n## And geometry 1 by name\n\nThe existing case moves the geometry up by one, which proves \"some other geometry\nis refused\". It does not pin the incompatibility this change introduces. Geometry\n1 is every database written before the second class became 96, and it is what an\noperator is holding, so it is refused by name — with container opens forced to\nfail, so the refusal is an ordering and not merely an outcome: nothing is mapped\nbefore the config is read and rejected.\n\nFour mutations, each confirmed to fail the suite:\n\n- capacity 91 reverted to 89 — fails to build, on the assertion that pins it;\n- 90 and 91 routed back to container 2 with the capacities left intact — 2 of 3;\n- `capacity + 1` kept in the class below — 2 of 3;\n- geometry 1 accepted — 1 of 25 in the barrier suite;\n- a value too large truncated into the last class instead of refused — 1 of 3.\n\nSuites: 226 cases with statistics, 221 without.",
          "timestamp": "2026-08-14T16:59:53+02:00",
          "tree_id": "5371b523d46208e59a9755938ca62acd5b778e6f",
          "url": "https://github.com/utxo-z/utxo-z/commit/0764ce8a74e466e865d4d747d671ef1bc05036c9"
        },
        "date": 1786719881934,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 271108.02,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 269698.53,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 235856.23,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 358050.1,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 387.47,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 452.1,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12765510.1,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 26051730.51,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12419420.09,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12943.42,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2092239.97,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2297299.42,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 65203.12,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 6255.7,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 1972.86,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3019.21,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2727.39,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 52.74,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 52.7,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 52.94,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 52.75,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 53.26,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 53.05,
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
          "id": "b3cf6cb91f50796d3ab8201b604b8767b04dfcb6",
          "message": "feat: an instrument that answers how large a segment has to be, instead of inheriting one (#133)\n\nThe file sizes were chosen once and everything since has been fitted into them.\nThis asks the question the other way round — given a class and a target bucket\ncount, how many bytes does the segment need to build and operate that map — so a\nsize can be decided rather than inherited.\n\n`utxoz_sizing` is diagnostic only. It is never installed, nothing a database does\ndepends on it, and it is allowed to see `bad_alloc` precisely because it is the\none place that may: finding the smallest segment that works means asking segments\nthat do not. `configure()` must never learn its sizes that way, which is the\nreason this exists apart from it.\n\n## Three parts, not one number\n\nA single figure from a bisection hides its own assumptions, so the estimate is\ncomputed and broken down, the minimum is measured, and the difference between them\nis reported as overhead rather than smoothed away. A reader can check the\narithmetic against the measurement and see which half they are trusting. On Linux\nthe overhead came to 1743 bytes against a 1276 MiB map — small, and now a number\nrather than a belief.\n\nThe search starts from the arithmetic instead of from nothing. Bisecting the whole\nplausible range would build a 1.2 GiB table thirty times to learn what one\ncalculation and a dozen trials already say; it brackets around the estimate and\ncloses to a precision it reports, because a file size should not be chosen on the\nlast byte.\n\n## Building is not operating\n\n`--fill` inserts up to the rotation threshold and checks the bucket count never\nmoved. That is the half a construction test cannot reach: a segment can hold the\narrays and still run out when the entries arrive. At the production step for\ncontainer 0 it inserted 13,074,431 entries with no rehash, which is the property\nthe policy will rest on.\n\nIt also explains a figure that would otherwise look wrong. Constructing the map\ntouches the group metadata and leaves the slots alone, so a 1339 MiB file reports\n16 MiB of blocks until it is filled. Logical size and physical blocks are reported\nseparately for that reason, and Windows says `unavailable` rather than offering the\nlogical size as consumption — a number that reads like a measurement and is not.\n\n## The classes come from the geometry\n\nThe dispatch enumerates the containers by hand, because each is a distinct type,\nso it is pinned twice: the size list and the count. Changing `container_sizes`\nstops this compiling rather than leaving an instrument quietly measuring a geometry\nthat no longer exists. Verified by changing it.\n\nEleven cases exercise the instrument at small steps — the ladder, capacity to\nbucket count, every class, monotonicity, the recommended size building and the\nfloor being a floor, the next step not fitting, the fill not rehashing, nothing\nleft behind, seven bad command lines each named in its error, and the JSON parsing\nand landing where it was asked to. The production figures are a lane that runs\nonce per platform and uploads its JSON, rather than a suite that builds gigabyte\ntables in every case.\n\n## Build\n\nNothing here uses `CMakeUserPresets.json`. Two Conan installs in one checkout give\ntwo presets with one name, and `cmake --build --preset` then fails or, worse,\nsilently runs a binary from the other configuration — which happened during this\nwork and nearly turned a mutation that was caught into one reported as missed. The\nlane names the executable it ran.\n\nSuites: 237 cases with statistics, 232 without.\n\n## It was not measuring what it said\n\n`try_build` constructed a map called \"m\" in an otherwise empty segment. Production\nplaces a stamp first and constructs the map under `map_object_name`, and both\nconsume space — so the floor being reported was the floor for a file the store\nwould never have written.\n\nThere is one build helper now, used by the search, the physical measurement, the\nfill and the growth probe, and it does what `create_container` does in the order\nit does it: segment, stamp with a fixed identity, then the map under the real name\nwith the real allocator. The numbers moved by less than the precision — the\nallocator had slack the stamp fits into — but the report now measures the thing it\nnames.\n\n## Coexistence is asked, not inferred\n\n`free_after >= 2 * (slots + metadata)` is arithmetic about free bytes, and what\nthe policy needs to know is whether the allocator will actually satisfy a growth.\nSo the map is built in the segment, seeded past any empty-map shortcut, and asked\nto `rehash` to the next bucket count. What is reported is what happened, and by\nwhich method.\n\nKept separate from `next_builds_alone`, because they are different questions: one\nis whether the larger map could ever live there, the other whether Boost can get\nto it from the map that is already in place.\n\nThat measurement is also what `--segment` is for. Pointed at the file that exists\nrather than the one the tool would recommend, it answers the question that started\nall of this — and the answer is not a deduction any more:\n\n    container 0, 7 864 319 buckets in today's 2 GiB   → it grows on its own: YES\n    container 0, 15 728 639 buckets in 1339.80 MiB    → no\n    reference,   7 864 319 buckets in today's 4 GiB   → it grows on its own: YES\n\n## An exception is not a measurement\n\n`catch (std::exception)` turned a permission error, a full filesystem or a bug\ninto \"does not fit\", and the number that came out looked exactly like a real one.\nOnly `bad_alloc` now means capacity; anything else aborts with a diagnostic and\nits own exit code, so a caller can tell \"it does not fit\" from \"nothing was\nmeasured\". The build at the recommended size no longer swallows its failure —\nreporting on a file with no map in it is worse than reporting nothing — and\n`--fill` names the error rather than leaving `filled: false` unexplained.\n\n## The floor is a bracket, and says so\n\nThe search now keeps both ends it tried: a size that did not build, and one that\ndid. Both are re-tried and reported with the result, the precision is their\ndistance, and the upper end is called an upper bound rather than \"the minimum\",\nbecause the minimum is a byte nothing tested. A mutation that lets the lower end\nbe accepted fails the suite.\n\nThe margin rounds up instead of truncating. `value / 100 * percent` loses up to a\nhundred bytes per point, and this is a safety bound.\n\n## Measured again\n\n    class 48, 15 728 639 buckets\n      bracket    1276.00 MiB did not build, 1276.00 MiB did  (±2552 bytes, 22 attempts)\n      overhead   1743 bytes\n      margin 5%  63.80 MiB          recommended  1339.80 MiB\n      filled     13 074 431 entries, bucket_count unchanged, 390 s\n      next step  31 457 279 neither builds alone nor is reachable by growth\n\nSuites: 237 cases with statistics, 232 without.\n\n## Three ways a failure could still look like a measurement\n\nA stamp that would not go down was folded into the search as one more size that\ndid not fit. It is fifty-six bytes at the start of a segment already sized for a\nmap thousands of times larger, so \"there was no room\" is not what a failure there\nmeans — a permission error or a name collision would have been counted as\ncapacity. `nullopt` is now reserved for the map: the stamp failing throws with its\nerror code, and `--fail-stamp` reaches that path for a reason unrelated to space,\nso the refusal is tested rather than asserted.\n\nA `--fill` that was asked for and did not finish reported `filled: false` beside a\nsuccessful exit code. A script reading that code would accept a report that never\ndid what it was told. It is fatal now, with the cause named and the temporary\nremoved.\n\nAnd every full-mode class stamped its segment as container 0. The kind travels\nwith the class now, from the same dispatch the sizes come from, and the stamp is\nread back after building — a segment identified as something it is not is a\nsegment the store would refuse, and measuring one is measuring the wrong file.\n\nThe small path through the fill was re-run rather than the six-minute one: nothing\nhere changes layout or capacity, and the figures are unmoved.\n\nSuites: 240 cases with statistics, 235 without.\n\n## Zero is not \"unset\"\n\nThree options used zero as though it meant absence, and one of them could hang the\ntool rather than answer wrongly: with `--precision 0` and a bracket one byte wide,\nthe midpoint is the lower bound, the lower bound is set to itself, and the search\nnever ends. All three are refused on the way in — precision, buckets and segment —\nand the search keeps a one-byte floor of its own so an internal caller cannot\nreintroduce it. `--segment` is an optional now rather than a sentinel, so a value\nof zero is a mistake and not a missing option.\n\nThe precision case has its own test, and it is timed: a case that only checked the\nmessage would pass while the tool ran forever.\n\n## What the lane runs\n\n`find -name 'utxoz_sizing*'` would also match a `.pdb`, an object file or a stale\ncopy from another configuration, and the first one found would be measured with\nnobody the wiser. It is the exact name now, executable on Unix and `.exe` on\nWindows — and if there are none or more than one, the lane says so and stops\nrather than choosing.\n\n## A writer that does not depend on its callers\n\nEvery string this tool emits is validated or a literal, so nothing could have got\ninto the JSON unescaped. That is a fact about today's callers rather than about\nthe writer, and one nobody could safely add a field to. Strings are escaped —\nquotes, backslash, the three whitespace controls and everything below U+0020 —\nand `fill_error` is gone from the report altogether: a fill that failed does not\nproduce a report, so carrying its message into one described a state that cannot\nexist.\n\nMeasurements unmoved: 1276.00 MiB floor, 4296 bytes of overhead, 1339.80 MiB\nrecommended. Suites: 241 cases with statistics, 236 without.\n\n## bash 3.2\n\nThe exact-name lookup used `mapfile`, which is a bash 4 builtin. macOS runners\nship bash 3.2, so the step died with \"command not found\" after all 244 tests on\nthat platform had already passed — a green suite followed by a red job, for a\nshell feature.\n\nReplaced with a form 3.2 has, and checked against all three answers it has to\ngive: nothing found, exactly one, more than one. No other script in the repository\nuses `mapfile` or `readarray`.",
          "timestamp": "2026-08-14T19:12:46+02:00",
          "tree_id": "c0d12e2ad716a6a630fc0130e572abca8e1a12bb",
          "url": "https://github.com/utxo-z/utxo-z/commit/b3cf6cb91f50796d3ab8201b604b8767b04dfcb6"
        },
        "date": 1786727833938,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 282827.86,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 477888.84,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 268409.89,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 505809.14,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 483.97,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 536.08,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12535380.61,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 25989469.68,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 11900411.53,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12074.14,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2313761.76,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2392338.3,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 70755.61,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 13150.8,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2710.93,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3403.84,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2824.64,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 56.3,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 56.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 56.39,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 56.19,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 56.19,
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
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "701b6d129af5d05fa45b26adc08ca9bf929edb27",
          "message": "feat!: a container's capacity and its file size are one decision, made once (#134)\n\nThey were two, and neither was decided. The file sizes arrived with the initial\nlibrary structure in January and nothing has revisited them since; the capacity\ncame out of `find_optimal_buckets`, a bisection that created and destroyed a\nsegment of the\nfull file size at every probe and read `bad_alloc` as its answer. The result was\nnot a choice but whatever survived the probes — and what actually limited\ncontainer 0 was neither memory nor measurement but a constant, 7864304, sitting\nin the call.\n\n`capacity_policy.hpp` holds both now, per container and per profile, measured by\nthe instrument from #133 on Linux, macOS and Windows.\n\n## Container 0\n\n    bucket_count           7 864 319  ->  15 728 639\n    entries before rotating 6 537 215  ->  13 074 431\n    file                   2048 MiB   ->  1340 MiB\n    a growth fits in it    yes        ->  no\n\nTwice the entries per generation in two thirds of the file, and the step above no\nlonger fits — so a growth cannot complete even if the guard were bypassed.\n\nThe size is not a round number that happens to work. The floor was measured at\n1 337 987 188 bytes on Linux, 48 fewer on macOS and 56 fewer on Windows — a fixed\ndifference, `sizeof(segment_manager)`, constant across all twelve measurements.\nFive per cent on the largest, rounded up to a whole mebibyte, is 1340 MiB, which\nis also a multiple of every page and mapping granularity in use. One size for\nthree platforms: a statement about the rule, not about the files, which remain\ncorrectly refused across platforms by `platform_abi_id`.\n\nThe other four classes and reference keep exactly what they had, recorded rather\nthan discovered and marked as not certified. Choosing them needs data this project\ndoes not have yet — a live histogram of output sizes, survival, which tier answers\na lookup — and the measurements are in doc/capacity-policy.md waiting for it.\n\n## The invariant, and where it is enforced\n\nA generation's bucket count never changes. That is the design: a container that\nfills up gets a new generation and compaction deals with the cost later, so a map\nthat grew instead is a defect even where there was room for it.\n\nThe guard now compares integers — `133/160` of the bucket count, which is\n`0.875 × 0.95` exactly — and takes that count from **the map that is open**, not\nfrom the policy. A generation written under the old policy keeps its own threshold\nof 6 537 215; a new one gets 13 074 431; the fixtures get theirs. The policy\ndecides what a new segment is created with. The file decides how it is operated.\n\nThe float it replaces could not represent a bucket count exactly above 2^24, which\nreference mode already exceeds. The error was far smaller than the margin, and a\nthreshold that decides whether a file rotates should not be computed in a type\nthat rounds.\n\n`rehashes_observed` counts generations whose bucket count moved, in **every**\nbuild rather than behind the statistics switch — an operator running without\nstatistics is exactly who needs to know. It is checked against the count the\ngeneration was opened with, so a growth from any path is caught, not only one that\nan insert witnessed.\n\nIt cannot become an error the caller acts on. It runs after the insert, and a\nretryable failure there would invite a second write of an entry that is already\nstored. The entry stands; the growth is logged and counted, and a build with\nassertions stops rather than burying the evidence.\n\n## Tested at three levels, because they fail separately\n\nThe arithmetic, where an off-by-one lives: the threshold is inclusive, so a map\nholding exactly that many is full and the next insert rotates — stated once so\nthat \"13 074 431 are allowed\" and \"rotate before the 13 074 432nd\" cannot drift\napart.\n\nThe behaviour, driven through the ordinary path at a size every CI run can afford:\ncontainer 4 in the test profile is 959 buckets and fills at 797. Insert to the\nlimit, nothing moved; one more, and a second generation appears while the first\nkeeps its bucket count and its entries; both are still readable, through\n`resolve()` for the one that rotated out. Reference gets its own pass rather than\nan argument by analogy.\n\nAnd the invariant itself, which no assertion can stand in for: the counter, across\nevery class and a rotation, in both configurations.\n\nMutations, each confirmed: removing the guard, moving the threshold to Boost's\ngrowth point, allowing one more insert, changing the requested capacity, and\ndropping the bucket-count check while relaxing the guard all turn the suite red.\nChanging anything in the production table fails the build: geometry 3 is compared\nagainst the table it is defined to be — every file size, capacity and bucket\ncount, for all five containers and reference — rather than against a list of\nfields somebody chose to check. Container 3's file size, the reference file size\nand any bucket count were each moved, and each stopped the build.\n\n`configure()` went from 0.18 s to 0.02 s with production sizes, and the suite from\n86 s to 65 s, but speed was never the argument — the argument is that a size can\nnow be read, checked and defended.\n\nSuites: 259 cases with statistics, 254 without.\n\n## Two thresholds, because there are two questions\n\nThe first version of this used one number for both, and it was the wrong one for\nhalf of them.\n\nA **live** container keeps five per cent of reserve — `133/160` of its bucket\ncount — because it is still receiving inserts and should make a new generation\nwith room to spare rather than at the last possible entry.\n\nA **sealed compaction target** is built once and never inserted into. It needs\nonly to not grow, so its limit is where Boost actually grows: `7 * buckets / 8`,\nin integers. Applying the live threshold to it refused merges that fit perfectly\nwell — two generations totalling fifty entries over the reserve, in a map with\nfive thousand to spare — which is what turned two existing compaction tests red.\n\nThe formula is a claim about Boost, so it is checked against Boost: real maps,\nfilled one entry at a time, at every step this project uses. Three points, and\nthe third is what makes the other two mean anything — at the limit the bucket\ncount holds, and one past it the map grows. Without that control the first two\nwould pass against a map that never grows at all.\n\nA target may therefore end up above the operating threshold, which is fine while\nit is sealed. The moment it becomes the active map, an insert rotates rather than\npushing it further: the guard reads the map's own size and answers before the\ninsert, not after.\n\n## Two guards nothing was watching\n\nBoth of these were written, reviewed and green, and neither was observed by a\ntest — the code could have been deleted and nothing would have said so.\n\n**A duplicate at the limit.** The order matters only when the map is full: below\nthe limit `emplace` answers both questions at once. So the case builds a target\nthat reaches the limit exactly — 12 767 entries plus 672 — and offers a third\nsource of exactly one entry, which makes the outcome independent of the order the\nmap iterates in. When that entry duplicates one already present the answer is\n`duplicate_key`; when it is new, `insufficient_space` and a retry with fewer\nsources. Asking the capacity first turns it red: the duplicate is reported as a\ngroup that is too large, the retry then succeeds, and a locally inconsistent\ndatabase is published as a clean compaction.\n\n**A packed target the next insert must not touch.** Building this state is the\nwhole difficulty, and the first attempt did not build it: the merged target landed\non exactly the growth point, where both thresholds refuse alike, so replacing the\noperating threshold with the growth point left the case green. It now lands\nstrictly between them — the operating threshold plus half the reserve — and the\nmutation turns it red twice over: no new generation appears, and the sealed\ngeneration gains an entry.\n\n## Four assertions that allowed either answer\n\n`compact_all` does not report `insufficient_space` to its caller: a group that\ndoes not fit is refused inside the walk, which retries with fewer sources and\nreports that it finished. Four cases said `if ( ! outcome) CHECK(the error is\ninsufficient_space)`, which asserts nothing at all — measured at each of the four,\nthe returned value is success. They now require it, and the refusal is read where\nit is visible: in the files.\n\n## wasm32, and what the geometry assertion does not say there\n\nThe pinned table was first written in decimal bytes, and `4294967296` does not\nnarrow to a 32-bit `size_t`. That is not a problem with the pin: `4_gib` is\ncomputed in `size_t`, so on wasm32 `reference_file_size` already wraps to **zero**,\nsilently, and has for as long as the constant has existed. The table is now\nwritten in the project's own unit literals, which follow the target's `size_t`\nexactly as the constants do.\n\nSo the assertion says less there than it looks like it says: both sides are the\nsame truncated value, which is agreement rather than certification. Reference mode\ncannot create its production segment where that constant is zero. Issue #135\nrecords it with the options; it is deliberately not fixed here, because fixing it\nmeans changing the type of every configured size and the Boost boundary they cross.\n\n## Compaction\n\nIt now knows before inserting whether an entry fits. A duplicate is rejected first\nand separately — the database being locally inconsistent is a different fact from\nthe group being too large, and it costs no capacity — and a new key is not\nattempted at all if it would take the map past the growth point. A refused group\nleaves every source untouched, publishes nothing, and lets the caller retry with\nfewer sources. The per-entry bucket-count check stays as a detector of something\nunexpected rather than as the mechanism.\n\n## Two tests that were arithmetic on the wrong width\n\n`bucket_step` returns `size_t`, and two checks in the arithmetic case deliberately\nreach past any real geometry — the point of them is that `max_entries_for` takes a\n`uint64_t` and stays exact. Shifting a `size_t` by 35 is undefined where `size_t`\nis 32 bits, so the ladder is built in the wider type there and tied to the real\none where both can represent the answer.\n\nAnd one bound for scanning generations instead of three. Sixteen in four cases,\nthirty-two in one, sixty-four in another: a case that scanned fewer than another\nwould miss a generation and report a smaller database, silently and only\nsometimes.\n\n## Counting a growth once\n\nThe counter compares against the count last *observed*, not the one the generation\nopened with. Comparing against the opening value reports the same growth again on\nevery insert that follows, turning one defect into a figure that measures how much\nwas written afterwards. A second, different growth is a second violation.\n\nDetection is separated from the assertion so the counting can be tested in a build\nwhose assertion would otherwise stop the program.\n\n## geometry_id 3\n\nNeither the config nor the stamp records a capacity, so before this there was\nnothing a reader could check: a binary with a new policy would have opened an\nolder database and operated it with thresholds meant for a different file. From 3\nonwards `geometry_id` identifies the whole storage geometry — the classes, which\npayload goes where, the capacity and the segment size — and geometry 2 is refused\nbefore anything is mapped. There is no migrator. `map_layout_epoch` and\n`hash_epoch` do not move.\n\nFixtures regenerated under geometry 3, with their bucket counts unchanged: the\nphysical difference comes from the identity, not from a capacity that drifted. Two\nruns at a fixed identity produce eighteen identical files.\n\n## What the mutations do and do not show\n\nRed, as they should be: shifting the growth limit by one fails the build on the\nassertion that pins it; counting every insert after a growth instead of the\ntransition fails seven cases; removing the live guard, moving its threshold to the\ngrowth point, allowing one more insert, and changing the requested capacity all\nfail.\n\nNot shown, and worth saying rather than leaving implied: **removing the compaction\nguard changes nothing observable in any configuration these tests can build.** At\nthe ten-megabyte test profile no class can grow at all — going from 959 buckets to\n1919 needs 18.8 MiB — so `bad_alloc` refuses first and the outcome is identical.\nThe guard matters where a file has slack, which today means reference in\nproduction, and that is the case this PR deliberately does not touch.\n\nSuites: 254 cases with statistics, 249 without.\n\n## The guard was untestable, and that was the point\n\nRemoving the compaction guard changed nothing observable, and the reason was not\nthat the guard is unnecessary: at ten megabytes no container can grow at all —\n959 buckets to 1919 needs 18.8 MiB — so `bad_alloc` refused first and the outcome\nwas identical either way. The tests could not tell \"the policy saw that a unique\nentry would not fit\" from \"we tried to grow and the allocator said no\", which\nleaves `bad_alloc` working as an accidental protocol.\n\nA capacity seam fixes that: a small map in the same file leaves room for the step\nabove, so Boost really can grow there. The case opens with that as a control — it\nfills a map in that very segment past the limit and requires the bucket count to\nmove — because without it the rest would pass against a map that could never have\ngrown, which is the mistake being corrected.\n\nWith the room proved present, compaction still grows nothing. Neutralising only\nthe preventive check takes the counter from zero to five.\n\nThe seam is per container, not global: one capacity for every class would ask the\n10240 class for a hundred and fifty megabytes inside a ten-megabyte file, which is\nhow the first attempt failed.\n\n## Smaller things from the same round\n\n`max_size_without_rehash` divides before multiplying, so nothing overflows however\nlarge the bucket count grows, and the remainder term keeps it exactly `floor(7n/8)`.\nIt is documented as a certified property of the Boost version this project\nsupports rather than a universal truth — the empirical tests are the authority and\nthis function is the shorthand.\n\nThe watch keeps both states again. `last_reported` is the deduplication;\n`at_open` stays so that every report can name where the generation started rather\nthan where the previous growth left it. A second growth now reads \"grew to 3839,\nfrom 1919 and originally 959\".\n\nAnd a duplicate key is reported as a duplicate even when the target is at its\nlimit: a database that is locally inconsistent and a group that is too large send\nthe caller to different places, and a duplicate costs no capacity, so it is\nchecked first.\n\nSuites: 256 cases with statistics, 251 without.\n\nThe count the mutation produces is written down where it is asserted, so that\nnobody reads it as an expected value. The assertion is equality with the count\nbefore — any growth at all fails it — and removing the guard yields several\nrather than one because `compact_all` tries the largest group and shrinks on\nrefusal, each attempt growing its own target before the per-entry detector stops\nit. How many depends on how many groups get tried, and is not part of the\nscenario.\n\n## A seam nothing reads\n\nThree of the refusal cases armed `fail_container_open` and said it proved the\nconfig is rejected before anything is mapped. It proved nothing: `open()` never\nreads that failpoint — only the compaction path does — so the seam was inert and\nthe claim was decoration on a test that could not fail either way.\n\nWhat they check now is observable and belongs to the code under test: the\nmodification time of every file before the refused open and after it. And the\nfirst thing that measurement said was that the claim as written was false — an\nopen that refuses still takes the directory's claim and records who holds it, so\n`.utxoz.lock` does change. That is correct behaviour, the refusal is a decision\nthis instance had to hold the database to make, and it is excluded by name with\nthe reason beside it. What must not change is the data, and it does not.\n\n## Capacity is part of the geometry, and now says so\n\n`geometry_id` 3 covers the capacity policy, but nothing pinned it: the classes had\nan assertion and the capacities did not, so a bucket count or a segment size could\nhave moved without the id following. Both now fail the build if they do.\n\nThat surfaced a duplicate: `bucket_step` was defined in the policy header and\nagain in the sizing tool. The tool uses the policy's, which is the point of having\none — an instrument measuring a ladder the store does not use would be measuring\nsomething else.\n\n## Smaller\n\nBelow the limit `emplace` is the only lookup a compaction entry costs; the\nseparate `find` runs only at the limit, where the order genuinely matters and a\nduplicate must still be reported as a duplicate. The dead `bucket_count_before`\nis gone from both insert paths. And the paragraph describing the inclusive\noperating threshold now sits above `max_entries_for`, which is what it describes,\nrather than above the growth-point function it had drifted onto.\n\nOne nitpick is skipped: requiring `compact_all` to fail with `insufficient_space`\nwould be wrong, because it succeeds. It refuses individual groups internally and\nreports that it finished, having merged nothing — checked before deciding.\n\n## Two properties, two observations\n\nModification time proved neither of them. It is not evidence that content is\nintact — a file can be rewritten with the same bytes — and it is not evidence of\nordering, since a segment can be mapped read-write and left alone.\n\n**The data is unchanged** is now a byte-for-byte digest of every persisted file\nbefore and after the refused open, plus a check that nothing half-made was left.\n`.utxoz.lock` stays excluded, with the reason: an open that refuses still takes\nthe directory's claim and records who holds it, so the honest claim is about the\ndata rather than about every file.\n\n**Nothing was mapped** is a counter at the only place this store maps an existing\nsegment, incremented inside `open_existing_segment` itself. Zero after a\ngeometry-mismatch open is the ordering, not an inference from it. A counter rather\nthan a seam that fails when reached: what has to be shown is that the edge was\nnever reached, and a seam that fails on arrival only shows what happens if it is.\n\nAnd the counter has a control, which is what makes it mean anything: the same case\nrepairs the config, opens successfully, and requires the counter to move. Without\nit a counter that never counted would satisfy the zero check, and the ordering\nwould be asserted by a number that is always zero — which is exactly what the\nmutation confirms: neutralising the increment turns that case red only because the\ncontrol is there.",
          "timestamp": "2026-08-17T12:25:51+02:00",
          "tree_id": "3eee45d4d9754fd6caab86d1347691fb1cd1c2f4",
          "url": "https://github.com/utxo-z/utxo-z/commit/701b6d129af5d05fa45b26adc08ca9bf929edb27"
        },
        "date": 1786962623074,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 266999.7,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 557969.67,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 484610.03,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 788542.75,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 452.49,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 530.41,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12907707.09,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 26081017.88,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12711958.38,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12992.91,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2291964.12,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2407346.46,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 124005.12,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 13096.46,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2789.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3641.92,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2965.88,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 1072.98,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 1059.29,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 1066.49,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 1074.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 1101.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 1089.64,
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
          "id": "2e39afa48a5e22debfd246ef76c519d21054c310",
          "message": "feat: count what is stored, instead of what this process happened to do (#137)\n\n`print_sizing_report()` looked like it answered \"what is in this database\". It\nprinted \"Current entries\", a file size per container, \"Wasted bytes\", and a\nglobal histogram of value sizes. Every one of those except the file size is a\ncounter that started when this process opened the database, and the file size is\nthe setting a new segment is created with rather than what any file occupies. A\nnode restarted a minute ago printed almost nothing and printed it under names\nthat read like storage.\n\nNothing walked the files. `census()` does.\n\n## What it reports\n\nPer generation, per class and in total: entries, bucket count and load factor;\nthe payload each entry actually uses and the capacity it leaves unused; the\nsegment's size and the bytes its allocator never handed out; the file's length\nand the blocks the filesystem actually gave it; and a histogram of the payload\nsizes that occur.\n\nThat last one is the number this was built for. Choosing container sizes from\ninserts this session has seen is choosing from a sample nobody selected; choosing\nthem from the payloads in the database is the measurement, and it is now one\ncommand.\n\n## Exact, modelled, residual — never added up as one kind of number\n\n    exact       payload, unused capacity, object padding, segment size,\n                segment free, logical file size\n    modelled    occupied slots, empty slots, group metadata — computed from the\n                certified map layout, not observed from the allocator\n    residual    unattributed_allocated_bytes = allocated - modelled\n\nThe residual is a subtraction, so every modelling error lands in it. It is\nreported as a residual and never described as the segment manager's own overhead,\nwhich would be a claim about bytes nobody counted. When the modelled parts come\nto more than was allocated the model is wrong, and `model_inconsistent` says so\nand the residual is withheld rather than clamped to zero — a zero there would\nread as a tight fit.\n\nNothing is double counted: empty slots and group metadata are inside the map's\nallocation, `segment_free_bytes` is what the allocator never handed out, and\n\n    occupied + empty + group_metadata + unattributed + free == segment_size\n\nholds per generation and is asserted per generation. In aggregate it would hold\nby construction and would be checking arithmetic instead of the model.\n\n## Physical blocks, and the two platforms that do not measure the same thing\n\nA version file is created at its full size and filled in gradually, so on a\nfilesystem with sparse files its length and its cost are different numbers — and\nthe cost is the one that is paid. POSIX answers with `st_blocks x 512`, which is\nblocks allocated. Windows answers with `GetCompressedFileSize`, which returns the\nsize on disk only where the volume supports compression or sparse files, and\notherwise returns the logical size — so a Windows figure equal to the file size\nis not evidence that nothing is sparse. The method travels with the figure for\nexactly that reason.\n\nIn a test database of 600 000 entries, 20 MiB of segment reported 475 KiB of\nblocks. That difference is the point.\n\n## Missing is not zero\n\nEvery figure that might not exist carries a status: `measured`, `not_applicable`,\n`unavailable` with a reason. The JSON writes the number as null unless it was\nmeasured, so a consumer comparing two machines cannot read \"nobody could measure\nthis\" as \"this is small\". A sum containing an unmeasurable part is unmeasurable\ntoo, rather than a smaller number wearing the name of a complete one.\n\n## What this scope does not claim\n\n`physical_stored` counts every entry in every generation, once per copy. It is\nnot the logical state, and the difference is not hypothetical: an insert consults\nonly the active map of the class its payload size selects, so one outpoint\ninserted twice with payloads of different sizes is stored in two classes at once.\n\nMeasured, not inferred:\n\n    insert 8B   -> INSERTED          (class 0)\n    insert 200B -> INSERTED          (class 3)\n    db.size() = 2\n    find -> payload 8 bytes\n    apply_deletes: erased=1\n    find after delete -> payload 200 bytes   <== the other copy answers now\n\nSo `entries` is a count of stored entries, not of distinct outpoints, and the\nreport says so in its own text — a pasted excerpt carries the caveat with it. The\nlogical census is the next piece of work and has its own algorithm and its own\nmemory bound.\n\nThe same walk turned up something worse, filed as #136 rather than fixed here:\n`full_resolve()` consults the LRU-cached files first, so on an already\ninconsistent database which copy answers depends on what this process read\nearlier. Two nodes with the same files can disagree, and one node can disagree\nwith itself across a restart.\n\n## The tool\n\n`utxoz_census <dir> [--text|--json] [--mode=reference] [--snapshot]` opens the\ndirectory the ordinary way — `open()`, which validates the config and the stamps\nand takes the claim — and asks the library to count. It uses only the public API\nand does not read the format itself: a tool with its own idea of what a segment\nlooks like is a second implementation of the format, and the day they disagree\nthe untested one wins.\n\nThe claim is exclusive, so a running node holds it and the tool refuses rather\nthan reading underneath it. `--snapshot` records that the directory is a copy\nwhose consistency depends on how it was taken, which nothing here can see.\n\n## print_sizing_report()\n\nKept, and no longer misleading. It says what it is before the numbers — session\ncounters, not the contents of the files — and points at census() for the rest. It\nstill reads no file, because a function that quietly grew a full pass over the\ndata would be worse than one that overstated its numbers. And in reference mode\nit prints one class instead of four containers that do not exist.\n\n## Cost\n\n600 000 entries across 10 files in 4-6 ms with the page cache warm, which is what\na repeated run measures; a cold cache makes it an I/O measurement instead. Every\nreport carries its own `duration_ms`, `files_examined` and `entries_examined`, so\nthe cost is a figure in the output rather than a claim in a commit message.\n\n## The tool is not a test artifact\n\n`utxoz_census` is built by `UTXOZ_BUILD_TOOLS`, on by default, and installed with\nthe library. It was inside the test block first, which meant a node operator\nbuilding with tests off could not produce the one thing here they would actually\nrun. It depends on neither the fixtures generator nor the sizing instrument, and\na CI job builds with tests, examples and benchmarks off, asserts that those three\ntargets were *not* built, installs, and runs the installed copy.\n\n## open_existing(), because the tool must not create what it measures\n\n`open()` creates a database where there is none. Right for a store; wrong for an\ninstrument, and silently so — a mistyped path leaves a new empty database behind\nand is then reported as holding nothing.\n\nA caller cannot fix this from outside. Checking first and opening second has a\nwindow between the two, and open-or-create fills that window by creating. So the\nproperty lives where the claim is held: `open_existing()` takes the claim, asks\nunder it whether a database is there, and returns `database_not_found` having\ncreated nothing. A typed `open_intent` rather than a second bool beside\n`remove_existing`, where `open(path, false, false)` would say nothing about which\nfalse is which.\n\nThe window itself is a test rather than an argument: a seam removes the config\nimmediately after the claim is taken and before the open decides anything. With\nthe decision inside the claim the open refuses; had it been made before, it would\nhave said yes and then created a database over its own answer.\n\nAbsence is `database_not_found`; a database that is there and is wrong returns\nwhat is wrong with it, so a corrupt config is `config_file_corrupt` and never\nabsence. The directory is not created either. One exception, documented: taking\nthe claim creates `.utxoz.lock`, which is permanent by design — replacing it is\nexplained in database_lock as worse than keeping it. A lock file is not a\ndatabase and is never read as one, so a refused open leaves a directory that\nopen_existing() still refuses.\n\n`open()` keeps its behaviour for the callers that have it, and a case pins that\ntoo. The census tool now uses open_existing() and knows no file names at all: it\nreads the format through the library or not at all.\n\n## An entry that cannot be true\n\nThe first version took `min(actual_size, capacity)`. That is a silent\nnormalisation of corruption in the one tool whose purpose is to be believed: an\nimpossible length would be reported as a full entry and nothing would be said.\n\nThe config and the stamp certify identity and layout. Neither certifies that each\nentry is internally consistent. An entry whose recorded payload length exceeds its\nclass, a map claiming more entries than buckets, and any count that cannot be\nmultiplied without overflowing are all `entry_corrupt` — a new code, distinct from\nevery stamp error, which say the file is not ours or is not where it claims to be.\nThe census fails and there is no partial report. The diagnostic names the class,\nthe generation and the offending value, and never the key or the payload.\n\nUnchecked arithmetic gets the same treatment for the same reason: on file data it\ndoes not produce an obviously wrong number, it produces a plausibly small one.\n\n## Provenance is not scope\n\n`--snapshot` used to append a sentence to `scope`, which is an enumerated value a\nconsumer switches on. It now sets a separate `source` object —\n`declared_external_snapshot` and `consistency` of `live_database_exclusive` or\n`not_verified` — carrying its own note that this is the caller's declaration and\nthat nothing here can verify how a copy was taken. `scope` stays exactly\n`physical_stored` either way, and a test pins that.\n\n## Tests\n\nSixteen cases. The parts add up to the whole in every direction the report offers\n— histogram buckets to class entries, payload buckets to payload bytes,\ngenerations to classes, classes to totals, and the segment identity per\ngeneration. Reference reports exactly one class and counts its historical\ngeneration. A deleted entry stops being counted, because it stops being stored.\nThe JSON parses, is versioned, distinguishes zero from unavailable, and is\nidentical for the same state apart from the duration. Neither presentation\ncontains a key or a payload, checked with markers planted in both.\n\nAnd two refusals, which are the cases that matter: a generation that has gone\nmissing and a generation that will not open as a segment both fail the census. A\nreport short by one file looks exactly like a database with one file fewer.\n\nThe corruption cases run in both width classes, because `actual_size` is a\n`uint8_t` below 256 and a `uint16_t` above it and a check tried on one\nrepresentation has been tried once. Each has a control on either side: the census\npasses on the file as written, fails with one field edited, and passes again when\nthat field is restored.\n\nThe checks that only impossible inputs reach — a map with more entries than\nbuckets, a product that overflows — are reached directly rather than by forging a\nfile, because forging one is fragile and version-dependent, and an unobservable\ncheck is one somebody deletes as dead code.\n\nAnd the tool is tested as a process: it censuses a database, refuses one another\nprocess is holding and then succeeds once that process lets go, and refuses an\nempty directory while leaving it empty.\n\nMutations, each confirmed red: counting an entry twice; skipping a historical\ngeneration; writing an unavailable figure as 0; carrying on past a segment that\nwill not open; and taking `min(actual_size, capacity)` instead of refusing.\n\n## From review\n\nA historical generation is now stamp-checked exactly as the ordinary open checks\nit. A mapped segment is not yet a segment of *this* database, and a file renamed\ninto a name that is not its own was being counted as the generation it was\npretending to be.\n\n`finish_generation` re-checks that entries do not exceed buckets before\nsubtracting, and refuses rather than saturating: clamping to zero would turn an\nimpossible file into a plausible report, which is the same silent normalisation\nthis census refuses to do with payload lengths. A `static_assert` ties the\ncapacity the geometry publishes to the capacity the stored type actually has,\nsince the report carries one and computes from the other. Reference mode counts\nits generations per class instead of reading the walk's file total. The Windows\nblock measurement clears the error before the call, because INVALID_FILE_SIZE is\nalso a legitimate size. The tool's directory check uses the error_code overload,\nso \"the filesystem would not answer\" and \"there is nothing there\" stay different\nanswers. And the JSON cases parse the document instead of searching it: a\nsubstring check passes on output that is not JSON at all.\n\n## Packaging\n\nThe first run of this had four jobs red for one reason: the recipe exports\n`src`, `include`, `examples`, `tests` and `benchmarks`, and not `tools`, so every\nbuild from the exported sources configured a target whose only source file was\nnot there. Local builds never saw it — the file is in the working tree.\n\n`tools/*` is exported now, with a `with_tools` recipe option that defaults to on,\nso a consumer who installs the library gets `bin/utxoz_census` rather than a\nsource tree to build again. Verified by building the package the way the publish\njob does and looking inside it: `bin/utxoz_census` and `include/utxoz/census.hpp`\nare both there. `census.hpp` joins the umbrella header, which listed every other\npublic header.\n\nThe off path was run too, because it is the one that matters for\ncross-compilation: `-o '&:with_tools=False'` produces a package with the library\nand the whole API, `census.hpp` included, no `bin/`, no object compiled from\n`tools/census.cpp`, `UTXOZ_BUILD_TOOLS:STRING=False` in the cache, and\ntest_package still passing. The installed binary is built for the package's\ntarget and is not a host build-tool, so a consumer cross-compiling has a reason\nto turn it off beyond size.\n\n## An inspection creates nothing, including the class that has none\n\nThe first version of this door promised to create nothing and created a\nten-megabyte file. `open()` creates the active container of a class that has no\ngenerations, which compaction produces by draining one completely — an ordinary\nstate, not damage. So a database with one class full and another empty was opened\nfor measurement, given version zero of the empty class, and then censused: the\nreport carried a generation the census had just made.\n\nMeasured, before and after: `cont_3_v00000.dat`, 10485760 bytes, then nothing.\n\nThe empty class now stays absent under inspection and is censused as zero\ngenerations and zero entries. That is the policy rather than a repair, and it\ndecides the contract: an inspection supports `census()` and `close()` and refuses\neverything else with `inspection_only`, because there is no map for the rest to\nwork on and making one is the thing being avoided. Which is also why it is called\n`open_for_inspection()` now: `open_existing()` says which databases it opens and\nnot what may be done with the result.\n\nA config with no generations in any class is still `database_not_found` — not a\ndatabase with empty classes, a config with nothing behind it.\n\nThe test digests every file before and after, so \"unchanged\" is a statement about\ncontent and not only about names, and it carries the control that `open()` does\ncreate the class, which is the reason this had to be a second door.\n\nMutations: letting the inspection create the empty class, and dropping the\nrefusal of non-inspecting operations. Both turn the suite red.\n\n## A config with no generations is not a database\n\nopen_existing() promised to create nothing and created five segments. A directory\nholding a valid config and no data files — the shape a database has between its\nconfig being written and its first segment existing, and what is left when the\ndata is removed and the config is not — took the creation branch, made version\nzero of every class, and then reported a database holding nothing. True of the\nfiles it had just made; not of anything that was there.\n\nMeasured before the change and after it: five `cont_*_v00000.dat` created, then\nnone, with `database_not_found` instead.\n\nAll five catalogues empty, not any one: a class whose generations were all\ndrained by compaction is legitimately empty and gets version zero back on the\nnext open. Reference mode has the same cut, and a case for each, each with the\ncontrol that open() still does create them — which is why this had to be a second\ndoor rather than a change to the first.\n\n## From review\n\nThe doc and the tool's own header still said the tool opens with `open()`. They\nsay `open_existing()` now, and the note about it writing says what it does write:\nnot a database, but the lock file, and the settling of an interrupted merge in a\ndatabase that is there.\n\n`group_metadata_model` is checked arithmetic like everything else derived from a\nfile. A bucket count large enough to overflow it cannot reach it today, because\nthe caller multiplies by the slot size first and that fails long before — but\nthat is an argument about the order of two functions, and a reader should not\nhave to reconstruct it to trust the number. Tested at the boundary rather than\nonly at absurd values: the largest count that works, and the first that does not.\n\n## constexpr, and the compiler that was right\n\nMaking the metadata model checked left it `constexpr` over calls to\n`checked_add` and `checked_mul`, which were not. MSVC refused to compile it —\n`error C3615: constexpr function cannot result in a constant expression` — and\nMSVC was correct. A `constexpr` function that cannot be invoked in any constant\nexpression is ill-formed, no diagnostic required: MSVC diagnosed it, GCC and\nClang were entitled to stay silent, and they did, right up until something asked\nfor the value. Asked for one, GCC gives the same two errors.\n\nSo this is the fix rather than a way around it: the whole chain is `constexpr`,\nand two `static_assert`s ask for an answer at compile time — one for a value, one\nfor the refusal. Not because the values need testing, which a case already does,\nbut so that the next time this decays it decays on every compiler at once instead\nof on the one platform nobody runs locally.\n\n## The benchmark job's comment step\n\nIt failed with GitHub's own \"No server is currently available to service your\nrequest\", which turned a job that benchmarked correctly into a red one and said\nnothing about the benchmarks. Posting is not the measurement: the step is\ncontinue-on-error now. Nothing is lost when it fails — the same report is already\nwritten to the step summary — and the step still shows as failed, so a persistent\nproblem is visible rather than swallowed.\n\nSuites: 295 cases with statistics, 290 without. No change to geometry_id, the\nepochs, container sizes, capacities, file sizes, the rotation fraction, the\nfixtures, or anything persisted.",
          "timestamp": "2026-08-17T21:44:56+02:00",
          "tree_id": "0db35a6ac83c52b3e06381f64427eaa037d938e2",
          "url": "https://github.com/utxo-z/utxo-z/commit/2e39afa48a5e22debfd246ef76c519d21054c310"
        },
        "date": 1786996181846,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 339695.89,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 390654.15,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 396292.08,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 671262.38,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 453.09,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 496.84,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 13331909.5,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 26537411.92,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12962543.24,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 13285.37,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2264033.67,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2222569.47,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 111853.32,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 11957.42,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2593.69,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3373.14,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2995.29,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 1113.5,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 1113.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 1117.27,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 1093.01,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 1100.47,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 1048.67,
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
          "id": "9aeb84f9223ca22627e7b704aa96dd497a93ea3d",
          "message": "feat: measure how lookups are actually resolved, by class and by generation (#138)\n\nThe census counts what is stored. This counts what the read path does with it,\nwhich is the other half of what choosing a geometry needs: how often each class\nis asked, how often it answers, how many generation files a lookup costs, and how\nfar back the answer was found.\n\n## One cumulative level, not a switch per counter set\n\n    UTXOZ_STATISTICS_LEVEL = off | basic | lookup      CMake and Conan, default basic\n\n`off` compiles nothing. `basic` is what this process did — inserts, probes,\nresolutions, lifetimes — and is the default. `lookup` is `basic` plus the\nper-class read-path telemetry this commit adds. Cumulative, so the code asks\n`>= 1` or `>= 2` and there is one authority rather than a boolean per counter\nset; names rather than numbers, because a build log that says \"2\" says nothing.\nAn unknown value is refused by both CMake and Conan, with the list — a typo must\nnot quietly produce the default, or whoever asked for `lookups` reads counters\nthat were never compiled and finds zeros.\n\nThe old `statistics` boolean is gone rather than kept alongside it.\n\n## The width is part of the level\n\n`sharded_counters` is parameterised on its field count instead of carrying one\nwidth wide enough for the widest user. That was the mistake in the first version:\ngoing from 8 fields to 32 doubled every instance in every build, including builds\nthat never enable the widest user. Measured per open database:\n\n    off      7 bytes\n    basic    16 KiB   (two 8 KiB counter sets, exactly what they were before)\n    lookup   96 KiB   (those two, plus five 16 KiB per-class sets)\n\n## Three numbers that used to be one, and one that was never a class\n\n**A full-mode lookup is not addressed to a class.** The caller supplies a key,\nnot a size, and `find()` probes the active maps in class order until one answers.\nSo \"the class of a lookup\" exists only for a lookup that was answered, and an\nabsent one belongs to no class at all. `lookups_received`, `deferred` and\n`absent` are therefore global and say so in the output; everything else is per\nclass.\n\n**The depth that existed was a version distance, not an order of search.**\n`current_versions_[Index] - version` is how far back a generation is numbered.\nCompaction removes generations, so the numbering has gaps, and the file cache is\nsearched before the catalogue, so the order of the search is not the order of the\nversions either. This now carries both, named for what they are:\n\n    probe_ordinal      how many generation files a key was searched in — the cost\n    version_distance   how far back the answering generation sits — the age\n\nA test pins them apart rather than arguing they differ: the same key, two sweeps.\nCold, it is found in the second file searched — ordinal 2, distance 2. Warm, the\ncache puts that file first, so it is found in the first — ordinal 1, distance\nstill 2. One number could have said the search got cheaper or that the data got\nyounger, and those are different facts.\n\n**Files visited were counted per sweep, not per key.** A thousand keys across\nthree files recorded three. `generations_probed` is now per key, so a key nobody\nfinds is charged for every file it was searched in — the sum is exact, and it is\naccumulated once per file as `pending.size()` rather than once per key, so the\ninner loop touches no atomic.\n\n## Where it counts\n\n    full_find, find_in_latest_version   where the search stopped, and which class\n                                        answered\n    full_resolve, reference_resolve     per file: opened, cache hit, and one\n                                        probe per pending key\n                                        per answer: ordinal and version distance\n    published                           once per sweep per class, and only when\n                                        the sweep completed — an abandoned\n                                        attempt is retried and must not be\n                                        counted twice\n\n`active_maps_probed` is **derived**, not counted: the suffix sum of \"the search\nstopped here\" over this class and the ones after it. That is not tidiness. The\nfirst version incremented a counter per class per lookup and measured **+160%**\non a key that misses everywhere — five atomic increments on five cache lines for\none lookup. One increment and a suffix sum give the same numbers for 36%.\n\nThe `std::vector<uint32_t>` of depths the sweep used to allocate is gone; the\nhistogram is fixed-size and on the stack.\n\n## What it costs, isolated\n\nStatistics on against statistics off answers a different question: it measures\nevery counter in the build, and two of the three sets existed before this. So the\nmeasurement is by level, on the same head, plus the parent for reference. Seven\ninterleaved rounds, medians in nanoseconds:\n\n                                       parent      head        head        head\n                                        basic       off       basic      lookup\n    active hit, first class             38.47     31.12       35.02       38.80\n    miss, every class probed            17.53     13.82       17.81       17.83\n    sweep of 256 keys / 3 generations  14 954    13 116      13 013      13 298\n    mixed, 9 active hits to 1 historical 532.2     451.5       526.2       576.2\n\n**The telemetry costs +10.8% on a find that hits and nothing on one that misses.**\nNot the +42% first reported: that was statistics on against off, and most of it\nwas pre-existing.\n\n**The default level got faster.** head `basic` against parent `basic`: -9.0% on a\nhit and -13.0% on the sweep. Two reasons, both of them counters that stopped\nbeing counted:\n\n  - `probes` is `answered + deferred`, so the third increment on the hot path was\n    arithmetic all along;\n  - `active_maps_probed[i]` is `(answered by class i or after) + deferred`,\n    because the search stops at the first class that answers. Counting it\n    directly measured +160% on a lookup that misses everywhere; counting only\n    where the search stopped was still an atomic for a number addition had.\n\nThat second one is why a miss now costs nothing at `lookup`: there is no counter\nleft on that path.\n\n**And at `lookup` a hit costs what the parent's `basic` cost** — 38.80 against\n38.47, inside the spread. The telemetry is paid for by what the derivation saved.\n\n**Compiled out, this head is the parent**: head `off` against parent `off` is\nwithin 2% on every workload, by minima; that column's medians are contaminated by\none slow round and are not used.\n\n**Resolution.** Median and minimum agree to within 0.4–3.4% in every cell except\nthe contaminated one, so a difference of five per cent or more is real and\nanything under it is not. The sweep's +2.2% at `lookup` is at that boundary; the\nmixed workload's +9.5% is above it.\n\nNo threshold is proposed. One optimisation was tried and reverted: folding a\nhit's two increments into a single shard lookup made no measurable difference.\n\n## Without statistics\n\nEvery counter, every tally field and every recording call is inside\n`UTXOZ_STATISTICS_ENABLED`. So `basic` keeps the sweep improvement — the `std::vector<uint32_t>` of depths\nis gone at every level — and compiles none of the telemetry.\n\nWhat remains at every level is the shape of the report: the types exist,\n`get_statistics()` and `to_json()` work, and `statistics_level` says which kind of\ndocument it is. At `basic` the per-class list is **empty** rather than five rows\nof zeros, which would read as a database nobody queried. One test asserts all of\nthat from the same source at every level, so they cannot drift.\n\n`rehashes_observed` is untouched. It is a safety counter compiled into every\nbuild and it does not belong with telemetry.\n\n`rotations_per_container` is also untouched, and still reports the active version\nnumber rather than a count of rotations. Correcting it needs a real counter plus\ntests over rotation, compaction and renumbering; it is a known debt, out of scope\nhere, and deliberately not papered over.\n\n## Tests\n\nFifteen cases, every one asserting exact numbers. A counter that is merely\nnon-zero proves only that something incremented it.\n\nAn active hit in the first class and in a later one — the second showing the four\nclasses before it being asked and the fifth not. A miss asking all five. A key in\nthe newest historical generation at ordinal 1, one in the oldest at ordinal 2, an\nabsent key charged for both files. A sweep of three keys over two files counting\nfive probes, which is the case that tells \"generations probed\" from \"files\nopened\" — with one key in a batch they are the same number. Two classes counted\napart. Repetition moving each counter exactly seven times. Reference reporting one\nclass labelled reference, with its history. Cold and warm cache. Compaction, where\nthe histograms must still account for the key exactly once.\n\nMutations, each confirmed red: counting a lookup twice; omitting the active\ngeneration; omitting a historical generation; attributing the answer to the wrong\nclass; recording an absent key as found; confusing generations probed with files\nopened; and using the version distance as if it were the ordinal.\n\nThe counters are not free in memory either, and an earlier version of this said\nthey were on the grounds that a shard is padded to a cache line anyway. That is\nwrong: eight fields are 64 bytes and fit one 128-byte shard, thirty-two are 256\nand take two, so an instance goes from 8 KiB to 16 KiB. A full-mode database now\nholds seven of them — probes, resolutions, and five classes — which is 112 KiB\nagainst the 16 KiB the two older sets took. About 96 KiB more per open database,\nonce, not per operation.\n\nSuites, all three levels: **310 at `lookup` with nothing skipped, 310 at `basic`\nand 305 at `off` with 26 skipped** — the cases that assert exact counts say\nnothing when the counters are not compiled, and reporting them as passed would\noverstate the suite's coverage.\n\nCI covers the three levels, the two platforms beyond Linux for `lookup`, and both\nrefusals: CMake and Conan each rejecting a level they do not know.",
          "timestamp": "2026-08-18T10:36:44+02:00",
          "tree_id": "a8d1cdac32de2855beeeb1c46a2a0e50f4b02979",
          "url": "https://github.com/utxo-z/utxo-z/commit/9aeb84f9223ca22627e7b704aa96dd497a93ea3d"
        },
        "date": 1787042450896,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 809808.5,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 809051.51,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 862199.97,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1024248.89,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 605.53,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 545.43,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 16461359.25,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 33139714.12,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 15878170.3,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 16187.62,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2866297.47,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2944256.76,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 141770.51,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 15001.34,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2685.26,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3853.92,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3827.58,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 1313.46,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 1308.38,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 1320.39,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 1276.46,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 1282.8,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 1280.8,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: active hit, first class",
            "value": 16727273.43,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: miss, every class probed",
            "value": 33462528.75,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: sweep of 256 keys over three generations",
            "value": 40113.77,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: mixed 9 active hits to 1 historical",
            "value": 1095309.71,
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
          "id": "466f19f7ad8e79b87655f6b4d2f9a1e2a2aea55c",
          "message": "feat: verify that no outpoint is stored more than once, and answer with a verdict (#139)\n\nA correct UTXO set holds each outpoint at most once, across every class and every\ngeneration. Nothing in the write path enforces that, so whether a given database\nsatisfies it is a question that has to be asked of the files.\n\n`verify_unique_outpoints()` asks it.\n\n## Why this is not a census\n\nAn earlier draft of this counted distinct outpoints as a second `census()` scope\nand described the duplicates it found — a multiplicity histogram, four\noverlapping shape totals, an exclusive breakdown by flag combination. All of that\nwas built on the premise that duplicated keys are a population worth describing\nstatistically.\n\nThey are not. They are corruption, and the report a broken database deserves is a\nverdict and a place to start looking, not a distribution. The taxonomy is gone,\nthe scope is gone, and `census_report` is byte-for-byte what it was: same fields,\nsame `schema_version`, same JSON.\n\nWhat is kept is the part that was never in question — the bounded walk, the\npartitioning, the budget — because verifying uniqueness postmortem needs exactly\nthat walk whatever is later decided about enforcing it at insert or during\ncompaction.\n\n## A verdict, not an error\n\nA database with duplicates is not a database this can fail to read. The walk\nfinishes, every figure it reports is exact, and the answer is `unique == false`.\nThere is no `error_code` for a duplicate and `entry_corrupt` is not reused for\none: *the verification could not be completed* and *the verification completed and\nthe answer is no* are different facts, and only the first is about the instrument.\n\n`utxoz_verify_unique` puts that in its exit status — 0 no outpoint is stored more\nthan once, 2 some outpoint is, 1 could not tell — which is why it is a second\nbinary rather than a flag on the census.\n\nThe invariant is \"at most one copy\", and the wording says so. \"Every outpoint is\nstored exactly once\" was in four places and was wrong in all of them: an empty\ndatabase stores no outpoint at all and satisfies the invariant, which a test\nasserts — so the text contradicted the suite. A census that started returning 2 because of what it found would break\nevery caller that reads a non-zero status as \"the tool failed\".\n\n## It still does not choose a winner\n\nFor a key whose copies disagree there is no \"the value of this key\": the store has\nno precedence across classes and, in history, which copy answers can depend on the\nfile cache (issue #136). This reports how many copies exist and where they are,\nand stops. Repair is a rebuild from the blocks into a new directory, verified and\nthen swapped in — not a choice made here.\n\n## Outpoints are withheld unless asked for\n\nAn outpoint identifies an output on a public chain. By default a finding gives the\nmultiplicity and the locations — class, generation, whether the generation is\nactive — and not the key, so a report can be pasted into an issue without a second\nthought. `include_outpoints` puts them in, for whoever is going to look at the\nentries.\n\nNo substitute form. A truncated hash would be neither private — it is reversible\nagainst the chain — nor useful, since it cannot be fed back to `find()`.\n\nThe document says which it is (`\"outpoints_included\": false`) rather than leaving\nit to be inferred from an absence, because a report with no findings and one with\nthe outpoints withheld would otherwise look the same.\n\n## The sample is bounded, and so is what holds it\n\n`keys_with_multiple_copies` is the count; the findings are a sample of at most\n`max_findings` keys locating at most `max_locations_per_finding` copies each,\ndefaults 16 and 16. Both omissions are reported, so a truncated sample never reads\nas a complete one. Without the caps, one key with a million copies would be a\nmillion lines.\n\nThe sample is collected in two flat vectors metered by the same allocator as the\nrecords, each reserved exactly once so that nothing grows and no two blocks are\never alive at the same moment, and converted into the public `duplicate_finding`s\nafter the walk, when the records are no longer held.\n\n**The returned report is outside the budget, and the contract says so.** Claiming\notherwise would mean claiming that `reserve(n)` allocates exactly n — the standard\npromises *at least* — and evidence from three implementations is not a structural\nproperty. What is bounded instead is the shape: at most `max_findings` findings of\nat most `max_locations_per_finding` locations each, published as\n`report_bytes_estimate`, and never growing with the database. The dangerous part,\nthe part proportional to what is stored, is inside the meter; the report is small,\ncapped, and lives after the walk.\n\nDeclaring the final size and filling the public vectors directly — which is what\nthis did first — is not the same promise. It accounts for neither geometric\ngrowth, nor a `capacity()` larger than what was asked for, nor the moment during a\nreallocation when the old block and the new one are both alive. With\n`max_findings` at 17 a plain vector ends up holding 32 having briefly held 16 and\n32 together, against 17 accounted for; the report would have said the ceiling was\nhonoured while the operation held more, which is the exact property the metered\nallocator exists to provide. A case now runs 1, 16, 17 and 33 findings and pins\nthe composition, and another fills a sample of 17 for real.\n\n## The budget is enforced, not described\n\nThe first version of this computed what it expected to hold and reported it. That\nis a description, and two things follow from the difference:\n\n  - a container allocated beside the records would not appear in the figure, so\n    the report could say the ceiling was honoured while the process held\n    considerably more. The contract said \"budget\"; the code delivered an estimate;\n  - a reservation known to be too large was refused after being described, which\n    on a real database is the difference between a diagnostic and a `bad_alloc`.\n\nNow the walk holds its records in a `counted_vector`, and it is the allocator that\nasks the meter. A request past the ceiling is refused **before the system is asked\nfor the memory**, and `estimated_peak` is the most the meter ever had outstanding\n— observed, not predicted.\n\nWhat the ceiling covers is a list pinned by a test rather than a claim, and every\nentry but one is a real allocation the meter saw: the planning counters, the group\nlist, the sample, and the records. The exception is the sort's allowance\n(1 048 576), because introsort's own stack has no allocator to route through.\n\nThe counters and the group list were plain vectors under a hand-computed allowance\nof 192 KiB. The worst case they can actually reach — a reallocation holding two\nblocks at once — comes to **196 608 bytes against an allowance of 196 608**. It\nheld, by nothing but one allocator's growth factor, and the `static_assert` that\nwas supposed to guard it compared the *final* sizes rather than the transient. They\nare metered now, and the group list is reserved to its exact maximum of one group\nper prefix so that it never grows: 96 KiB always, half what the allowance set\naside, and a figure the meter knows rather than one somebody has to keep right.\n\nThe headroom is held back rather than subtracted in a report — the meter's ceiling\nis the budget without it, so the walk cannot reach it.\n\n`files_revisited` is `generations_visited` now, because that is what it counts: the\nactive generation of a class is already mapped and is walked without opening\nanything, so the old name overstated the I/O by every active generation. On the\nmeasured database the visits are five times the opens.\n\n`duplicate_group` can only be built from metered storage. A copy of a group would\ntherefore have to be metered too, which is what stops the old bug returning as\nsomebody's convenience — and a copy into a default-allocator vector does not\ncompile at all.\n\nOne meter serves the whole operation rather than the engine alone: the walk takes\nit rather than making it, so `fixed_overhead` is what the meter actually holds\nbefore the records rather than a sum of the terms somebody remembered.\n\nA release that does not match an acquisition leaves the meter **unbalanced**, and\nan unbalanced meter authorises nothing further. It used to clamp to zero, with a\ncomment saying that clamping would hide the defect — which is what clamping did:\nit hid the mismatch and then handed out memory against a figure that had stopped\ndescribing what was held.\n\nRefusing later acquisitions is not enough on its own: a mismatch in the *last*\ndeallocation of all has nothing after it to refuse. So the metered containers live\nin a scope of their own and the books are checked once every one of them has been\ndestroyed, in the walk and in its caller both. A meter that lost track did not\nhonour a ceiling, and there is no verdict to give on that basis.\n\nIt is not a sandbox: code that calls the global allocator directly is outside any\nin-process budget, and doc/uniqueness.md says so.\n\n## Exact, bounded, and with no temporary file\n\nUnchanged from the walk this replaces, and restated because it is the substance:\n\n    1. a planning pass counts entries per 12-bit prefix — counters only;\n    2. prefixes are grouped so each group's records fit the budget, from the\n       counts just observed rather than an estimate;\n    3. each group is walked, collected, sorted by key, grouped;\n    4. a prefix that cannot fit is refused with the budget it would have needed.\n\nNothing is written. No temporary directory, no cleanup path, no residue on any\nfailure path, because nothing is created. The partition hash is ours — FNV-1a with\na splitmix finaliser — deliberately not the map's, which `hash_epoch` pins. A\ncollision is never an equality: the hash chooses the group, membership is a\n`memcmp` of thirty-six bytes.\n\n**The cost is re-reading, and it is reported rather than implied.** Measured on a\n500 005-entry database (500 000 distinct outpoints, five stored twice), 1.3 GB on\ndisk, warm cache:\n\n    256 MiB (default)  1 planning + 1 data pass    1.0 M examined    219 ms\n    16 MB              1 planning + 3 data passes  2.0 M examined    344 ms\n    4 MB               1 planning + 12 data passes 6.5 M examined    856 ms\n\nSame verdict and same counts from all three; the cost is linear in the partitions.\n`estimated_peak` came to 56 028 721, 15 998 600 and 3 999 952 bytes against those\nbudgets — inside each. `report_bytes_estimate` was 5 392, 5 408 and 5 480: it moves with the number of\npasses, which the budget bounds, and not with the database. It is an estimate\ncomputed from the requested element counts and is **not** described as an upper\nbound — `reserve` promises at least what it was asked for, so excess capacity and\ntransients are the standard library's. `duration_per_pass_ms` is in both renderings, because a\nsmall budget re-reads the whole database once per group and where the time went is\nwhat says whether to raise it. Resident memory was 1.38, 1.37 and 1.35 GB, dominated by the\ndatabase's own mapped pages, which no budget here can bound. doc/uniqueness.md says\nso rather than letting a reader take the figure for what `ps` will show.\n\n## Tests — 45 cases, exact numbers\n\nThe verdict on a clean database and on an empty one. Duplicates across\ngenerations of one class; across two classes; across both at once, with the\nlocations asserted in each dimension; historical-only; five copies of one key. A\ndeliberate partition collision, found by search rather than hard-coded, counted as\ntwo keys.\n\nThe sample: no outpoint anywhere in the JSON or the text without the opt-in, and\nthe real key with it; `max_findings` and `max_locations_per_finding` honoured while\nthe counts stay uncapped; a sample too large for the allowance refused, and an\noverflowing one too. The verdict with `max_findings = 0`, where there is no sample\nat all and the answer is still no.\n\nThe budget: the meter itself, refusing at the boundary and taking nothing when it\nrefuses; a `counted_vector` past the ceiling throwing without reaching the\nallocator; the composition of the fixed overhead, term by term; a small budget\nforcing 13 partitions and reporting identical figures; the peak inside the budget\nand equal to its parts; a budget too small, refused **before any collecting pass**\n— asserted on the segment-mapping counter, so \"before the work\" is a measurement\nrather than a comment; a pass limit; a prefix that cannot be split; a sample the\nbudget cannot hold, and the same sample against a budget that can. Determinism\nacross a warmed file cache. An unreadable generation, and a failure during the\ncollecting passes, each producing no report and no verdict.\n\nA latched store: refused with `recovery_required` rather than judged, while the\ncensus on the same object still answers.\n\nThe tool: exit 0 on a unique database, exit 2 with duplicates — with the outpoints\nwithheld and then, with the flag, present — and exit 1 for a missing directory, a\ndirectory holding no database, a budget that cannot hold the work, and a malformed\noption. `--text` producing something that is not JSON, and `--mode=reference`\nreading a database the default mode cannot. An argument that begins with a dash\ntreated as an option rather than as a directory. A report that could not be\nwritten in full answering 1 rather than a verdict (Linux only: /dev/full is what\nmakes a write fail on demand, and the behaviour it pins is not platform-specific\neven though the way to provoke it is). And the help text carrying the defaults\nthat `verify_options` actually holds.\n\nAnd that the census neither answers this question nor changed to ask it.\n\n## A verdict is not rendered on a latched store\n\n`census()` does not consult the recovery latch and this does, which is a\ndeliberate difference rather than an inconsistency. A latch means an operation\nthat may have applied part of its work did not finish, so a merged generation can\nsit beside the sources it was built from. Counting that is honest — two copies are\ntwo copies, and the census says what is stored. Rendering a *verdict* on it is\nnot: it would report the database as violating uniqueness when what it is looking\nat is a compaction that was interrupted. That is a false accusation about\nintegrity, and it is the kind that gets acted on.\n\n## The narrowing guard is checked where 32-bit code is built, not where it is not\n\nThe counts are `uint64_t` and `reserve` takes a `size_t`, so on wasm32 a count above\n2^32 narrows on the way in and `reserve` succeeds at a size nobody asked for.\n\nA case written as `if constexpr (sizeof(size_t) < 8)` would have compiled there and\nexecuted nowhere: the wasm job builds the library and runs no `ctest`, and this\nsuite could not run there in any case — it needs a real filesystem and\nmemory-mapped files. \"The case exists\" is not coverage if no job runs it.\n\nThe record count of a partition group narrows through the same cast and is guarded\nthe same way.\n\nFitting `size_t` is not fitting a vector, and that gap is reachable from the\ncommand line rather than from a damaged database — on libstdc++, which caps\n`max_size()` at `PTRDIFF_MAX / sizeof(T)`: about 1.28e17 for a `sampled_finding`,\nagainst an overflow check that admits counts up to 2.56e17. Between them lies a\nwindow where `--max-findings` narrows cleanly and `reserve` answers with\n`length_error`. libc++ caps at `SIZE_MAX / sizeof(T)`, where the two coincide and\nthe window is empty — which is why the case asserts the contract (a count a vector\nwill not hold comes back as `insufficient_space`) rather than which guard refuses\nit. Every reservation is now checked against the vector's own\n`max_size()` first, for the diagnostic, and `length_error` is caught as a backstop\n— a reservation a vector will not make is a refusal this API owes its caller, not\nan exception crossing the library boundary.\n\nSo the width is a parameter of the primitives, and **production calls wrappers that\ntake no width at all** — the freedom to pass the wrong one is removed rather than\nreviewed. `count_fits_addressable(count, addressable)` and\n`sample_fits_addressable(findings, locations, addressable)` are driven from Linux at\n16, 32 and 64 bits; `count_fits_platform` and `sample_fits_platform` are what the\nwalk calls. Both are pinned by `static_assert`, and under\n`#if SIZE_MAX < UINT64_MAX` the assertions are about the wrapper itself, which only\na 32-bit build evaluates.\n\nTwo mutations of the primitive **do not compile**: the boundary is checked by the\ncompiler on every platform this ships to. A mutation that deliberately bypasses the\nwrapper and calls the primitive with a 64-bit width survives on a 64-bit build,\nwhere the two are the same value — as any deliberate bypass of any abstraction\nwould. What changed is that it is now a bypass rather than a slip.\n\n## Mutations\n\nForty-five applied: thirty-six red, three that no longer compile, six that\nsurvive.\n\nThe published figure in the previous revision of this message — \"eighteen applied,\nseventeen red\" — was wrong, and wrong in both directions: it undercounted what had\nbeen run and the parts did not add up. These are counted from the runs.\n\nThe two that were the reason the budget changed are red now:\n\n  - **materialising a duplicate group into a second vector** — metered, so the\n    peak rises and the identity between the peak and its parts breaks. The same\n    copy into a default-allocator vector **does not compile**: `duplicate_group`\n    can only be built from metered storage;\n  - **removing the pre-pass group validation** — caught on the segment-mapping\n    counter, which is what makes \"refused before the work\" observable rather than\n    asserted in a comment.\n\nThe rest, each red: the verdict taken from the sample instead of the counts; a\nduplicate counted as a distinct key; grouping by partition prefix instead of by\nkey; historical generations skipped; the class not recorded on a copy; the pass\nlimit ignored; a fit predicate that always accepts; a partial report when a pass\nfails; `include_outpoints` ignored; `max_findings` ignored;\n`max_locations_per_finding` ignored; the sample not declared to the walk; the\nbookkeeping left out of the ceiling; a meter that never refuses; the headroom\nhanded out instead of held back; the recovery latch not consulted before a\nverdict; the tool exiting 0 whatever the verdict; a failure to open reported as a\nverdict; a failed walk reported as not-unique; the tool's `--mode` ignored;\n`--text` producing JSON anyway; a dash-prefixed argument taken for a directory;\nthe result of writing the report ignored; the help text restating a default\ninstead of reading it; the sample collected in the public vectors again; the\nsample reserved geometrically instead of once; the report's own storage not taken\nfrom the ceiling; an unbalanced release clamped instead of failing closed; the\nper-pass durations left out of the JSON.\n\nSix survive, and none is a coverage gap:\n\n  - reporting the peak as `record_capacity × record_bytes + fixed_overhead +\n    headroom` instead of what the meter observed. While nothing else is metered\n    the two are equal, which is exactly what the identity test asserts. The reason\n    to prefer the meter's figure is that it stays right if something else ever\n    *is* metered — and the mutation guarding that is the group copy above, which\n    is red;\n  - removing the check that the collecting pass found what the planning pass\n    counted. Nothing can make the two disagree while the exclusivity contract\n    holds, so no test reaches it — the same category as the arithmetic guards in\n    census_arithmetic.hpp, and the code says so where it sits;\n  - removing the balance check after the metered containers are destroyed. Nothing\n    in the production path unbalances the meter, so no test can reach it through\n    the walk; the mechanism it depends on is unit-tested directly, including the\n    case where the mismatch happens with nothing left to acquire;\n  - the call site choosing a 64-bit width instead of this platform's. On a\n    64-bit build the two are the same value, so nothing can observe it. The width\n    is a named constant so that the wiring is one reviewable word, and everything\n    the guard *decides* is checked below.\n\nA second limit, unchanged: a mutation comparing the full 64-bit hash instead of the\nkey survives, because no test can produce a 64-bit collision — the birthday bound\nis 2^32 keys. The realistic form of that bug, grouping by the prefix, is caught,\nand the comparison itself is a `memcmp` of thirty-six bytes read directly.\n\nSuites: 352 cases at the default statistics level, 347 with statistics off.\n\n## Out of scope\n\nNo geometry, capacities, file sizes, rotation threshold, compaction, precedence\npolicy, issue #136, `find()` API or container selection. Where the invariant\nshould be enforced — at insert, during compaction, postmortem, or several of\nthose — is not decided here; a postmortem verifier is needed under every one of\nthose answers, and this is it. Comparing the contents of duplicate copies is\ncancelled: under this invariant there should be no copies to compare.",
          "timestamp": "2026-08-19T00:42:16+02:00",
          "tree_id": "8b910c54d00bdba15178e2fa0bf41c5c6c6666f9",
          "url": "https://github.com/utxo-z/utxo-z/commit/466f19f7ad8e79b87655f6b4d2f9a1e2a2aea55c"
        },
        "date": 1787093226435,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 244948.92,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 262963.27,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 427755.61,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 287728.38,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 330.08,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 343.77,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 12478282.03,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 25661230.16,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 12123599.91,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 12036.88,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2034335.05,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2347927.83,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 84457.75,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 10866.38,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2208.18,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3089.07,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 2205.97,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 980.5,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 956.25,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 1011.59,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 1004.23,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 1017.6,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 1010.08,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: active hit, first class",
            "value": 12954845.92,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: miss, every class probed",
            "value": 25969045.47,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: sweep of 256 keys over three generations",
            "value": 31153.33,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: mixed 9 active hits to 1 historical",
            "value": 857689.45,
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
          "id": "fff255837299f9b1a41c6191d3b9863e703ac83d",
          "message": "fix: rotate before Boost decides to grow, and verify what a failed insert left (#140)\n\n* fix: rotate before Boost decides to grow, and verify what a failed insert left\n\nA real initial block download logged this twice, on the 44-byte class:\n\n    [error] [UTXO-Z] Error inserting into container 0: boost::interprocess::bad_alloc\n\nBoth times the store rotated, retried, and carried on; the download finished,\ncompacted, and a uniqueness walk over the 59 125 454 entries it produced found\nno outpoint stored twice. So nothing was lost. But `error` was the wrong level\nfor an event the store recovered from, and — more to the point — the event\nshould not have happened at all.\n\n## Why the guard was late\n\nThe store rotates when a map holds `max_entries_for(bucket_count)` entries: a\nfixed fraction of capacity, chosen to sit below the point at which Boost would\ngrow the map. `capacity_policy.hpp` asserts exactly that, and the assertion is\ntrue of a *new* map.\n\nBoost's threshold is not fixed. `foa/core.hpp::recover_slot` lowers it on erase:\n\n    /* If this slot potentially caused overflow, we decrease the maximum load\n     * so that average probe length won't increase unboundedly in repeated\n     * insert/erase cycles (drift). */\n    size_ctrl.ml -= group_type::maybe_caused_overflow(pc);\n\nOn a workload that erases nearly as much as it inserts, that walks Boost's growth\npoint down toward the operating threshold and past it. Then the store believes it\nhas room, Boost decides to grow, and the segment — sized so that the map fills it\n— has 67 MB free against a bucket array of 1.32 GB. The allocator refuses.\n\nClass 0 took 398 800 611 inserts against 354 992 550 erases over 13 generations.\nThe two thresholds start 688 128 entries apart, so about 2.5 % of the erases in a\ngeneration having touched an overflowed group is enough to cross.\n\nThe first test in `test_insert_transition.cpp` reproduces the drift on a real map\nwith no failpoint at all. It needs two conditions that are easy to miss and are\nwritten down there: the map has to be nearly full, and the keys have to look like\ntxids. Sequential keys spread so evenly that nothing overflows and the threshold\nnever moves — the first version of that test churned four million of them and\nmeasured no drift whatsoever.\n\n## What changed\n\n**The guard asks the map, not the policy alone.** The limit is now\n\n    min(max_entries_for(bucket_count), map.max_load())\n\n`max_load()` is public and `noexcept`. The off-by-one comes from\n`foa/table.hpp::emplace_impl`, which grows when `size >= max_load()`, so a map\none entry short is still safe — the same shape the operating threshold already\nhad, which is why the two combine by taking the smaller.\n\n**A key already in the active map no longer rotates.** This is a hole that was\nalready open: the guard rotated without asking whether this insert would insert.\n`emplace` looks a key up before it grows, so a key that is present never grows the\nmap — but rotating on it sealed the generation holding it and put a second copy in\nthe empty one. Two copies of one outpoint, made by the guard meant to keep the\nfile healthy. It never fired in the real download, and it is closed now with one\nprobe, taken only on the path that has already decided to rotate. It closes the\nactive map's half of the question and no more.\n\n**The `bad_alloc` path verifies before it retries.** It reads size, bucket count\nand the live growth point before the `emplace` and again after the throw, asks\nwhether the key is there, and classifies:\n\n  - unchanged and the key absent — the guarantee held. `info`, one rotation, one\n    retry, and a second `info` when the retry succeeds;\n  - anything else — the container contradicted what `emplace` documents.\n    `critical`, the instance latches, `entry_corrupt`, no rotation and no retry;\n  - a second failure on a generation created moments ago — capacity or\n    configuration, not corruption. `error` and `insufficient_space`.\n\nBoost's guarantee is quoted where it is relied on: the new arrays are allocated\nbefore the `try`, deleted and rethrown if the element cannot be built, and the\nsize is incremented only after the rehash completes. The documented carve-out is\nthe hash function, and `outpoint_hash::operator()` is `noexcept`.\n\n**One rotation and one retry, where there were three.** On a segment that cannot\ntake a single entry the old loop made three files before giving up — up to 4.2 GB\nof empty generations for class 0.\n\n**A rotation that fails now latches too.** `new_version` closes the previous\ncontainer before it makes the next one, so a failure leaves `containers_[Index]`\nnull and `container<Index>()` dereferences it. That is not corruption — the data\nis intact and a new instance can open it — but this instance has nowhere to put\nthe next entry of that class, and finding that out by dereferencing nothing is\nnot an option. It answers `file_open_failed`.\n\n## The latch is not a recovery\n\n`cleanup_pending_` means a merge published its target and could not retire what it\nsuperseded: there is a sidecar on disk and the next open settles it, so\n`recovery_required` is honest. This is not that. A map that moved under a\n`bad_alloc` has no sidecar, no merge record and nothing to rebuild from, and the\nlatch is a bool in memory that a restart erases — after which `open()` maps the\nsame file and uses it, because `validate_stamp` certifies the format identity and\nnot the coherence of what the file holds.\n\nSo the latch carries the code it answers with, `entry_corrupt` here, and the log\nsays to stop and verify or rebuild rather than to restart. Making the quarantine\nsurvive a restart is a persisted mark with its own format and its own question\nabout who may lift it; that is a separate piece of work.\n\nIt blocks the instance, not just the two inserts: inserts, finds, resolutions,\ndeletes, compaction, traversals, census and the uniqueness walk. `sync()` and\n`census()` consult the integrity latch and deliberately not the recovery one —\na census over a half-applied compaction is honest, and flushing a map that is not\nwhat it claims is the one thing not worth doing.\n\n## Counters\n\n`preventive`, `capacity_exception`, `failed` and `unexpected_post_exception`, per\ncontainer, separate from `rehash_count` because a rotation is not a rehash.\nPresent in every build including one compiled with statistics off: they are a\nhandful of rare operational transitions, and the reason they exist is that the\ninvestigation into the first one had to be reconstructed from a log line carrying\nnothing. `failed` completed no rotation and is not part of the total;\n`unexpected_post_exception` rotates nothing at all.\n\n## Tests — 14 cases\n\nThe drift itself, on a real map, without a failpoint. The same drift end to end:\na map driven below its growth point, reopened through the public API, where the\nnext insert of a new key rotates before Boost can grow it — that case goes red the\nmoment the guard stops consulting `max_load()`, which is what makes the guard\ntested rather than merely asserted.\n\nThe effective limit at both thresholds. The classifier on each of its inputs. A\nkey already present at the rotation threshold: no rotation, `false`, the\ngeneration count unchanged, and one copy — asked of `verify_unique_outpoints()`\nrather than inferred. The same in reference mode. A new key at the threshold,\nwhich does rotate and is counted as preventive.\n\nThe exceptional path is reached from a seam rather than by filling a segment\nuntil the allocator happens to refuse: that depends on a geometry, takes a hundred\nthousand inserts, and cannot say which insert failed. `fail_insert_emplace` makes\nexactly the next N inserts throw, so each branch is asserted exactly — one\ncapacity-exception rotation, one new file, a successful retry, one physical copy\nand no map that grew; a second failure refused with `insufficient_space` without a\nsecond rotation or a second file; and, with `fail_insert_after_mutating`, the state\nthe container promises cannot happen, which latches the instance and is then\nrefused by insert, find, resolve, deletes, compaction, sync, traversal, census and\nthe uniqueness walk alike. A rotation that cannot make its file leaves no file and\nno catalogue entry, latches with `file_open_failed`, and a new instance opens what\nwas already there. Reference mode covers the same three outcomes.\n\nSuites: 361 cases at `lookup`, 361 at `basic`, 356 with statistics off.\n\n## What the benchmark against the parent found\n\nIt found a regression I had put there. The diagnostic captured the segment's free\nbytes before every `emplace` — and `can_insert_safely()` already walks the free\nlist on every insert to get the same figure. Two walks instead of one, on a list\nwhose length is not O(1): `insert P2PKH (43B)` went from about 30 ns to 61 ns.\nThe guard now hands the value it already paid for to the caller, and there is one\nwalk again.\n\nAfter that, the aggregate benchmarks over four alternating runs of each binary,\ncomparing minima:\n\n    bulk insert 10K (P2PKH)      -2.1 %\n    bulk insert 10K (chain mix)  +1.4 %\n    simulated IBD (100 blocks)   +0.8 %\n\nThose average ten thousand inserts each, which is what a per-insert change needs.\nThe single-insert micro-benchmarks are not usable here and the numbers say so: the\nparent measured 58.04 ns and then 78.74 ns for `insert 123B` on the same binary,\na 36 % spread, and across runs they disagree in direction — +5 % on P2PKH against\n+121 % on P2SH, which do the same work in the same container. The largest deltas\nanywhere in the suite are -8.5 % to +2.4 %, and the improvements outnumber the\nregressions, which is what noise looks like.\n\n## Not here\n\nNo geometry, capacity, file size or epoch changes. No persisted quarantine. The\nobservation that `can_insert_safely()` calls `get_free_memory()` on every insert,\nwhich walks the segment's free list, is recorded as separate performance debt.\n\n* fix: keep the failure diagnostic from overruling the failure\n\nFour corrections to the recovery path, all on the same theme: the log line\nthat explains a failed insert must not be able to change what that insert\ndecided, and must not name things it inferred.\n\n- The free-memory probe is read inside the `catch` whose remaining work is\n  to classify the map, latch the instance and count the cause. It is a\n  subtraction over three fields of the segment header and in practice\n  cannot fail, but it is not declared `noexcept` — and a throw escaping\n  from there would have discarded all three, turning a classified refusal\n  into an unhandled exception. It is now read best-effort, and returns a\n  sentinel the log prints as \"unavailable\" rather than a figure.\n\n- `free_before` was taken once, before the guard, and then reported by\n  every attempt. After a rotation the attempt runs on a different segment,\n  so the line could compare one generation's free bytes with another's —\n  and after a rotation forced without reaching the segment it reported the\n  untouched initial value, which as a zero read as \"no bytes left\". It is\n  seeded with the sentinel and re-read after each rotation that completed.\n\n- The retired generation is captured before the rotation instead of\n  reconstructed as `new - 1`. A version number is an identity, not a\n  position; version_catalog.hpp documents that the numbering has holes in\n  it wherever compaction has drained one.\n\n- The tests read `.error()` without first establishing that the result held\n  no value, which is undefined and lets a case that wrongly succeeded pass\n  by reading a dead union member. `refused_with` asks the two questions in\n  the order they have to be asked, and labels which operation failed.\n\nNew coverage: the probe's own contract; all three insert outcomes driven\nwith the probe failing throughout, in both modes; and the diagnostic itself,\nread back through the public log callback.\n\n* fix: settle the decision before saying anything about it\n\nA failed insert has to end in a classified error code. Everything it says\nabout the failure — assembling the message and emitting it — could throw,\nand until now that was enough to change the outcome:\n\n- `fmt::format` built the shared diagnostic before anything was decided, so\n  a failure to allocate the *message* left a contradicted map uncounted,\n  unlatched, and reported as an unrelated exception;\n- the log at the end of a *successful* rotation ran inside `rotate_for`'s\n  `catch (std::exception const&)`, so a refusing backend turned a generation\n  that exists on disk into `file_open_failed`, refused the retry the insert\n  was owed, and latched the instance out of the size class for good;\n- the log before the rotation gated the rotation itself, and the log after a\n  stored entry gated the report of it.\n\nTwo boundaries, at the two places a throw can start:\n\n- `utxoz::log` never propagates. Formatting and emission both run inside a\n  guard, and the six level functions are `noexcept`. The backend ends in a\n  spdlog sink or a caller's callback, neither of which this library owns.\n- `detail::diagnose` guards what the caller builds — `fmt::format`,\n  `outpoint_to_string`, `free_bytes_display` are the caller's arguments and\n  are evaluated before the log call is entered.\n\nThe state a failure needs — the counter, the latch, the rotation, the return\n— is now settled before either boundary is crossed, in full and reference\nalike.\n\nAlso, found while testing the above: `failpoints::clear()` never reached\n`fail_after_segment_create` or `fail_after_segment_stamp`. A case that armed\none made every case after it fail to create a container, which is precisely\nwhat test_failpoint_reset exists to prevent; two call sites had been storing\nthem back by hand. Both are now cleared and both are pinned by that test.\n\nAnd three corrections to the diagnostic tests: `log_capture` restores the\ncallback it displaced rather than clearing one it does not own; `field_in`\nmatches a whole field name, so `generation` no longer reads the value of\n`old_generation`; and the generation identifier is read from `census()`,\nwhich reports identities, rather than from the rotation count, which is a\ndifferent statement that happens to share a value.\n\n* fix: report the rotation that happened, not the one about to\n\nThe `diagnose` contract says the counter, the latch, the rotation and the\nreturn are settled before anything is said about them. The recoverable path\ndid not honour it: it emitted \"rotating and retrying\" and then called\n`rotate_for()`. Nothing could escape — `diagnose` catches and `utxoz::log`\nis `noexcept` — but the ordering was still wrong on its own terms, and it\nleft a line that a failed rotation then contradicted two lines later.\n\nBoth rotation sites now act first and describe the outcome afterwards, in\nfull and reference alike:\n\n- the preventive rotation reports `old_generation`/`new_generation` and the\n  figures of the sealed map, snapshotted while it was still open;\n- the recoverable one reports the rotation it made, or, when the rotation\n  could not be made, the figures of the failure that wanted it — which\n  `rotate_for`'s own message does not carry.\n\nFor that to be truthful the shared diagnostic must not read anything a\nrotation replaces, so the generation and the free-byte count are copied when\nthe handler is entered rather than read through when the message is built.\nWithout the copies the failure's own `generation=` field named the\ngeneration created to replace it.\n\nAlso: a direct case for `field_in`, which reads `generation` off a line that\ncarries `old_generation` and `new_generation` too.",
          "timestamp": "2026-08-20T14:41:03+02:00",
          "tree_id": "a621b8b2f04dc42928c17b0d69d158d56c58dcc6",
          "url": "https://github.com/utxo-z/utxo-z/commit/fff255837299f9b1a41c6191d3b9863e703ac83d"
        },
        "date": 1787229964278,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 406370.08,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 533879.56,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 544766.02,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 866834.35,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 506.57,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 574.36,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 14899633.88,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 26944310.57,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 14291921.02,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 14183.09,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2227719.54,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2308187.06,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 125048.56,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 13336.55,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 2998.11,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 3920.1,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3240.13,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 1105.22,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 1100.52,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 1093.89,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 1098.85,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 1113.27,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 1101.81,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: active hit, first class",
            "value": 15051681.09,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: miss, every class probed",
            "value": 26614488.29,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: sweep of 256 keys over three generations",
            "value": 32060.75,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: mixed 9 active hits to 1 historical",
            "value": 874130.41,
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
          "id": "21d483f5e47776b7d77db7484b3bf9b4891e5a94",
          "message": "test: name the repeated setup, and hold clear() to every seam (#141)\n\n* test: name the setup thirty sections were repeating\n\nFive lines — open, check, move, store the first key — appeared identically in\nthirty places and said nothing about the case they belonged to. A case's\nsubject starts at the failpoint it arms.\n\n`full_with_one` and `reference_with_one` derive from `temp_db` rather than\nholding one, so `dir` still names the directory and the destruction order is\nunchanged: the database closes while its files are still there.\n\nNo case's failpoints or assertions change; the suite runs the same 366\nassertions in the same 23 cases at all three statistics levels.\n\n* test: hold clear() to every seam, not to six of them\n\nThe case is named \"clear() reaches every seam\" and named six of the\nthirty-four. That is how `fail_after_segment_create` came to sit outside\n`clear()` unnoticed until a case armed it and every case that ran afterwards\nfailed to create a container, pointing at nothing.\n\nAll thirty-four are now armed, proved to have been armed — a subset check\nagainst a `clear()` that reset nothing would have passed as readily — and\nchecked afterwards, from one list used for all three so they cannot drift.\n`forced_database_id` and `before_target_publish` are handled beside it,\nbeing an array and a function pointer.\n\nThe array is checked byte by byte rather than at its ends. `first_byte_not`\nreturns the index instead of a bool so a failure names the byte: an `all_of`\nwould report `false`, which says nothing about where. Verified by leaving\nbyte 7 dirty in `clear()` — the edge-only form passed, this one reports\n`7 == -1`.\n\nAdding a seam still means adding a line here, but it is one line in one\nplace, and a seam missing from `clear()` now fails in this file rather than\nsomewhere unrelated. Verified by dropping three different seams from\n`clear()`: each names itself.",
          "timestamp": "2026-08-20T16:07:26+02:00",
          "tree_id": "defb5a11a08ecb21fb55f07ef623566e1a0e07af",
          "url": "https://github.com/utxo-z/utxo-z/commit/21d483f5e47776b7d77db7484b3bf9b4891e5a94"
        },
        "date": 1787235703231,
        "tool": "customBiggerIsBetter",
        "benches": [
          {
            "name": "insert P2PKH (43B)",
            "value": 965738.84,
            "unit": "ops/sec"
          },
          {
            "name": "insert P2SH (41B)",
            "value": 1285782.82,
            "unit": "ops/sec"
          },
          {
            "name": "insert 123B",
            "value": 1353907.51,
            "unit": "ops/sec"
          },
          {
            "name": "insert 89B",
            "value": 1774732.76,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (P2PKH)",
            "value": 396.64,
            "unit": "ops/sec"
          },
          {
            "name": "bulk insert 10K (chain mix)",
            "value": 511.5,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (latest version)",
            "value": 8537809.27,
            "unit": "ops/sec"
          },
          {
            "name": "find miss",
            "value": 22441169.23,
            "unit": "ops/sec"
          },
          {
            "name": "find hit (chain mix)",
            "value": 14312075.25,
            "unit": "ops/sec"
          },
          {
            "name": "batch find 1K hits",
            "value": 15839.31,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes hit (1 entry)",
            "value": 2526709.32,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes miss (1 entry)",
            "value": 2889899.52,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes (100 entries)",
            "value": 146209.98,
            "unit": "ops/sec"
          },
          {
            "name": "apply_deletes 1K",
            "value": 17226.25,
            "unit": "ops/sec"
          },
          {
            "name": "simulated IBD (100 blocks)",
            "value": 3045.79,
            "unit": "ops/sec"
          },
          {
            "name": "insert-heavy workload (1K inserts, 100 finds)",
            "value": 4419.27,
            "unit": "ops/sec"
          },
          {
            "name": "read-heavy workload (5K finds on 1K entries)",
            "value": 3795.79,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 1K (P2PKH)",
            "value": 2283.5,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (P2PKH)",
            "value": 876.99,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (P2PKH)",
            "value": 168.78,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 100K (P2PKH)",
            "value": 236.16,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 10K (123B)",
            "value": 2285.31,
            "unit": "ops/sec"
          },
          {
            "name": "close+reopen 50K (123B)",
            "value": 1642,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: active hit, first class",
            "value": 15672558.11,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: miss, every class probed",
            "value": 22009148.55,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: sweep of 256 keys over three generations",
            "value": 41786.52,
            "unit": "ops/sec"
          },
          {
            "name": "telemetry: mixed 9 active hits to 1 historical",
            "value": 1001698.95,
            "unit": "ops/sec"
          }
        ]
      }
    ]
  }
}