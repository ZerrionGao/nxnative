# Local game data

Keep each game's dumps and keys under `Games/<game>/` and do not commit them.

- `input/` is the NSP folder passed to `scripts/build_ipa_from_nsp.py`. Put the assembled base XCI and `keys/` there. Segmented dumps and extract caches are removed once the assembled XCI and the packed apps exist.
- `profile/<title ID>/` may hold a provisional profile used with `DM_GAMES_DIR` (see [PORTING.zh.md](../PORTING.zh.md) for Dynasty Warriors 8 XL and Warriors Orochi 3 Ultimate).
- `artifacts/apps/` holds Mac `.app` bundles; `artifacts/signed/` or `artifacts/ipas/` hold phone packages. Keep only the newest of each kind.

The `input/` and `artifacts/` directories are ignored by Git because they can contain copyrighted game data, private keys, and large build outputs. Committed notes and profiles under `Games/dw8` and `Games/wo3` never include dumps or keys.
