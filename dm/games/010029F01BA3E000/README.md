# Xuan Yuan Sword 7 (`010029F01BA3E000`)

Base game 1.0.0 (v0), Unreal Engine 4.25.0, SDK 17.5.3, NVN. Modules `main`, `subsdk0` (NintendoSDK_movie), and `sdk`. Simplified Chinese (`zh-Hans`) is included in the game.

The Mac bring-up uses the general Unreal allocator and address-region rules, as the other Unreal profiles do. No game-specific host branch, fixed-address patch, or shader translation rule was added.

## Containers and shaders

The four pak files use version 9 with frozen memory-image indexes. `dm/gfx/tools/uepak.py` reads the footer's compression method names, the relative pointers, sparse directory maps, and frozen entries. Its bounds checks reject truncated pointers, arrays, and sparse-map storage. The same reader also handles the older non-frozen index. The format references are linked beside the reader; its regression tests are `dm/tools/tests/test_uepak_v9.py`.

The shader archives use the existing LZ4 archive reader, as DRAGON QUEST VII Reimagined does: 1,489 global and 79,611 game entries, 77,563 shaders after deduplication (635 compute, 63 geometry, 66,558 pixel, 10,307 vertex). Keep `shader_groups` and `shader_note` unchanged when moving the generated profile; both are shader inputs.

## Memory and input

The profile uses a 4.375 GB alias region and a 5 GB total guest reservation. The generic Unreal rewriter recognizes this main's FMallocBinned3 and caps its 62 GB engine region to the available alias space. SDK 17.5.3 build `311deab1f062b504f3d646a49ddcd4e8` also uses the registered 62 MB node-area cut and a 1 GB-to-32 MB alignment cut: the latter moved the 4.06 GB reservation from alias +0.53 GB to +0.06 GB, ending at +4.125 GB. A 75 s Mac probe passed the original abort but remained at the Playerium splash. A follow-up interactive Mac probe continued an existing save from the title into the house's 3D interior, reached frame 6600 and ran without aborting. Mapped alias pages reached +1.50 GB; this is distinct from the reserved engine-region end. This probe is Mac evidence; the separate iPhone check is recorded below. See `Games/xys7/input/010029F01BA3E000_build/ios-probe/formalinteractive/layout.ok` for the recorded layout and screenshots.

The game uses the left stick for movement and the right stick for its camera, with A to interact. Keep the default docked operation mode and the 3D controller layout.

## Bring-up evidence and limits

On an Apple M5 Max, the diagnostic host reached the Simplified Chinese title, normal difficulty, a new game, the house controls tutorial, movement and camera control, opening the door, and the yard's first combat objective. An automatic save was generated and continued after restarting the host. These runs used a separate partial shader cache to investigate while the full library compiled; they do not validate the final app or every material.

The full Mac shader build completed with all 77,563 shaders reported OK and no failed variant compiles. The final self-contained Mac app was launched with `--no-launcher`, loaded the automatic save into the house, and responded to movement and camera input. Its signed host's UUID and file-backed sections match the current build. The default settings launcher reports 12 settings, but desktop automation could not attach to its window (`cgWindowNotFound`), so the settings-window interaction is not validated. Audio device callbacks run and recover from a 215 ms callback stall observed during one full-library run; listening quality and first combat have not been verified. Later chapters, long sessions, and other Macs have not been tested.

The build machine has Metal 32023.917 while the repository lock names 32023.922.4. The build uses the existing `--allow-other-metal` option and its separate compiler cache, without changing the lock or shader chain.

## Found on this game

- [Unreal pak version 9 frozen indexes](../../../docs/reference/shaders.md#unreal-pak-version-9-frozen-indexes): general container parsing, with no game-ID gate.
- [1 GB alignment of regions of 1 GB and more](../../../docs/reference/memory.md#1-gb-alignment-of-regions-of-1-gb-and-more): SDK 17.5.3's large-region alignment cut and the 5 GB Mac probe.

The offline `metal-tt` pipeline archive built on this machine was rejected at run time because its deployment target was incompatible with the installed OS. The deliverable is therefore packed with `--gpu none`: the existing host builds its pipeline bases on the first start and caches them outside the app. The first packaged launch completed the prebuild with no failures. No Metal lock or shader-chain change is needed for this fallback.

Final empty-cache check: the bundled app's `DM_PSO_PREBUILD=tool` run used a new `DM_PSO_USER` and `DM_PSO_ARCHIVE`, built 10,300 bases from 10,307 pairs in 117.6 s, left 7 to the draws by the host's existing rules, reported 0 failures, and exited with 0. The final app requires arm64 macOS 26.6 or later; the tested machine runs macOS 27.2.


## iPhone check

The full signed iPhone build installed on an iPhone 17 Pro Max running iOS 27.2. With `DM_LAUNCHER=0` and `DM_CTL` scripted input, it displayed the first-run brightness setting, loaded the 3D house interior, dismissed the controls tutorial, and responded to short input. The runtime log reached frame 8100 without RED lines, aborts, crashes, or missing shaders; its last interval reported 30 fps and a 3194 MiB footprint. Installation, launch JSON, screenshots and `ios-runtime-02.log` are in `Games/xys7/input/010029F01BA3E000_build/logs/`. Independent review passed this limited scope. Touch controls, settings-launcher interaction, combat, save/restart, listening quality and long sessions remain unverified.
