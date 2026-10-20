# Análise dos agentes (2026-10-20): propostas e verificações, texto integral

Gerado por 8 agentes só-leitura sobre o branch `fix/run-windows-brace` (91f388c). As áreas Ryzen e VRAM NÃO têm verificação (a do Ryzen não foi gravada; a da VRAM foi interrompida): confira essas propostas no código antes de implementar.


## ÁREA: Features keyed to 01.007.000 / PPSA01341 / 1.07 hashes or code bytes, checked against the user's PPSA01342 01.005.000
### Diagnóstico
Bottom line: almost every guest patch already runs on the user's PPSA01342 1.05. Two features matter for fps and do not work. (1) The 1080p outputs make the emulator stop at boot. (2) The lighting rule matches no 1.05 shader. The version-agnostic way to get a 1080p 3D scene is -RenderHeight 1080, which works today. Average fps is unknown: the log only records frames over 100 ms.

A. Features that DO apply on PPSA01342 01.005.000 (proved from code and the session log)
- Job-worker idle/spin patches. They are located by signatures that "every build seen (01.005.000 and 01.007.000) has" (src/loader/demonsSoulsIdle.h:24-36). Log lines 37-38: "Demon's Souls idle wait: installed 50 us backoff (call at eboot+0x820f3b)" and "Demon's Souls job wait: the spin's TSC read at eboot+0x8210d1 backs off 50 us (live kyty_local_spin_backoff_us)".
- memmove fast path. Uses a body-hash list per version that includes 01.005.000 (src/loader/demonsSoulsCopy.cpp:86-87). Log 39: "Demon's Souls copy: installed verified coherent memmove (libc.prx of 01.005.000)".
- GPU buffer pages. Seven functions are fingerprinted at fixed 1.05 RVAs (src/loader/demonsSoulsGpuPages.h:43-51). Log 40: "GPU buffer pages: 3 verified allocation calls of 01.005.000 installed, 4096-byte pages".
  - So the US eboot holds PPSA01341 1.05's code byte for byte at the same addresses.
  - The precompile also matched all 6,997 hint programs of PPSA01341 1.05.
  - Conclusion: PPSA01342 1.05 equals PPSA01341 1.05 for code and shaders, and every "1.05" fact in references/demons-souls.md holds for this dump.
- Scene and touch guards. Log 41-50, at the 1.05 addresses listed in demonsSoulsSceneGuard.cpp:53-54.
- Compute chains (barriers omitted). Gated on the title only (demonsSouls.h:17-19, demonsSouls.cpp:40-48).
- CPU linear copy. Its list includes the 1.05 hash 0x81bb2b1b9c751eca (demonsSouls.cpp:52).
- Engine convars. Injected by name into packagecmdlineargs.txt (gameArgs.cpp:36-54). Log 51-52: "Game args: F:/PPSA01342-app/packagecmdlineargs.txt read with" / "+doIndexBufferCulling=false". Whether 1.05 registers each name is unknown: the engine-log hook is 1.07-only.
- -RenderHeight / -Aspect render size. Located by masked signature plus value checks; 1.05 sites are documented (demonsSoulsRenderSize.h:28-38, .cpp:92-104). Not exercised in this session: the log has no "Demon's Souls render size:" line.
- Frame generation and FSR4 TAA inputs. Both versions' hashes are listed (src/local/frame-gen.h:27-29).

B. Features that do NOT apply, and why
1. The 1080p outputs ("1080p Quality" / "1080p Performance"; -Output 1080q / 1080f).
   - LaunchPlan.cpp:219-232 and run-windows.ps1:349-361 pass `--game-patch patches\1080p-*.json` for any PPSA0134[0-2].
   - Both files say `"id": "PPSA01341"`, `"version": "01.007.000"`.
   - ValidateGame requires the title to match (src/loader/gamePatch.cpp:111-116), so it prints "Game cheat error: cheat file does not match the loaded game".
   - Then src/loader/runtimeLinker.cpp:1641-1645 runs `EXIT("Failed to apply game cheat\n")`. **On PPSA01342 the emulator stops at boot.**
   - The launcher's warning (MainWindow.cpp:417-424, 601-605) says only "the HUD may have the wrong size".
   - Even with a matching title (PPSA01341 1.05) the patches would not apply:
     - The HUD mod is four writes at 1.07 offsets that share one shift. Its anchor is a 9-byte 0xCC code cave at 0xDDA224 that becomes `vmovss xmm0,[rip+0x171f930]; ret`. The other three writes are 1-byte edits at 0xE60430, 0xE641FA and 0x1559259, each +0x14. That retargets three rel32 calls from a getter at 0xDDA210 to the stub (inferred). A recompiled 1.05 eboot will not keep 7.8 MB of relative layout, so the result is NotFound.
     - The 1440p-scene mod checks only 4 bytes, under kMinSearchedBytes = 8, so it is skipped as TooShort (gamePatchLocate.h:34, 72-78).
     - r_Wants4K would still be injected, which gives the game's own 1080p tier. Its UI keeps the 4K layout off screen (demonsSoulsRenderSize.h:13-14).
   - Latent risk: the anchor chosen is the largest 'off', which here is the CC padding (gamePatchLocate.h:79-81). On another build Place() is tried at every CC run with only three 1-byte checks, so a false "Moved" write into code is possible. Probability unknown, small.
   - What 1080p is worth: a 1080p scene is 2,073,600 px instead of 3,686,400, 43.75% fewer for every render-size pass. The nearest measurement is -RenderHeight 1080 on a laptop RTX 3080 + Ryzen 9 5900HX at 1-1: 20.7 -> 24.5 fps standing, 18.7 -> 22.2 walking, +18-19% (run-windows.ps1:320-321).
   - -RenderHeight 1080 gives the same scene size on 1.05 today. 1080f would add only the output side: no 3840x2160 RGBA16F HUD canvas, composite and present. That is estimated under 1 ms per frame and unknown.
2. Lighting rule. It lists four 1.07 hashes (Translate.cpp:960-981): 7e676bf37482d768, decd2b04b5eee050, a3ecbeb1830c210c, and fog f0dc79c3467d5e0b.
   - 1.05 recompiled almost every shader ("10 of about 21000 in common", pipelineCache.cpp:149-150), so no 1.05 program matches.
   - Those programs run two lanes per invocation on the 3060 Ti (log 12: "Vulkan compute wave64: emulated on 32-lane subgroups").
   - KYTY_LANE_LOCAL_TITLE does nothing on 1.05.
   - On 1.07 the rule measured: lighting 4.34 -> 3.42 ms GPU and +1.6..+5.5% fps; the fog +3.5% at 1-1 (CPU-bound PC).
3. HUD anchoring for -Aspect. Uses 1.07 hashes (hud-anchor.h:36-37, 60). It is inactive at 16:9 (hud-anchor.h:160), so it is irrelevant on the user's 16:9 monitor.
4. FSR 3.1 frame generation without the HUD. FinalCompositeShaders are 1.07's 4K and 1080p programs (frame-gen.h:46-51). On 1.05, HUDLessColor is "not available" (fsr-frame-gen.cpp:642-647), so the HUD ends up inside generated frames.
5. Test harness. gamecmd, level travel, teleport, glide and waypoints use 1.07 fixed addresses verified by bytes (demonsSoulsWarp.cpp:266-287). On 1.05: no same-process convar A/B and no level tours, only cross-boot A/B.
6. tools/local/demons-souls-patch.py (boot-flow skip). Fixed to PPSA01341 / 01.007.000 (lines 101-102). Not perf-related.
7. File-open follow list. libAmpr.cpp:1590-1598 seeds this PC's recording only from files-<title>_<version>.files, and only files-PPSA01341_01.007.000.files ships (98,368 paths).
   - So first visits on PPSA01342_01.005.000 are not followed.
   - Laptop measurement: 1-1 loading fog 110 s unfollowed vs ~20 s followed (libAmpr.cpp:1497-1499).
   - In the user's session the Ampr workers ran ~9.9 s busy per 10 s during loads ("Ampr: 3152 commands in 10 s on the workers, 9916.9 ms in all").
   - Idle-dominant SLOW frames total 21.0 s; how much of that is file opens is unknown.
8. Recorded shader tier.

   | Version | Programs | Pipelines | Hint file size |
   |---|---:|---:|---:|
   | 1.05 (hints-PPSA01341_01.005.000) | 6,997 | 6,991 | 138,046 B |
   | 1.07 | 13,345 | 16,007 | 340,372 B |

   - 1.07 figures come from static-precompile/README.md:60 and are a lower bound; the file has grown since.
   - general.md: the recorded tier is what play actually uses. Log 32: "Shader prefetch: 20374 programs (32931 inputs, 4448 of them recorded play's; 5523 recorded first)".
   - How much of the 814 SLOW pipelines a 1.07-sized tier would have removed is unknown.
9. Launcher and run-windows tested lists hold only PPSA01341 (GameInfo.cpp:31-34, run-windows.ps1:193), so "untested" dialogs appear. Cosmetic.
   - tests/ShaderRecompilerComputeTests.cpp:32324-32326 still expects PPSA01342 to be unsupported, which demonsSouls.h:17-19 contradicts.
   - So baseline failure #34 "DemonsSoulsProfile at title isolation: another title enabled the profile" is a stale test, not a regression.

C. Links to the measured data
- The GPU sat at 85-90% load and VRAM went over budget in 7 of 37 samples. The render-size lever (P1) and the lighting rule (P2) are the version-keyed items that reduce GPU work.
- Compile stalls were 61% of 77.5 s. The 1.05 recorded tier (P4) is the version-keyed part of that.
- Idle and loading time relate to the file-follow seed (P3).

Not version-keyed: SRT AOT plans are matched by plan source, not by version, and no KYTY_SRT_AOT_LIBRARY is in use here.
### Propostas
- **fix-1080p-other-builds** (small, muda SPIR-V: não): Stop the 1080p outputs from aborting PPSA01342 and route them by build (1080f -> render height 1080)
  - Mudança: Four small changes.
(a) src/loader/gamePatch.cpp ValidateGame():
- Treat another region of the same game as the same title when Libs::Graphics::DemonsSouls::IsSupportedTitle(title_id) && IsSupportedTitle(plan->title_id), like OtherRegion() in shader-hints.cpp:276-287.
- Set plan->same_version = (title_id == plan->title_id && version == plan->version), so writes are placed only by search.
- Keep failing for a different game.
(b) Map the outputs by build in src/fork-launcher/src/core/LaunchPlan.cpp (output block, lines 219-232) and run-windows.ps1 (-Output block, lines 349-361), plus the two warnings in MainWindow.cpp (417-424, 601-605):
- When contentVersion != 01.007.000, the 1080p patch cannot be placed.
- 1080f -> set KYTY_RENDER_HEIGHT=1080 with the 4K output; no KYTY_RENDER_CONVARS and no --game-patch.
- 1080q -> the 4K output (same 1440p scene).
- Print one line saying why.
(c) src/loader/gamePatchLocate.h Locate():
- Never use as anchor a write whose 'off' bytes are all one value (0xCC, 0x00 or 0x90).
- Count such bytes out of CheckedBytes for the search, so a mod needs at least 8 real code bytes to be moved.
- This turns the 1.07 HUD mod into NotFound on other builds instead of a possible false 'Moved' write.
(d) Fix stale docs and tests:
- tests/ShaderRecompilerComputeTests.cpp:32324-32326: expect PPSA01342 supported.
- README.md:36-37 and docs/PORTABLE-README.md:259-266: the GPU-limit probe on 1.05 is the 3D resolution, not Output 1080p.
  - Arquivos: src/loader/gamePatch.cpp, src/loader/gamePatchLocate.h, src/fork-launcher/src/core/LaunchPlan.cpp, src/fork-launcher/src/ui/MainWindow.cpp, run-windows.ps1, tests/ShaderRecompilerComputeTests.cpp, tests/GamePatchLocateTests.cpp, README.md, docs/PORTABLE-README.md
  - Efeito esperado: Today on PPSA01342, picking either 1080p output stops the emulator at boot. Yet docs/PORTABLE-README.md:259 tells GPU-limited users to try exactly that.

After the fix, "1080p Performance" gives a 1920x1080 3D scene: 2,073,600 vs 3,686,400 px, 43.75% fewer for every render-size pass.
- Nearest measurement: laptop RTX 3080 + 5900HX at 1-1, +18% standing (20.7 -> 24.5 fps) and +19% walking (18.7 -> 22.2) (run-windows.ps1:320-321).
- On the user's 3060 Ti (85-90% GPU load) expect +10-20% where GPU-bound and ~0 where the render thread bounds; the real figure is unknown until measured.
- Render targets shrink by an estimated ~0.15 GB (assumption: ~15 full-size targets x ~6 B/px x 1.61 Mpx). This helps the 7 of 37 over-budget VRAM samples by an unknown amount.

The fix itself adds nothing over passing -RenderHeight 1080 by hand. It removes a boot abort and a latent false patch placement.
  - Risco: Low for the code itself.
- (a) lets a cheat file of one region apply to another, but only where all its original bytes are found, as already happens across versions.
- Possible new pipelines at the 1080p render size: storage images with dynamic mip levels carry their mip count in the shader specialization (ResourceMaterialization.cpp:168-175). For example, a hi-Z or downsample chain has 11 levels at 1920 vs 12 at 2560.
- A few such programs may compile at first use (unoptimized-first on NVIDIA, then cached); the count is unknown.
- Viewport and scissor are dynamic state (shaders.cpp:1120-1121), so graphics pipelines are unaffected.
  - A/B: 1. Run run-windows.ps1 -Game F:\PPSA01342-app -Set KYTY_FPS_HUD=2 (default 1440p scene) against the same command plus -RenderHeight 1080 (after the fix, the same as -Output 1080f).
2. Use 2 boots per side, alternated, at a fixed standing spot plus a camera spin, and read fps and GPU busy from the overlay.
3. Check that the log prints "Demon's Souls render size: performance mode renders 1920x1080 instead of 2560x1440 (output 3840x2160)".
4. For (c): unit-test Locate() in tests/GamePatchLocateTests.cpp with a CC-anchored mod on an image whose CC runs differ: it must return NotFound.
- **lighting-rule-105** (medium, muda SPIR-V: sim): Find the 1.05 counterparts of the four lane-local lighting/fog shaders and add them to the title list
  - Mudança: 1. In src/graphics/shader/recompiler/frontend/translate/Translate.cpp, next to TitleLaneLocal (lines 967-980), add KYTY_LANE_LOCAL_HASHES=<hex,hex,...>. It is parsed once and its hashes are treated like TitleLaneLocal when KYTY_LANE_LOCAL_TITLE != 0. This lets candidates be tested without a rebuild.
2. Add "KYTY_LANE_LOCAL_HASHES" to StoreSwitches() in src/graphics/host_gpu/renderer/pipeline/pipelineCache.cpp:937-939, so a candidate set gets its own translation store.
3. The local agent identifies candidates on the user's PC:
   - Decode the four 1.07 records from tools/local/static-precompile/hints-PPSA01341_01.007.000.hints with warmfile.decode_record (pip install zstandard). Take their code word count, threads, user-data count and buffer/image slots.
   - Scan _Build\static-precompile\released-PPSA01342_01.005.000.seeds (records with code) for wave64 compute programs with the same layout and ±15% word count.
   - Lighting candidates: code with S_ORN2_SAVEEXEC_B64 plus V_READLANE_B32 at lanes 31 and 63, one of them also with V_READFIRSTLANE.
   - Fog candidate: DS_* LDS traffic without S_BARRIER over the 214x120x72 volume.
   - references/demons-souls.md lists six light-union shaders in 1.07; only three were proved, so match by layout, not by idiom alone.
4. Optionally confirm with -Set KYTY_LANE_LOCAL_LOG=1,KYTY_LANE_LOCAL_TITLE=1. That forces a new store and prints "LaneLocal CS <hash>: <reason>" (same refusal reasons as 1.07).
5. After same-spot screenshots at 1-1, Latria and Shrine are within noise (the fork's 10-14 check), add the 1.05 hashes to TitleLaneLocal. KYTY_LANE_LOCAL_TITLE=0 then stays the A/B switch.
  - Arquivos: src/graphics/shader/recompiler/frontend/translate/Translate.cpp, src/graphics/host_gpu/renderer/pipeline/pipelineCache.cpp, tools/local/static-precompile/warmfile.py, tools/local/static-precompile/hints-PPSA01341_01.007.000.hints
  - Efeito esperado: On 1.07 (RTX 5090, CPU-bound PC) the rule measured: lighting GPU 4.34 -> 3.42 ms (-0.92 ms), +1.6..+5.5% fps; the fog rule +3.5% at 1-1.

On the 3060 Ti, a derivation, not a measurement:
- On the 5090 the GPU frame was ~9-10 ms (general.md lane-local row: 10.05 -> 9.07 ms), so the rule removed ~9-10% of GPU time.
- With GPU load at 85-90%, a similar share would shrink the GPU frame by ~9-10%, plus the fog's share.
- That is up to ~+10% fps in GPU-bound views and 0 where the render thread bounds.
- Whether the 1.05 programs share the idiom and the gain: unknown until found and A/B'd.
  - Risco: Wrong lighting if a 1.05 program uses the wave-wide union for more than choosing which lights to walk.
- Mitigation: screenshot A/B at 3 places, and the switch stays.
- Disk and time: each KYTY_LANE_LOCAL_HASHES value keeps its own ~5 GB translation store, and the first launch with it retranslates ~20K programs (~2 min).
- Editing Translate.cpp changes KYTY_RECOMPILER_SIGNATURE (generate_version.cmake:41-64), so the store is rebuilt once after the update (~2 min).
  - A/B: 1. Use two exes, or one exe with -Set KYTY_LANE_LOCAL_HASHES=<candidates> vs without; after the hashes are built in, -Set KYTY_LANE_LOCAL_TITLE=0 vs default. Each side has its own translation store, so let each translate once first.
2. Run 2 boots per side, alternated, at the same spot with -Set KYTY_FPS_HUD=2; compare fps and GPU ms.
3. Take screenshots at the same spot for both sides.
- SPIR-V changes only for the ≤4 matched programs.
- Their few pipelines are not in the static .binaries and compile once at first use (unoptimized-first on NVIDIA).
- The other ~20K programs keep hitting the static cache.
- The 1-hour precompile does NOT need re-running; a later run picks them up.
- **file-follow-other-build** (small, muda SPIR-V: não): Seed the file-open follow list from another Demon's Souls build when this build has none
  - Mudança: In src/libs/libAmpr.cpp, the call_once block at 1590-1598 and SeedFileRecording at 1500-1504.
- When tools/local/static-precompile/files-<title>_<version>.files is missing and the title is DemonsSouls::IsSupportedTitle, use the first files-PPSA0134?_*.files: the same version first, then any.
- When this PC's recording _PipelineCache/files/<id>.files already exists, append once the release list's paths it lacks. Mark this with a sidecar <id>.files.seeded so it never repeats.
- This is safe:
  - Paths are /app0-relative (local-platform.cpp:722-726).
  - Duplicates are dropped on load (index.emplace, line 726).
  - Files that do not exist are skipped by the opener threads (line 614: "a file of an earlier session gone").
  - Arquivos: src/libs/libAmpr.cpp
  - Efeito esperado: Faster loading and first-visit streaming, not steady fps.
- Measured on a 16 GB laptop: 1-1 loading fog 110 s unfollowed vs ~20 s followed (libAmpr.cpp:1497-1499).
- On the user's PC (32 GB, NVMe) the gain is unknown.
- The session had 21.0 s of idle-dominant SLOW frames (render thread waiting for the game), and Ampr workers ran 9.9 s busy per 10 s window during loads. The part of that caused by file opens is unknown.
- It helps only worlds not yet visited on this PC, which after the tutorial is most of the game.
  - Risco: Low. Only extra background CreateFileW calls at below-normal priority, with 16 threads and a window of 1024 (local-platform.cpp:590-592). Paths absent in 1.05 cost one failed open each.
  - A/B: Cross-boot, first visit of a world: run -Set KYTY_FILE_FOLLOW=0 against the default.
- First visits are one-shot, so restore a copy of _PipelineCache\files\PPSA01342_01.005.000.files between boots and reboot Windows (cold file cache) before each.
- Time from loading start to control, e.g. Nexus -> Boletaria 1-1.
- **hints-105-coverage** (medium, muda SPIR-V: não): Raise the 1.05 recorded shader tier with this PC's play (ship merged 1.05 hints)
  - Mudança: No emulator code change; this is a data file.
1. After a few hours of play with the warmup recording (KYTY_SHADER_WARMUP=1 is already in run-windows.json), run kyty_shader_precompile on the user's PC:
   --game F:\PPSA01342-app --seeds _Build\static-precompile\seeds-PPSA01342_01.005.000.seeds --make-hints tools\local\static-precompile\hints-PPSA01342_01.005.000.hints --recorded _PipelineCache\warmup-v2\<device>\PPSA01342_01.005.000.shaders --recorded _Build\static-precompile\hinted-PPSA01342_01.005.000.seeds
   (or the released- file the log names).
   The second --recorded carries the shipped 1.05 hints with code, so the result is the union.
2. Commit it to tools/local/static-precompile/.
   - precompile-windows.ps1:175-177 and fork-launcher Precompile.cpp:103-109 already prefer an exact-title file.
   - OtherRegion() (shader-hints.cpp:276-287) lets PPSA01341/PPSA01340 1.05 use it too.
3. precompile-windows.ps1 -ExportHints is the user-facing equivalent, without the merge.
  - Arquivos: tools/local/static-precompile/hints-PPSA01342_01.005.000.hints, precompile-windows.ps1
  - Efeito esperado: The 1.05 recorded tier is about half of 1.07's: 6,997 programs / 6,991 pipelines vs ≥13,345 / 16,007; the files are 138,046 vs 340,372 B.
- general.md: recorded play is what gets used (two 1-1 walks: ~2,000 of ~2,200 pipelines from the recorded tier).
- The session logged 814 SLOW pipeline lines, and compiles were 61% of the 77.5 s stalled.
- Which share a fuller tier would remove is unknown; measure it with precompile.py runtime-coverage on a cold route.
  - Risco: Low. Hints carry no game code. A precompile re-run on this PC compiles only pipelines not already in the static cache. Use the default mode, not -Quick: with binaries the final merge keeps only what the run's seeds make (precompile-windows.ps1:33-36), so -Quick may drop the non-recorded pipelines (not verified).
  - A/B: 1. Before and after the re-run with merged hints, play the same route from a fixed save with an empty driver-cache key.
2. Run tools/local/static-precompile/stalls.py on both logs; compare the SLOW *Pipeline counts and the total compile ms in SLOW Frame lines.
- **hud-1080p-port-105** (large, muda SPIR-V: não): Port the 1080p HUD patch to 1.05 by masked signatures (only after a ceiling check on 1.07)
  - Mudança: Ceiling first: on a 1.07 install, compare -Output 1080f with -RenderHeight 1080 in GPU ms at a fixed spot. Port only if the output-side saving is clearly above noise (>~3%).

If it is:
1. Someone with a 1.07 dump reads the five sites with tools/local/windows/guest-disasm.py:
   - getter 0x900DDA210 --size 0x40;
   - call sites around 0x900E6042F, 0x900E641F9, 0x901559258 (inferred from the +0x14 rel32 edits);
   - the stub's float at 0x9024F9B5D (--dump 1);
   - the 1440p-scene site 0x90090A667 ±0x40.
   Write masked patterns with rel32 and RIP wildcards from these.
2. Add src/loader/demonsSoulsHud1080.cpp with Install(), called from runtimeLinker.cpp next to DemonsSoulsRenderSize::Install, active when KYTY_RENDER_CONVARS sets r_Wants4K.
   - GuestCode::Find the getter by its pattern (exactly one match).
   - Find its callers as e8 rel32 whose target is the getter, filtered by the context patterns (exactly three).
   - Write a stub into int3 padding next to it: b8 <imm32 float>; c5 f9 6e c0; c3. That is mov eax,imm; vmovd xmm0,eax; ret, with no constant-pool dependency.
   - Retarget the three calls.
   - For 1080q, patch the scene-size site found by a ≥8-byte pattern.
3. Then drop patches/1080p-*.json.

Data needed from the user's dump: none beyond running the emulator with the patterns; the local agent can also verify hits with guest-disasm.py <low>:<high> --find <hex>.
  - Arquivos: src/loader/demonsSoulsHud1080.cpp, src/loader/runtimeLinker.cpp, patches/1080p-fast.json, patches/1080p-quality.json
  - Efeito esperado: Small for this user.
- Over -RenderHeight 1080 it saves only the output side: the 3840x2160 RGBA16F HUD canvas clear/draws, the 4K composite and the 4K present. Estimated under ~1 ms per frame on a 3060 Ti (~0.2 GB/frame of 4K target traffic at 448 GB/s ≈ 0.5 ms); real figure unknown.
- Unknown whether WantsBase also lowers other internal buffers.
- On the user's 2560x1440 monitor a 1080p output is upscaled (softer HUD), whereas the 4K output is downscaled.
  - Risco: Medium.
- Wrong call sites mean a broken HUD or a crash at boot; mitigated by unique-match requirements and refusing to install on any mismatch.
- Needs 1.07 access, which the user lacks.
  - A/B: Cross-boot: -Output 1080f (ported) vs -RenderHeight 1080, 2 boots each, same spot, -Set KYTY_FPS_HUD=2. Also take a screenshot of the HUD at 1080p output in game and in menus.
### Ações do usuário sem código
- Do not choose Output "1080p Quality" or "1080p Performance" in the launcher, or -Output 1080q / -Output 1080f in run-windows.ps1, with PPSA01342. The emulator stops at boot with "Game cheat error: cheat file does not match the loaded game" and then "Failed to apply game cheat" (gamePatch.cpp:111-116, runtimeLinker.cpp:1641-1645). Use the 3D resolution instead: powershell -ExecutionPolicy Bypass -File .\run-windows.ps1 -Game F:\PPSA01342-app -RenderHeight 1080 (or 1296 / 1152; launcher Output 4K + 3D resolution). Confirm the log line "Demon's Souls render size: performance mode renders 1920x1080 instead of 2560x1440 (output 3840x2160)". If it prints "the video settings of a known build not found", report it.
- Find out whether the GPU or the CPU limits fps. Add -Set KYTY_FPS_HUD=2 (detailed overlay: frame times, render-thread busy, GPU via NVML, VRAM) and run two boots each of the default and -RenderHeight 1080 at the same standing spot plus a camera spin. A clear fps rise with 1080 means GPU-bound; then keep the highest render height that holds your target.
- A/B the index-buffer culling setting on this GPU. The default removes it (KYTY_GAME_CONVARS=doIndexBufferCulling=false). It was measured on a CPU-bound PC as +6.4% fps but GPU busy 60 -> 71% (run-windows.ps1:314-318), which may reverse on a GPU-bound 3060 Ti. Compare the default with -Set KYTY_GAME_CONVARS= (empty removes it, so the game culls on its GPU again). Use 2 boots per side at the same spot and read fps and GPU % from KYTY_FPS_HUD=2. Note: the culling shaders' pipelines may compile on first use.
- Check which convar names exist in the 1.05 eboot before relying on -Quality or the convar defaults: python -c "d=open(r'F:\PPSA01342-app\eboot.bin','rb').read();print([(n,d.count(n.encode())) for n in ('doIndexBufferCulling','r_enableSpotShadows','r_enableAmbientShadowing','sunShadowMaxDistance','r_forceShadowLOD','r_forceViewportHeightForPixelSizeCalc','r_Wants4K')])". If every count is 0 (segments compressed), search the running emulator instead: python tools\local\windows\guest-disasm.py 0x900000000:0x904000000 --find 646f496e64657842756666657243756c6c696e67 --data (that hex is "doIndexBufferCulling").
- If the names exist and you accept fewer shadows (no ambient shadowing, no torch or spot-light shadows, sun shadows to 60 m), try -Quality shadows. It measured +12/+15/+17% at 1-1/Latria/Nexus on a CPU-bound PC; on the 3060 Ti it also removes shadow-map GPU work, amount unknown. Use the same A/B as above.
- Give first visits a file-open list, in F:\dev\KytyPS5 with the emulator closed. 1) Back up _PipelineCache\files\PPSA01342_01.005.000.files if it exists. 2) Run cmd /c "type tools\local\static-precompile\files-PPSA01341_01.007.000.files >> _PipelineCache\files\PPSA01342_01.005.000.files" (copy the file instead if it does not exist yet). Duplicates are ignored on load and missing paths are skipped. A/B with -Set KYTY_FILE_FOLLOW=0 on a world's first visit.
- After more play (the shader warmup records it), re-run precompile-windows.ps1 -Game F:\PPSA01342-app in the default mode, not -Quick. It folds this PC's recorded specializations into the static cache and compiles only what is missing. Then run precompile-windows.ps1 -ExportHints and share my-hints-PPSA01342_01.005.000.hints: the shipped 1.05 hints cover about half of 1.07's (6,997 vs ≥13,345 programs).
- Nothing to do about these; they are 1.07-only and none matters for fps on 16:9. KYTY_LANE_LOCAL_TITLE has no effect on 1.05 (no hash matches). -Aspect HUD anchoring and FSR 3.1 frame generation's HUD-free frames (on 1.05 the HUD goes into generated frames) need 1.07. gamecmd, travel, glide and waypoints in live control, and tools/local/demons-souls-patch.py, are 1.07-only. Baseline test #34 (demons_souls_compute_chain) fails because the test still expects PPSA01342 to be unsupported, not because of your build.

## ÁREA: First-run pipeline compile stalls despite the static precompile (PPSA01342 01.005.000, RTX 3060 Ti, driver 617.42)
### Diagnóstico
Read-only. I decoded the shipped hints file (tools/local/static-precompile/hints-PPSA01341_01.005.000.hints, zstd via ctypes) and matched it against every SLOW line in session.log. The scripts are in /tmp/claude-0/-home-user-kytyps5/63de2676-ca52-54c2-9dbe-e0ba09d9262f/scratchpad/tools/ (hints.py, cover.py, cs.py).

1. Only the synchronous compiles block the frame.
- The frame's compile= time is LiveCensus::WaitCompile, opened in only three places:
  - the translation on the render thread (pipelineCache.cpp:2291). There is no "SLOW TranslateProgram" line in the whole log. The prefetch had translated all 20,374 programs by 23:11:16.
  - CreateGraphicsPipeline (pipelineCache.cpp:4178).
  - CreateComputePipeline (pipelineCache.cpp:4251).
- In the log, CreateCompute is 81 lines totalling 33.6 s (p50 305 ms, p90 682, max 4385). CreateGraphics is 62 lines totalling 13.2 s (p50 171 ms). Together that is 46.8 s, which matches the SLOW Frame compile component sum of 43.9 s.
- The worst frames are runs of sequential CreateCompute calls inside one frame:
  - "SLOW Frame 12952.5 ms ... compile=12854.2" holds 51 CreateCompute (CS ids 4217-4269, 111-566 ms each).
  - The 7081 ms frame holds 10, and the 4754 ms frame holds 4.

2. What the Async* lines are.
- AsyncGraphicsPipeline = TryCreateGraphicsPipeline (pipelineCache.cpp:4121-4158). Its two variants:
  - table: TableWorkers, 8 threads, table driver cache.
  - native XPR: Workers, 4 threads, main cache.
- AsyncComputePipeline = TryCreateComputePipeline (4284-4320), for table dispatches, built PipelineBuild::Full at 4313.
- The render thread never waits on them. They return null and the draw or dispatch takes the normal path (table-xpr.inc:2573-2580; native-xpr.inc:1909-1918).
- A new CS program's first dispatch always goes through the normal path, because table mode needs the program the normal path made first (pipelineCache.cpp:2181-2184 "if (entry == programs.end()) return {}"; table-xpr.inc:2520-2525). So every new compute program costs one render-thread CreateComputePipeline, plus one async table compile.
- An Async line can show seconds (for example "SLOW AsyncComputePipeline 5092.8 ms table CS=0x43aa1fdf1cdc60d8"), but that time is worker time. What the frame waited on was the normal-path "SLOW CreateComputePipeline 4385.0 ms CS=41 CS=0x43aa1fdf1cdc60d8".
- In this session, the Async lines total 293 s of worker time: 277 AsyncCompute, all table, 209.7 s; 394 AsyncGraphics, 298 native and 96 table, 83 s.

3. Where KYTY_PIPELINE_FAST_BUILD applies.
- Only the two sync paths use FirstBuild() (pipelineCache.cpp:2814-2822): 4184 and 4255.
- Async graphics take the default PipelineBuild::Full (pipelineCache.h:488). Table compute is Full explicitly (4313). Warmup pipelines and the precompile are Full too.
- On a store and driver-cache miss, Fast compiles with eDisableOptimization into no cache (shaders.cpp:802-807). BuildOptimized then queues the optimized build on OptimizeWorkers, 4 threads (2826-2843).
- The fork measured 113 cold CS at 0.6 s total, max 26 ms, unoptimized (docs/EXPERIMENTS.md:517). Here, unoptimized CreateCompute p50 is 305 ms. That equals the optimized AsyncCompute p50 of 308 ms.
- Cause unknown. Two hypotheses:
  - (a) The driver serializes compiles in one process (README: "22 threads kept only 1.3 cores busy"). The unoptimized build would then wait behind the optimized build of the previous program, which BuildOptimized queues immediately. Supporting evidence: three 15.4 s table compiles ended within about 1 ms of each other (log lines 7304-7306).
  - (b) Unoptimized compiles of these large portable modules are simply slow on 617.42. Supporting evidence: the first CreateCompute (4385 ms) and 60 of the 81 had no logged concurrent compile. Optimize builds are not logged, though.
- For graphics, pipelineCache.cpp:2785-2787 already notes native variants cost "~100 ms each in no cache, unoptimized or not".

4. Static store lookup and identity.
- CreatePipelineHandle (shaders.cpp:694-810) looks up VK_KHR_pipeline_binary by the driver's key of the full create info, except for Optimize builds. Then the driver cache (cached(driver_cache)), then it compiles.
- The store opened fine: "Static pipeline binaries: 45735 pipelines, 74331 binaries, 3831 MiB ... (5 ms)".
- PPSA01342 vs PPSA01341 is not the cause. OtherRegion (shader-hints.cpp:276-287) accepted the PPSA01341 1.05 hints, and all 6,997 matched by code ("without their code in the seed file: 0 programs, 0 pipelines"). The game id PPSA01342_01.005.000 is consistent across the store, the inputs, released seeds and the translation store.

5. Which variants the precompile makes (staticPrecompile.inc WarmSeeds).
- Guessed seeds (the 13,385 programs not in hints): only cs-first / gfx-first, the portable form of SeedSpecializations. No exact, table or native variants (393-401). The plan log confirms: "64 exact and 55 table modules".
- Hinted seeds (6,997): cs-first, cs-exact, cs-table(-portable), gfx-first, gfx-exact, gfx-table.
- Native XPR variants are never made. native_first needs --native-variants (816), and no script passes it ("no native variants" in plan-seeds.log; shader-precompile.cpp:35-37).
- precompile-windows.ps1:320 passes --covered-by $Recorded, so the guessed variants of every hinted program are planned Skipped (make_here 678-691). The plan held 80,568 keys and 45,735 were stored, so about 34.8K guessed pipelines were dropped.

6. Which of the 814 the store should have served (from the decoded hints).

| Kind | Lines | Time | Breakdown |
|---|---|---|---|
| CreateCompute | 81 | 33.6 s | 80 lines / 79 programs (33.3 s) are hinted |
| CreateGraphics | 62 | 13.2 s | 12 pair recorded (3.2 s); 7 both stages hinted, pair not recorded (1.1 s); 40 one stage hinted (7.4 s); 3 neither (1.5 s) |
| AsyncCompute table | 277 | 209.7 s | 84 hinted (50.1 s); 193 not hinted (159.6 s), never precompiled by design. The 15-17 s ones are non-hinted 1049-1078-slot table CS ("Table: CS 0x20bed6800 (0x45b11cf35fb3db72): reads 1078 slots") |
| AsyncGraphics native | 298 | 61 s | never precompiled; 236 (47.0 s) have their VS/PS pair in the hints' recipes |
| AsyncGraphics table | 96 | 21 s | 7 pair recorded (1.1 s) |

- The key finding: of 1,214 CS programs that reached table mode, 936 are hinted and 78 of those (8.3%) had a slow normal-path compile. Of the 278 guessed-tier programs, 1 (0.4%) did.
- Hinted programs with 4 recorded specializations missed 8 of 20 (40%); those with 1 missed 55 of 2,137 (2.6%).
- So the runtime met hinted programs with specializations the partial 1.05 hints lack, and the guessed fallback that serves 99.6% elsewhere had been skipped. Which specialization field differs is unknown: no PIPESRC or PIPEKEY lines were logged.
- The ReportSession lines ("Made where first needed ...") are absent from session.log (unknown why).

7. Warmup and prefetch.
- warmup-v2 was empty ("loaded 0 shader inputs and 0 pipeline recipes"). This session was recorded (normal variants only, pipelineCache.cpp:4207).
- The released seeds only order the prefetch's translation ("5523 recorded first"); they make no pipelines.
- The translation store was written ("630 MiB index, 5054 MiB of SPIR-V").

8. Driver cache.
- The key is fixed (run-windows.ps1:389). The signature includes driverVersion (pipelineCache.cpp:110).
- Saved at the end: "saved 36116165 bytes ... PPSA01342.bin" and "saved 31885284 bytes ... PPSA01342.table.bin".
- If the driver cache alone fixes revisits, the second session should show:
  - at start: "Vulkan pipeline cache: loaded 36116165 bytes from ...PPSA01342.bin", the same for .table.bin, "Shader warmup: loaded N shader inputs and M pipeline recipes" with N, M > 0, and the translation store kept 20374 of 20374;
  - on the replayed tutorial-to-boss route: SLOW Create*Pipeline and SLOW Async* lines near 0, and the SLOW Frame compile sum going from 43.9 s to about 0. A few may remain for optimized builds still queued at exit, which are dropped (CompileWorkers::Run).
- New areas (Nexus, 1-1, ...) will repeat session 1's pattern. "invalidating ... (driver, emulator, or data mismatch)" would mean the cache was lost.
### Propostas
- **covered-by-local-only** (small, muda SPIR-V: não): Keep the guessed fallback for hinted programs: skip guesses only for programs this PC recorded
  - Mudança: precompile-windows.ps1 only.

1. In the '$Recorded -eq "*"' block (lines 171-212), build a second seed file from this PC's sources only: the warmup-v2 recordings and recorded-<id>.seeds, without released-<id>.seeds. Use kyty_shader_precompile --make-hints recorded-local-<id>.hints --recorded <each local source>, then --apply-hints into local-<id>.seeds.
2. At line 320, pass '--covered-by local-<id>.seeds' instead of '--covered-by $Recorded'. With no local recording, pass nothing, so a hint-only install keeps every guessed variant. -Full stays as it is (never skip).
3. Update the header text (lines 14-18).

No emulator code changes. The recorded tier is still compiled first, so 'ready to play' is unchanged.
  - Arquivos: precompile-windows.ps1
  - Efeito esperado: Render thread.
- On this route, 79 of the 80 slow-CreateCompute programs are hinted: 80 lines, 33.3 s of the 33.6 s CreateCompute stall. This includes the 51-, 10- and 4-compile bursts behind the 12.9 s, 7.1 s and 4.75 s frames.
- If guesses serve hinted programs as often as they served guessed-tier programs here (277 of 278 table-mode CS with no slow normal compile), about 33 s of the 46.8 s render-thread compile goes away, up to 37 s with the 19 both-hinted CreateGraphics lines (4.3 s). That is roughly 40-48% of the 77.5 s stalled.
- It applies to every new area for every player with hint-only coverage.

Cost.
- About 34.8K more pipelines (80,568 planned keys minus 45,735 stored).
- Store: about +2.9 GB (3,831 MiB / 45,735 x 34,833).
- Precompile time: the script itself says 'a fifth of a full run'. That is an estimate of 12-15 min incremental here; unknown until run.
  - Risco: No runtime risk: the change only adds pipelines to the store. New installs take longer to precompile and use more disk. After this PC records play, those programs' guesses are skipped again (as today), which the fork measured as nearly lossless with own recordings ('two 1-1 walks met 1 of 2,227').
  - A/B: Possible today without code.
1. Copy _PipelineCache\static\PPSA01342_01.005.000.binaries aside (B0).
2. Run 'precompile-windows.ps1 -Game F:\PPSA01342-app -Full -Recorded F:\dev\KytyPS5\_Build\static-precompile\released-PPSA01342_01.005.000.seeds' (B1). Passing -Recorded explicitly keeps session 1's local recording out, so B1 = B0 + guesses only.
3. Make cold runs alternating B0/B1 by swapping the file, 2 per side, same route (boot to first boss, or the first 10 min). Each run uses: run-windows.ps1 -Set KYTY_DRIVER_CACHE_KEY=<fresh 64 lowercase hex>,__GL_SHADER_DISK_CACHE_PATH=<new empty dir>,KYTY_SHADER_WARMUP=0,KYTY_PIPELINE_SOURCE_LOG=1.
4. Metric: count and sum of 'SLOW CreateComputePipeline', count of 'PIPESRC cs-normal ... static=0', and the SLOW Frame compile= sum.
- **native-variants-recorded-tier** (small, muda SPIR-V: não): Precompile native XPR variants for the recorded tier
  - Mudança: precompile-windows.ps1:
- add '--native-variants' to the plan-recorded dry run (lines 313-314), so the keys are planned once;
- add it to Start-Precompile for shards whose $seedFile -eq $Recorded (lines 363-364).
The guessed tier keeps no native variants (--native-guesses stays off, as shader-precompile.cpp:35-37 justifies). staticPrecompile.inc already makes gfx-first-native / gfx-exact-native for unbuffered read-only recipes (806-830), so no C++ change is needed.
  - Arquivos: precompile-windows.ps1
  - Efeito esperado: - On this route, 236 of the 298 'SLOW AsyncGraphicsPipeline ... native' lines (47.0 s of 61 s of background compile) have their VS/PS pair in the hints' recipes. They would be store hits, assuming their state and specialization match; the same unknown as in the first proposal.
- Those draws leave the normal path on first sight instead of 0.1-0.9 s later.
- About 47 s less driver work competing with render-thread compiles.
- No direct frame-stall removal: the render thread does not wait on native variants.
- Cost: '7% of a full run' (shader-precompile.cpp:36), about 4 min of the 58; an incremental rerun compiles only these.
  - Risco: Precompile time and store size only. The native variants' keys must match TryCreateGraphicsPipeline's native_bindings layout, which WarmSeeds' create(..., native=true) already uses.
  - A/B: Copy the .binaries aside, rerun the precompile with the flag, then make cold runs as in the first proposal, alternating files. Metric: count of 'SLOW AsyncGraphicsPipeline ... native' and 'PIPESRC gfx-native ... static=1' over the same route.
- **async-compute-table-fast-build** (small, muda SPIR-V: não): Unoptimized-first for async table-dispatch compute pipelines
  - Mudança: pipelineCache.cpp, TryCreateComputePipeline:
1. In the worker job (4312-4313), pass FirstBuild(m_graphics) instead of PipelineBuild::Full. CreatePipelineHandle then tries cached(TablePipelineCache()) and builds unoptimized into no cache (shaders.cpp:802-807).
2. Where the finished job is adopted on the render thread (4297-4300, after it is moved into m_compute_pipelines, so the address is stable), call BuildOptimized with a lambda: CreatePipelineInternal(m_graphics, layouts, input_info, module, TablePipelineCache(), PipelineBuild::Optimize, true).
3. Make that optimized job account to the table cache saver (m_table_pipeline_jobs and TablePipelineDone instead of MainPipelineDone), so the optimized table pipeline is saved in PPSA01342.table.bin.
4. Add a switch KYTY_ASYNC_FAST_BUILD (default on, where FirstBuild is Fast) in src/local/performance-switches.h.
Graphics async variants stay Full: pipelineCache.cpp:2785-2787 records '~100 ms each in no cache, unoptimized or not'.
  - Arquivos: src/graphics/host_gpu/renderer/pipeline/pipelineCache.cpp, src/graphics/host_gpu/renderer/pipeline/pipelineCache.h, src/local/performance-switches.h
  - Efeito esperado: - The 277 table compiles took 209.7 s of worker time (p50 308 ms, p90 939 ms, max 16.8 s).
- If the fork's compute ratio holds (89 s to 0.6 s, EXPERIMENTS.md:517), a table pair is usable about one frame after first sight instead of 0.3-16.8 s later. Dispatches stop paying the normal path sooner, and the long optimized builds leave the period right after first sight (they move to OptimizeWorkers).
- If unoptimized compute is inherently about 300 ms on 617.42 (possible: CreateCompute p50 305 ms here), the gain is small. Unknown until the A/B.
  - Risco: For seconds, the GPU runs unoptimized table CS (some read 1,078 slots), at 85-90% GPU load: the GPU-ms cost is unknown. There is one extra unoptimized compile per variant. PromoteOptimized replaces the pipeline in place for every holder (2854-2866); table pairs hold const Pipeline* (pair.pipeline, set->pipeline), so the swap is visible. A session that ends before the optimized build runs saves nothing for that pipeline; the next run builds unoptimized again.
  - A/B: Two builds with tools/local/windows/ab-exe.ps1, or -Set KYTY_ASYNC_FAST_BUILD=0/1. Cold runs (fresh KYTY_DRIVER_CACHE_KEY, empty __GL_SHADER_DISK_CACHE_PATH, KYTY_SHADER_WARMUP=0), 2 per side, same route. Metrics: sum and max of 'SLOW AsyncComputePipeline', table_draws and table_evals in SLOW Frame lines after area entry, and render ms or GPU ms over a 10 s window right after the first boss area loads.
- **foreground-first-driver-compiles** (medium, muda SPIR-V: não): Render-thread compiles first: gate background driver compiles and defer optimized rebuilds out of bursts (only after the contention test confirms)
  - Mudança: pipelineCache.cpp / .h. Add a DriverCompileGate member: a mutex, a condvar, uint32 foreground, uint32 background_in_flight, int64 last_foreground_ns.
- Foreground: in CreateGraphicsPipeline (around 4182-4184) and CreateComputePipeline (4253-4255), a scope increments foreground, stamps last_foreground_ns, then decrements and notifies.
- Background: wrap only the vkCreate* part of these jobs in a scope that waits while foreground != 0 or background_in_flight >= cap (KYTY_BACKGROUND_COMPILES, default 1-2: the driver gives little in-process parallelism, README):
  - the async graphics job (around 4144);
  - the TryCreateComputePipeline job (4312);
  - the BuildOptimized job (2836);
  - the WarmPipelines workers (the CreatePipelineInternal calls around 3253/3265).
- OptimizeWorkers jobs additionally wait until now - last_foreground_ns >= KYTY_OPTIMIZE_QUIET_MS (default 2000), so an optimized rebuild never runs inside a burst like the 51-compile one.
- Table translation jobs on TableWorkers (ProgramCache push_job) are not gated.
- Master switch: KYTY_COMPILE_GATE as a kyty_local_* global in performance-switches.h.
  - Arquivos: src/graphics/host_gpu/renderer/pipeline/pipelineCache.cpp, src/graphics/host_gpu/renderer/pipeline/pipelineCache.h, src/local/performance-switches.h
  - Efeito esperado: - Ceiling, if in-process driver serialization causes the 305 ms unoptimized p50: the 143 sync compiles (46.8 s) fall toward the fork's unoptimized scale (max 26 ms each): about 1-4 s. That is up to -43 s of the 47.3 s of compile-dominated frames.
- If unoptimized compiles are inherently slow on this driver, the gain is about 0. Run the cold contention test in user_actions first. Build this only if run B's cs-normal unoptimized p50 is far below run A's.
- Graphics may gain little (fork note: native graphics variants cost ~100 ms unoptimized or not).
  - Risco: Table, native and optimized builds arrive later during bursts: longer normal path, and unoptimized GPU code for longer. An in-flight background compile cannot be preempted, so the worst wait is one compile. No deadlock: the render thread never waits on background jobs (table-xpr.inc:2573-2580, native-xpr.inc:1909-1918). Starvation of background work lasts at most as long as a burst.
  - A/B: Two builds via ab-exe.ps1, or -Set KYTY_COMPILE_GATE=0/1. Cold runs (fresh KYTY_DRIVER_CACHE_KEY, empty __GL_SHADER_DISK_CACHE_PATH, KYTY_SHADER_WARMUP=0, KYTY_PIPELINE_SOURCE_LOG=1), 2 boots per side, alternating, same route. Metrics: SLOW Frame compile= sum, p50/p90 of 'PIPESRC cs-normal ... build=1 unoptimized=1 ms=', and the longest frame at the first-boss area entry.
- **table-forms-of-guesses** (medium, muda SPIR-V: não): Table forms of the guessed specializations of hinted programs in the precompile
  - Mudança: staticPrecompile.inc, WarmSeeds:
- compile_variant (around 395-401) returns nullptr for any seed without a recorded specialization that binds resources. Instead, for a seed whose program (stage, hash) is in the recorded tier, compile the table form (table=true, byte_base_offset cleared, TableIndirectForm) of each SeedSpecializations() entry.
- Add those permutations to table_first / table_more and create 'cs-table' pipelines in the compute branch (around 760-790).
- Do the same for the table vertex modules of hinted graphics recipes.
Gate it with a new --table-guesses option that precompile-windows.ps1 passes only for the seeds tier.
  - Arquivos: src/graphics/host_gpu/renderer/pipeline/staticPrecompile.inc, src/local/shader-precompile.cpp, precompile-windows.ps1
  - Efeito esperado: - Background only.
- The 84 hinted AsyncCompute table compiles on this route (50.1 s of worker time) get a store hit where the runtime's normal specialization equals a guess. The first proposal makes that the common case: 99.6% for the guessed tier here.
- It removes their contention with render-thread compiles and their normal-path period.
- Cost: table CS compiles are heavy (non-hinted 1,078-slot ones took 15-17 s optimized). Likely tens of minutes more precompile; unknown.
  - Risco: Precompile time and store size. If the table translation of a guess is refused (EnterTableMode), nothing is produced. No runtime change.
  - A/B: As in the first proposal: a .binaries file swap plus cold runs. Metric: count and sum of 'SLOW AsyncComputePipeline ... table' for hinted hashes, and 'PIPESRC cs-table static=1'.
### Ações do usuário sem código
- Play the same route again (second session, no switches) and check the log for: 'Vulkan pipeline cache: loaded 36116165 bytes from _PipelineCache\local\1f3c29b7...\PPSA01342.bin' and the same line for .table.bin (31885284 bytes); 'Shader warmup: loaded N shader inputs and M pipeline recipes' with N, M > 0; and on the tutorial-to-first-boss part, almost no 'SLOW CreateComputePipeline/CreateGraphicsPipeline/Async*' lines and SLOW Frame compile= near 0. 'invalidating ... (driver, emulator, or data mismatch)' means the cache was lost. New areas will still stall as in session 1.
- Always close the game normally and wait for the 'Vulkan pipeline cache: saved ...' lines. The caches save after 20 s without compiles and at exit; a killed process keeps only the last periodic save.
- Do not update the NVIDIA driver (617.42) for now. Both the driver caches (signature includes driverVersion, pipelineCache.cpp:110) and the static .binaries (StaticBinariesSignature) are tied to it, and an update means another hour of precompile.
- Test the main fix today, without code. Copy _PipelineCache\static\PPSA01342_01.005.000.binaries aside, then run: powershell -ExecutionPolicy Bypass -File .\precompile-windows.ps1 -Game F:\PPSA01342-app -Full -Recorded F:\dev\KytyPS5\_Build\static-precompile\released-PPSA01342_01.005.000.seeds. It adds the guessed variants of the 6,997 hinted programs, about 34.8K pipelines, estimated +2.9 GB; it compiles only what is missing; the time is unknown, estimated 10-20 min. Afterwards, a plain 'precompile-windows.ps1 -Game F:\PPSA01342-app' also merges session 1's recording (warmup-v2) into the recorded tier.
- Cold A/B runs (for -Full and for any later code change): run-windows.ps1 -Game F:\PPSA01342-app -Set KYTY_DRIVER_CACHE_KEY=0000000000000000000000000000000000000000000000000000000000000001,__GL_SHADER_DISK_CACHE_PATH=F:\dev\nvcold1,KYTY_SHADER_WARMUP=0,KYTY_PIPELINE_SOURCE_LOG=1. Create the empty folder first, and use a new 64-hex key and folder for each run. This does not touch your real driver cache. Delete _PipelineCache\local\<that key> afterwards.
- One diagnostic session to learn why hinted programs miss: -Set KYTY_PIPELINE_SOURCE_LOG=1,KYTY_PIPELINE_KEY_LOG=1,KYTY_SLOW_LOG_MS=30. Then count 'PIPESRC <kind> ... static=0' per kind (cs-normal, cs-table, gfx-normal, gfx-native, gfx-table) and keep the 'PIPEKEY miss ...' and 'MODULE ...' lines. Compare those keys with: kyty_shader_precompile --game F:\PPSA01342-app --seeds _Build\static-precompile\released-PPSA01342_01.005.000.seeds --dry-run --timings released.timings.txt. A key that is listed there but static=0 means a key mismatch; a missing key means another specialization.
- Contention test (decides the gate proposal), cold runs as above, boot to the tutorial's first 5 minutes. Run A: defaults. Run B: also KYTY_TABLE_DISPATCH=0,KYTY_TABLE_XPR=0,KYTY_NATIVE_XPR=0 (no table or native async compiles). Run C: KYTY_PIPELINE_FAST_BUILD=0. Compare the p50 of 'PIPESRC cs-normal ... ms=' and of SLOW CreateComputePipeline. B much lower than A means background compiles inflate the render thread's compiles. A about equal to C means unoptimized builds gain nothing on this driver.
- Share this PC's specializations for future releases: after a few sessions, run 'precompile-windows.ps1 -Game F:\PPSA01342-app -ExportHints' and attach my-hints-PPSA01342_01.005.000.hints upstream. The shipped 1.05 hints cover only 6,997 programs (1.07: 13,345).

## ÁREA: CPU thread placement and Intel-only tuning on a uniform SMT Ryzen (5700X, 8C/16T, one CCD), plus PGO training for PPSA01342 01.005.000
### Diagnóstico
1) What 'auto' does on the 5700X. run-windows.json:6 sets KYTY_RECORDING_CPUS=auto. run-windows.ps1:281-283 copies it into KYTY_RENDER_CPUS ('auto'). Get-PerformanceCpus returns '' when the CPU sets show fewer than 2 efficiency classes (run-windows.ps1:461), which is the case on Zen 3. Lines 467-473 then remove both keys. The Qt launcher does the same: Platform.cpp:268-273 (performanceCpus) and LaunchPlan.cpp:264-272. The log confirms it: the "Performance switches:" line (session.log:30) has no KYTY_RENDER_CPUS and no KYTY_RENDER_PRIORITY=0. Result: the render thread Kyty.Gpu runs at THREAD_PRIORITY_HIGHEST (performance-switches.h:193-194) but is not pinned (176-179 skipped). The recording worker (vulkan-recording.cpp:259-265) and the upload worker (async-upload.h:126-128) are not pinned either. Nothing keeps any of them off CPU 0 (interrupts/DPCs) or CPU 1 (CPU 0's SMT sibling). The code already reads CoreIndex and drops the whole core of CPU 0 (run-windows.ps1:463-464), but only in the hybrid branch.

2) The evidence behind the Intel tuning has no SMT data. The measured win was 32.5 -> 33.4 fps, render 25.9-26.4 -> 25.0-25.4 ms (docs/CURRENT-PLAYABLE.md:219-220; "+2.5%" in general.md). It was measured on an i9-14900K with hyperthreading off (6 P + 16 E = 22 logical CPUs, docs/OPTIMIZATION-STATUS.md:8-9). The dead ends to respect: CPU 0 halved fps; a dedicated core or CPU-set eviction made lows worse (CURRENT-PLAYABLE.md:56-57).

3) Trap in the helper placement, and in the planned test 005. Every background thread calls AvoidCpuList / MakeBackgroundThread(getenv("KYTY_RENDER_CPUS")). The call sites are pipelineCache.cpp:1342, 1486, 1555, 2500, 2739, 3286, 3577, parallel-pages.h:76 and speculation.cpp:269. local-platform.cpp:828-848 then confines each one to (process & ~render list) unless KYTY_BACKGROUND_CPUS overlaps the process mask. On hybrid Intel the remainder is the E-cores plus CPU 0's core. On the 5700X, KYTY_RENDER_CPUS=2-15 leaves only CPUs 0-1 for these threads:
- TableWorkers 8, Workers 4, OptimizeWorkers 4 (hardware_concurrency/2, /4, /4 at pipelineCache.cpp:2788-2803);
- prefetch 2 + follow 2;
- background warmup 4 (SpareThreads, pipelineCache.cpp:1474-1477);
- 7 Kyty.Pages helpers;
- the table cache saver.

About 30 threads would share one physical core, so async compiles (AsyncCompute median 308 ms today) would queue about 8x longer. With 2,4,...,14 the helpers land on the odd siblings of every render core. So both variants of the planned test 005 (report.md: KYTY_RENDER_CPUS=2-15 and 2,4,...,14) are confounded unless KYTY_BACKGROUND_CPUS=0-15 is also set. This also explains why a uniform 'auto' cannot simply return "all but core 0".

4) Other places where a Ryzen is worse off than an Intel P-core box:
- (a) Priority inversion with no refuge. The game's 13 job workers (guest priority 700, pinned to one core on the console) run at THREAD_PRIORITY_LOWEST: low band -1, then core-bound -2 (kernel/pthread.cpp:120-137). Every emulator compile/prefetch/warmup thread runs one level higher, at BELOW_NORMAL (local-platform.cpp:850-853). On hybrid Intel those threads sit on E-cores, so the job workers keep the P-cores. On the 5700X they share all 16 CPUs. Session data: of 50 SLOW frames dominated by idle time during play (shutdown minute excluded), 22 (5.5 s) overlapped a logged pipeline compile of 100 ms or more and 28 (5.6 s) did not. That is a lower bound (compiles under 100 ms are not logged) and only suggestive.
- (b) Pool sizes count SMT threads. Sixteen compile threads can fill every sibling, the render thread's included. The three CompileWorkers pools have no switch.
- (c) Spin windows are counted in _mm_pause iterations that were tuned on Intel: graphicsRun.cpp:286 (Spins=500, "~20 us"), vulkan-recording.cpp:39 (4000, live global kyty_local_recording_spin), async-upload.h:130 (20000), parallel-pages.h:49 (20000). PAUSE latency depends on the microarchitecture, so how long these windows last on Zen 3 is unknown. Spinning on a sibling also takes issue slots from the render thread.

5) Places that are not worse:
- The precompile: PrecompileSetup::cpuMask returns 0 on a uniform CPU, so it uses all CPUs (Precompile.cpp:52-63), with 6 jobs x 3 threads (precompile-windows.ps1:251). It measured 58:55, against 61 min estimated for a 12900K's 16 P-core threads.
- KYTY_SHADER_WARMUP_THREADS=auto gives 14 (run-windows.ps1:407; log: "14 threads"), but only for the warmup that runs before the game. Beside the game it is SpareThreads=4; the prefetch uses 2 ("translate on 2 threads").
- Guest affinity masks never reach host threads.
- The 5700X has a single L3, so there is no cross-CCX cost.

6) Limits of the data. SLOW lines only cover frames over 100 ms, and there are no fps or render_ms windows. GPU load was 85-90% at 1440p, so many views may be GPU-bound: placement or PGO savings turn into fps only where render_busy is high. 61% of the stall time is pipeline compiles, which placement does not remove.

7) PGO.
- Who trained tools/pgo/kyty.profdata is unknown (shallow history, 6 commits; the file is index version 12, which llvm-profdata 18 rejects). PGO.md:33 records the last retrain on the 16 GB laptop, and demonsSoulsIdle.cpp:18-19 cites a laptop Ryzen 9 5900HX, so "trained on Intel" is not established. The CPU vendor barely matters for instrumented PGO: the counts depend on source, switches, game version, GPU memory class and route.
- What does matter: clang-cl mangles anonymous namespaces as ?A0x<hash of the main source file's full path>. I checked with clang 18 here: a/src/t.cpp gives ?A0x94C88F19, b/src/t.cpp gives ?A0xEFE96D90, and -ffile-prefix-map and -fmacro-prefix-map do not change it. The PGO name is "t.cpp:?f@?A0x94C88F19@@YAHH@Z". The repo profile holds 4,235 such names out of 24,074 mangled names (17.6%, about half of the file-local functions). That includes all 1,890 entries of vulkan-recording.cpp (the recording layer on Kyty.Gpu and Kyty.Record), 57 of pipelineCache.cpp and 26 of renderDraw.cpp. Unless his F:\dev\KytyPS5 path matches the trainer's exactly, these functions get no profile, and the warnings are suppressed (CMakeLists.txt:225-226). Small-VRAM and 1.05-only paths may also be cold in the profile.

8) pgo-train.ps1 on PPSA01342:
- Nothing in it or in bench-run.ps1 is tied to PPSA01341. bench-windows.py:30 swaps PPSA01340/1/2 saves on this branch. The run-windows.ps1:182 PPSA01341 default applies only when game-path.txt is empty.
- Blockers: no baseline save yet. It needs KYTY_BENCH_BASELINE (bench-windows.py:26) plus 'prepare'.
- pgo-train.ps1 has no -Game parameter (it relies on game-path.txt) and hardcodes StartupSeconds=180 (line 19), which may be short for an instrumented exe; bench-run retries from scratch 3 times.
- llvm-profdata is fixed at C:\Program Files\LLVM\bin (line 39). That is his LLVM 19.1.7, so it works.
- The profile path is parsed with sscanf %255s (live-control.h:357), so it must contain no spaces; his path is fine.
- The result must be copied to _Build\pgo\windows\kyty.profdata (build-windows.cmd:25-28), followed by a clean rebuild.
- Child 'powershell -File keys.ps1' calls should inherit Bypass through PSExecutionPolicyPreference; not verified on his PC.
### Propostas
- **uniform-cpu-auto-placement** (small, muda SPIR-V: não): 'auto' on CPUs whose cores are all alike: render/recording/upload threads on every CPU except CPU 0's core, background threads on every CPU
  - Mudança: run-windows.ps1:431-475: replace Get-PerformanceCpus with Get-CpuPlacement($allowed). The C# Query() also returns LastLevelCacheIndex (byte offset 16 of SYSTEM_CPU_SET_INFORMATION, next to 14 LogicalProcessorIndex, 15 CoreIndex, 18 EfficiencyClass); rename the type (e.g. Kyty.CpuSets2) so it cannot clash with an already loaded definition. Rules: (1) at least 2 efficiency classes over all sets (hybrid): exactly today's result, Render = fastest class without CPU 0's core, Background = ''. (2) Uniform, the allowed CPUs share one LLC, and at least 2 cores remain without CPU 0's core: Render = every allowed logical CPU whose CoreIndex differs from CPU 0's (both SMT siblings of core 0 out; on the 5700X 2,3,...,15), Background = every allowed CPU (0,...,15). (3) Uniform with several LLCs (5900X/5950X/X3D) or fewer than 2 cores left: '' for both, as today (not measured). In the auto block (467-473): resolve the keys from Render as now, and if Background is set and the config does not set KYTY_BACKGROUND_CPUS, set it. AvoidCpuList (local-platform.cpp:838-841) then gives background threads process & background = all CPUs instead of CPUs 0-1. Show it in the affinity line (480). Launcher: Platform.h/Platform.cpp, CpuSet gains llc (cpuSets(), Platform.cpp:249: info->CpuSet.LastLevelCacheIndex); performanceCpus becomes CpuPlacement cpuPlacement(quint64 allowed) returning {render, background} with the same three rules (Linux keeps {}); LaunchPlan.h:31-32 takes std::function<Platform::CpuPlacement(quint64)>; LaunchPlan.cpp:264-272 sets KYTY_BACKGROUND_CPUS with setIfMissing when background is non-empty, and shows it next to the render CPUs (62-65); MainWindow.cpp:335,487 and DryRun.cpp:27 point to Platform::cpuPlacement. No emulator change: the render thread stays non-exclusive (one SetThreadAffinityMask, no CPU sets for other threads) at HIGHEST; recording and upload workers follow KYTY_RECORDING_CPUS as on Intel.
  - Arquivos: run-windows.ps1, src/fork-launcher/src/platform/Platform.h, src/fork-launcher/src/platform/Platform.cpp, src/fork-launcher/src/core/LaunchPlan.h, src/fork-launcher/src/core/LaunchPlan.cpp, src/fork-launcher/src/ui/MainWindow.cpp, src/fork-launcher/src/core/DryRun.cpp
  - Efeito esperado: Applies the one measured placement win (render off CPU 0, non-exclusive: 32.5 -> 33.4 fps, render -0.9 ms on the HT-off 14900K, CURRENT-PLAYABLE.md:219-220) to the 5700X and to every uniform CPU (also Intel parts without E-cores), without boxing background threads in. On the user's PC it helps only views where the render thread bounds the frame (render_busy near 95%); with GPU load at 85-90%, likely between 0% and about +3% average fps, and possibly steadier 1% lows. It does not touch the compile stalls (61% of the 77.5 s). The size on SMT Zen 3 is unknown until measured.
  - Risco: Low. Hybrid output is byte-identical (same branch). Uniform multi-LLC CPUs keep today's behavior. Risk on the 5700X: the scheduler loses CPUs 0-1 for 3 threads, which matters only if the interrupt load there is negligible; the A/B decides. Background threads keep today's freedom, and with Background = all, SetThreadSelectedCpuSets selects every CPU, which is harmless.
  - A/B: Before the code change, the same exe with launch switches through ab-exe.ps1: -A <exe> -B <same exe> -SetB 'KYTY_RECORDING_CPUS=2-15,KYTY_RENDER_CPUS=2-15,KYTY_BACKGROUND_CPUS=0-15' -Rounds 3 -StartupSeconds 240 (ABBA; cross-boot noise is +-3-5%, so compare render_ms and low1 per window). Same-process check of the render thread alone: start with -Fps120 -Set KYTY_LIVE_FILE=<repo>\_Build\windows-bench\live-commands.txt, then python tools\local\bench-windows.py live "pin 2-15 ; measure 8 b1 ; pin 0-15 ; measure 8 a1 ; pin 2-15 ; measure 8 b2 ; pin 0-15 ; measure 8 a2 ; pin 2-15 ; measure 8 b3 ; pin 0-15 ; measure 8 a3" at a fixed standing spot with warm caches. Keep it only if every b window beats every a window in render_ms. After the change: run-windows.ps1 -DryRun lists KYTY_RENDER_CPUS=2,...,15 and KYTY_BACKGROUND_CPUS=0,...,15; A side -Set KYTY_RECORDING_CPUS=,KYTY_RENDER_CPUS=,KYTY_BACKGROUND_CPUS= (today's behavior).
- **avoid-list-floor** (small, muda SPIR-V: não): AvoidCpuList never boxes background threads into less than a quarter of the CPUs
  - Mudança: src/local/local-platform.cpp:842 (Windows AvoidCpuList): in the else-if branch, set mask = process & ~avoid only when popcount(process & ~avoid) * 4 >= popcount(process). Otherwise leave the thread unrestricted, and print once with std::call_once, e.g. "KYTY_RENDER_CPUS=%s leaves background threads %d of %d CPUs: not restricted (set KYTY_BACKGROUND_CPUS)". The KYTY_BACKGROUND_CPUS branch (838-840) is unchanged. Same check with CPU_COUNT in the Linux variant (local-platform.cpp:1854-1861). Callers stay as they are: pipelineCache.cpp MakeBackgroundThread sites, parallel-pages.h:76, speculation.cpp:269.
  - Arquivos: src/local/local-platform.cpp
  - Efeito esperado: Nothing changes in the default configs: on hybrid CPUs the remainder is 37.5% (12600K/13400: 6 of 16), 56% (14900K with HT: 18 of 32) and 77% (author's 22-CPU box), so the guard never fires; with proposal 1, KYTY_BACKGROUND_CPUS applies first. It protects hand-set lists on uniform CPUs, such as the planned test 005 KYTY_RENDER_CPUS=2-15: background threads would get 2 of 16 logical CPUs (12.5%), i.e. about 16 compile threads plus prefetch, warmup and 7 page helpers on one physical core, which means about 8x longer async compile queues (AsyncCompute median 308 ms) and slower ParallelPages copies on the render thread.
  - Risco: Very low. With a deliberately tiny background set the user must now name KYTY_BACKGROUND_CPUS explicitly. Needs a rebuild of the emulator only.
  - A/B: Functional check, not a performance A/B: build, run -Set KYTY_RENDER_CPUS=2-15,KYTY_RECORDING_CPUS=2-15 and confirm the one-time log line; tools\local\windows\threads.ps1 during an area entry should show Kyty.Warmup/Prefetch/compile threads using CPU above 200% of one core in total (impossible when boxed into CPUs 0-1). Optional two-build comparison: ab-exe.ps1 -A <old exe> -B <new exe> -Set 'KYTY_RENDER_CPUS=2-15,KYTY_RECORDING_CPUS=2-15', counting SLOW Async*Pipeline lines and their total ms.
- **background-compile-switches** (small, muda SPIR-V: não): Switches for the background pipeline pools' size and priority (defaults unchanged) to test the inversion with the game's LOWEST job workers
  - Mudança: (1) src/local/local-platform.cpp:850-853 MakeBackgroundThread: the priority is read once from KYTY_BACKGROUND_PRIORITY (-1 default = THREAD_PRIORITY_BELOW_NORMAL, -2 = THREAD_PRIORITY_LOWEST, 0 = NORMAL; other values ignored). (2) src/graphics/host_gpu/renderer/pipeline/pipelineCache.cpp:2788, 2796, 2803: the thread counts of Workers() and OptimizeWorkers() come from KYTY_PIPELINE_WORKERS and those of TableWorkers() from KYTY_TABLE_PIPELINE_WORKERS (clamp 1..64), defaulting to today's max(2, hardware_concurrency()/4) and max(2, hardware_concurrency()/2). Both appear in the 'Performance switches' line when set. Defaults stay as they are; keep or delete after the A/B, as the method requires.
  - Arquivos: src/local/local-platform.cpp, src/local/local-platform.h, src/graphics/host_gpu/renderer/pipeline/pipelineCache.cpp
  - Efeito esperado: Unknown, possibly zero. Mechanism: kernel/pthread.cpp:120-137 puts the 13 core-bound job workers at LOWEST, below the BELOW_NORMAL compile/prefetch/warmup threads. On hybrid Intel those threads are kept off the P-cores, but on the 5700X they share all 16 CPUs, so a compile burst can preempt the game's job work and leave the render thread idle. Upper bound from the session: 22 idle-dominated slow frames (5.5 s of the 77.5 s stalled) overlapped a logged compile of 100 ms or more. Counter-risk: slower async compiles keep table draws and dispatches on the normal path longer and can add synchronous CreateCompute/CreateGraphics time (the table path falls back, table-xpr.inc:694, 2521).
  - Risco: Low as switches (off by default). Wrong values can lengthen compile queues or starve compiles; the defaults stay until a cold-cache A/B shows a win.
  - A/B: Cold-cache cross-boot on the same route (the tutorial walk), at least 2 boots per side, each boot with a fresh 64-hex -Set KYTY_DRIVER_CACHE_KEY=<new> and an empty -Set __GL_SHADER_DISK_CACHE_PATH=<new dir>, and the static cache left as is. Side A: defaults. Side B: -Set KYTY_BACKGROUND_PRIORITY=-2. Further B variants: KYTY_TABLE_PIPELINE_WORKERS=4 / 12. Compare total SLOW Frame ms, the idle-dominated frames and their idle ms, the sum of compile=, and the count and median of SLOW Async*Pipeline lines; then a warm standing 'measure' to confirm no regression. A crude no-build probe exists (live 'prio 2 old' vs 'prio 2 low', kernel/pthread.cpp:3549), but it also raises guest threads with priority 478 or lower to HIGHEST, so it is confounded.
- **pgo-local-training** (small, muda SPIR-V: não): Train PGO on this PC and tree (pgo-train.ps1 that works for PPSA01342; profile names that survive another checkout path)
  - Mudança: tools/local/windows/pgo-train.ps1:
- param(...) gains [string]$Game='', [int]$StartupSeconds=300 and [switch]$NoPrecompile, all forwarded into $params for bench-run.ps1 (bench-run.ps1:150 already accepts -Game; line 19 hardcodes 180 today).
- Before the 5-minute instrumented build, run 'python tools\local\bench-windows.py reset' and exit with the hint '$env:KYTY_BENCH_BASELINE=<dir>; python tools\local\bench-windows.py prepare' when it fails.
- Refuse an -Out with spaces (live 'pgo' is parsed with sscanf %255s, live-control.h:357).
- Take llvm-profdata.exe from the directory of CMAKE_CXX_COMPILER in _Build\windows-pgogen\CMakeCache.txt, falling back to C:\Program Files\LLVM\bin (line 39).
- Print the install steps at the end: copy to _Build\pgo\windows\kyty.profdata (build-windows.cmd:25-28 prefers it), 'ninja -C _Build\windows -t clean', build-windows.cmd.

tools/local/PGO.md (Windows): train in the tree you build in. clang-cl names anonymous-namespace functions by a hash of the source file's full path (?A0x...), which file/macro prefix maps do not change; a profile from another path silently drops those functions (17.6% of the profile's names, all of vulkan-recording.cpp).

Optional for releases: configure both the training build and tools\release\build-release.cmd from one fixed path (e.g. a junction C:\KytyPS5 to the checkout, cmake -S C:\KytyPS5), and confirm with llvm-nm that the ?A0x hashes match the profile's (whether CMake keeps the junction path is unknown).
  - Arquivos: tools/local/windows/pgo-train.ps1, tools/local/PGO.md, build-windows.cmd, tools/release/build-release.cmd
  - Efeito esperado: Measured retrains: 43.83 -> 45.17 fps (+3.1%, PGO.md:30-31); 10-16 laptop render 33.60 -> 32.00 ms (-4.8%, PGO.md:33-38); general.md: '+3% per retrain, a stale profile loses it'. On his PC it also restores profile data for the anonymous-namespace functions, if his path differs from the trainer's (checkable today, see user actions), and trains the 1.05, small-VRAM and RTX 3060 Ti paths. It converts to fps only in render-bound views (GPU load is 85-90%), so the realistic gain is about 0-5% render ms where CPU-bound. Unknown until measured.
  - Risco: Low. The training run uses the baseline save (prepare/restore with hash checks), never the player's save. Takes about 5 min to build the instrumented exe, plus one guided run and a clean rebuild. The profile becomes stale after big source changes (perf/* branches), so retrain then. Instrumented-run fps means nothing.
  - A/B: Keep a copy of the current exe (with libwinpthread-1.dll) in _Build\ab\base\, rebuild with the new profile, then tools\local\windows\ab-exe.ps1 -A _Build\ab\base\kyty_emulator.exe -B _Build\windows\kyty_emulator.exe -Label pgo -Rounds 2 -StartupSeconds 240 (ABBA). Accept only if every B window has lower render_ms than every A window at the same spot. The translation store and caches stay valid because KYTY_RECOMPILER_SIGNATURE is hashed from the recompiler sources (src/generate_version.cmake:41-65).
### Ações do usuário sem código
- Before running test 005, fix its switches. KYTY_RENDER_CPUS alone boxes every background thread (pipeline compiles, prefetch, warmup, 7 page helpers) into CPUs 0-1 (local-platform.cpp:828-848). Use: run-windows.ps1 -Game F:\PPSA01342-app -Set KYTY_RECORDING_CPUS=2-15,KYTY_RENDER_CPUS=2-15,KYTY_BACKGROUND_CPUS=0-15, and for the even variant KYTY_RECORDING_CPUS=2,4,6,8,10,12,14,KYTY_RENDER_CPUS=2,4,6,8,10,12,14,KYTY_BACKGROUND_CPUS=0-15. Check with -DryRun, and confirm the log's 'Performance switches:' line contains KYTY_RENDER_CPUS=...
- Same-process A/B of the render thread off core 0, no rebuild. Do it on the second session or later (driver cache warm; the first run saved 31 MB), at a fixed standing spot. Start: run-windows.ps1 -Game F:\PPSA01342-app -Fps120 -Set KYTY_LIVE_FILE=F:\dev\KytyPS5\_Build\windows-bench\live-commands.txt. Then: python tools\local\bench-windows.py live "pin 2-15 ; measure 8 b1 ; pin 0-15 ; measure 8 a1 ; pin 2-15 ; measure 8 b2 ; pin 0-15 ; measure 8 a2 ; pin 2-15 ; measure 8 b3 ; pin 0-15 ; measure 8 a3". Compare render_ms, render_busy and low1. If render_busy is well under ~90% at that spot, the spot is GPU/game-bound: pick another. Optional: also pin Kyty.Record with 'pinthread <tid> 2-15' (tid from tools\local\windows\threads.ps1).
- Check whether the PAUSE spin counts tuned on Intel matter here, live in the same process: python tools\local\bench-windows.py live "set kyty_local_recording_spin 2000 ; measure 8 s2k ; set kyty_local_recording_spin 4000 ; measure 8 s4k ; set kyty_local_recording_spin 8000 ; measure 8 s8k ; set kyty_local_recording_spin 4000 ; measure 8 s4k2". 'set' resolves the symbol from _Build\windows\kyty_emulator_*.map; 4000 is the default. If render_ms and record_ms do not move, the spin-calibration idea is dead.
- Check whether his exe even uses the repo profile for anonymous-namespace code. First: & 'C:\Program Files\LLVM\bin\llvm-profdata.exe' show --all-functions tools\pgo\kyty.profdata | Select-String 'Wrapped_vkSignalSemaphore' (the profile has ?A0xF397ABF2). Then: Get-ChildItem F:\dev\KytyPS5\_Build\windows -Recurse -Filter vulkan-recording.cpp.obj | ForEach-Object { & 'C:\Program Files\LLVM\bin\llvm-nm.exe' $_.FullName } | Select-String 'Wrapped_vkSignalSemaphore'. A different ?A0x hash means ~4.2K functions (all of the Vulkan recording layer) run without PGO in his build.
- Train PGO today with the existing script. (1) Make the 1-1 benchmark save. (2) $env:KYTY_BENCH_BASELINE='F:\dev\bench-save' (a folder with PPSA01342\ inside), then python tools\local\bench-windows.py prepare. (3) Make sure game-path.txt holds F:\PPSA01342-app (pgo-train has no -Game). (4) Copy _Build\windows\kyty_emulator.exe and libwinpthread-1.dll to _Build\ab\base\. (5) powershell -ExecutionPolicy Bypass -File tools\local\windows\pgo-train.ps1 (StartupSeconds is fixed at 180; if bench-run reports no HUD, the instrumented start-up was too slow). (6) Copy _Build\pgo\windows\kyty-<date>.profdata to _Build\pgo\windows\kyty.profdata, run ninja -C _Build\windows -t clean, then build-windows.cmd. (7) tools\local\windows\ab-exe.ps1 -A _Build\ab\base\kyty_emulator.exe -B _Build\windows\kyty_emulator.exe -Label pgo -StartupSeconds 240. (8) python tools\local\bench-windows.py restore. No shader re-precompile is needed.
- Keep one Windows power plan for all measurements (report: Balanced). Optionally compare High performance across boots (at least 2 per side, cross-boot noise +-3-5%); the effect on Zen 3 wake-ups is unknown. Do not run measurements while compiling or precompiling, and use the 60 Hz monitor setting the report already plans.

## ÁREA: Video memory on an 8 GB card (RTX 3060 Ti, Windows/NVIDIA). Covers the small-VRAM policy, every budget figure, texture placement in the "plain" type, the texture and buffer collectors, swapping images to system memory, the pipeline-warmup cap, driver memory, and what happens past the OS budget.
### Diagnóstico
VERDICT: The user probably loses frame time to VRAM pressure. It is the second-largest stall source after pipeline compiles (47.3 s), but this is not proven. Estimate: about 4-5 s of extra non-compile stalls while over budget mid-session, plus the last 11.9 s of the log (about 7 fps). The effect on average fps is unknown, because frames under 100 ms are not logged.

WHAT THE CODE DOES ON THIS PC
1) Small-card detection. vma.cpp:147-163 marks any largest device-local heap under 11.5 GiB as small. Log: "Video memory: largest device-local heap 8024 MiB, budget 6349 MiB: treated as small (textures may leave video memory past half the budget)".
   - 6349 comes from GetTotalMemoryBudget (vma.cpp:288-330): OS budget 7256 - min(budget/8, 1 GiB) = 907 - KYTY_VRAM_RESERVE_MB.
   - KYTY_VRAM_RESERVE_MB was unset. run-windows.ps1:370-373 sets 512 only with -FsrFrameGen.
2) Values derived from 6349:
   - Image pool: min(1 GiB, budget/16) = 396 MiB (vma.cpp:168; KYTY_IMAGE_POOL=1 in run-windows.json).
   - Pipeline-warmup compute cap: budget/16 (pipelineCache.cpp:3030-3038). Log: "Pipeline warmup: compute pipelines up to 396 MiB of video memory". It had no effect this run: "Shader warmup: loaded 0 shader inputs and 0 pipeline recipes".
   - Texture collector static levels, trigger/pressure/critical: 3789 / 4813 / ~5735 MiB (textureCache.cpp:349-352).
   - Buffer collector trigger/critical: 4813 / ~5735 MiB (bufferCache.cpp:1472-1487).
3) Texture placement (vma.cpp:470-481):
   - An image that is only sampled or copied (graphicContext.h:183-186) goes to the driver's flagless "plain" type once VMA heap-0 usage reaches GetTotalMemoryBudget()/2, about 3.05-3.17 GB.
   - NVIDIA counts that type under heap 0 and keeps it in VRAM while there is physical room (comments vma.cpp:470-476 and streamBuffer.cpp:177-181).
   - Plain-type memory in VRAM grew from 0 to 3065 MiB: "Video memory (flip 34560): 7571 MiB in use of 7122 MiB (VMA device-local 4014 MiB, plain type 3065 MiB in video memory, driver 490 MiB)".
   - So the half-budget rule did not keep the process under the OS budget. Textures left VRAM only when the physical 8 GB ran out.
   - Nothing has a residency priority. VK_EXT_memory_priority is enabled only for KYTY_SIMULATE_VRAM_MB (vulkanWindow.cpp:1469-1474 and 1667-1673; graphicContext.h:35-36), and VMA is created without its priority flag (vma.cpp:74-77). When over budget, the OS and driver choose what to demote, render targets included.
4) Collectors.
   - Their input is VMA heap-0 usage (textureCache.cpp:4457-4459, bufferCache.cpp:2821-2823, vma.cpp:248-266).
   - Texture live levels (kyty_local_texture_gc_critical_mb=0): critical = max(OS budget - 256 MiB, 5735), about 6.7-7.0 GB. Pressure = critical - 900 MiB, about 5.8-6.1 GB (textureCache.cpp:4527-4538).
   - At pressure: the image pool is trimmed (4641). Images idle 90 frames are evicted, 20 per pass. GPU-written tiled images are swapped to host memory, up to 64 MiB per flip and 3072 MiB in total (KYTY_GC_SWAP / KYTY_GC_SWAP_MB, 238-253 and 4588-4600).
   - At critical: images idle 30 frames are evicted, 80 per pass, and textures that came back are no longer kept (4552-4566).
   - Buffers at critical: idle 32 frames. Buffers that came back are kept only below the OS budget (bufferCache.cpp:2867-2878).
   - KYTY_VRAM_RESERVE_MB moves only the static levels. The live texture levels take max(live, static), so the reserve does not move them. It does make buffer collection and plain placement start earlier.
   - VMA's usage is not the OS's. At the failure the log shows "heap 0: usage 9891 MiB of budget 7047 MiB" while DXGI said 7939 MiB. The gap of +1952 MiB is close to the 1939 MiB plain-in-VRAM estimate, and DXGI shared memory stayed at 1546 MiB.
   - So the collectors may have run at their critical level for most of play, evicting and re-uploading. Whether they did is unknown: their 10-second inventory line (textureCache.cpp:4482-4506: levels, deletions, swap-outs) is written with LOGF, which run-windows.json:62-63 ("--printf-direction", "Silent") silences.
5) TextureReduce is off by default in code: descriptors.cpp:579-584 returns 0 unless set; the comment at 572-578 says the fork keeps it opt-in. The run-windows.ps1 help text (39-42) and comment (374-376) saying "default 1 below 12 GB" are stale. This session ran without it: no "Textures: large block-compressed textures held without" line. KYTY_TEXTURE_PREFIX is on by default on this card (descriptors.cpp:559-570).
6) Driver memory and pipelines. The unoptimized pipelines that are replaced stay alive until the cache is destroyed (pipelineCache.cpp:2852-2866; destroyed at 3352). The "driver" part of video memory grew from 68 to 653 MiB over the session. How much of that is these pipelines is unknown.

MEASURED IN THE SESSION LOG
- 37 VRAM samples; 7 are over budget, by +11, +76, +309, +355, +449, +911 and +1196 MiB.
- The OS budget moved between 6976 and 7256 MiB.
- From 23:19:51 to 23:30:55, every sample is within about 400 MiB of the budget or above it.
- Play from 23:19:14 to 23:34:01 (888 s), banded by the last sample. Samples are logged only on 256 MiB moves, so the bands are approximate:

| Band | Play time | Non-compile SLOW frames | Stall per minute |
|---|---:|---:|---:|
| Under budget | 162 s | 8 (1.1 s) | 0.41 s |
| Near (within 300 MiB under) | 527 s | 27 (5.4 s) | 0.62 s |
| Over budget | 199 s | 31 (5.8 s) | 1.75 s |

- Extra stall over budget: (1.75 - 0.41) x 199 / 60 ≈ 4.4 s.
- Then the failure. Eight lines of "Vulkan: video memory full, an image is in system memory (36044800..40632320 bytes)" with "heap 0: usage 9891 MiB of budget 7047 MiB", rising to 10568. Only the first 8 failures are printed (vma.cpp:390).
- Next frame: "SLOW Frame 3679.6 ms idle=0.0 cpu=1921.1 ... full_upload_bytes=1660289024".
- Then 57 frames in a row averaging 144 ms (idle 119 ms, render CPU 21 ms, 9.5-14.3K table draws), about 7 fps, until the log ends 8.2 s later: "Video memory (flip 58560): 8250 MiB in use of 7054 MiB".
- The user calls that minute the game closing. Whether it was a menu or a level load is unknown, but the scene was being drawn.
- The render thread was idle, waiting for the game, which waits on GPU work. That fits GPU paging, but there is no GPU-time data. GPU load was 85-90% in play, so the game is likely GPU-bound at 1440p and paging adds directly to frame time.
### Propostas
- **vram-collector-line** (small, muda SPIR-V: não): Show the collectors' view of video memory in the default log
  - Mudança: In TextureCache::RunGarbageCollector (textureCache.cpp:4510-4525), wherever the existing 'Video memory (flip N)' line is printed, add a second line with std::printf (not LOGF, which run-windows.json silences). Print it only on small cards, when usage moves by 256 MiB or more. It should give:
- VMA heap-0 usage and budget (vmaGetHeapBudgets), shown beside the DXGI usage and budget;
- the texture level reached (below trigger / trigger / pressure / critical) and the three live levels computed at 4527-4538;
- since the last line, from existing members: m_gc_deletions, m_gc_swapouts/m_gc_swapout_bytes, m_gc_restores, m_gc_swap_stale, m_swapped_bytes, m_buffer_cache.GcDeletions(), m_buffer_cache.Registrations();
- the buffer collector's level.
In GraphicContext::ReportMemoryFallback (vma.cpp:388-399), keep counting after the 8 printed lines and print the running count and bytes in each later line, at most one per second.
  - Arquivos: src/graphics/host_gpu/renderer/cache/textureCache.cpp, src/graphics/host_gpu/vma.cpp
  - Efeito esperado: No fps change: about 37 extra lines a session, none per frame. It answers three open questions:
- whether the collectors ran at their critical level most of the time (VMA usage was 1952 MiB above DXGI at the failure);
- how much they evicted, swapped and re-registered per interval;
- how many images actually went to system memory (only 8 were printed).
The gap between VMA and DXGI usage at low usage, such as 5 GB, tells double counting of the plain type (gap ≈ plain-in-VRAM at all times) from evicted memory (gap only near the physical limit). That decides whether the os-usage proposal is right.
  - Risco: Very low: log output only, at the existing cadence.
  - A/B: None needed. The user plays the same route once with the build and sends the .out.log. Check that the VMA-vs-DXGI gap is logged at every sample.
- **texture-reduce-auto** (medium, muda SPIR-V: não): Drop the top mip of new large BC textures only while the card is near its OS budget (KYTY_TEXTURE_REDUCE=auto)
  - Mudança: 1. descriptors.cpp TextureReduceLevels (579-593): accept 'auto'. In auto mode, return kyty_local_texture_reduce_now.load(relaxed) on every call instead of the cached static. Values 0, 1 and 2 behave as today.
2. Add volatile std::atomic<uint32_t> kyty_local_texture_reduce_now in textureCache.cpp. Update it once per flip in TextureCache::RunGarbageCollector (next to the QueryLocalVideoMemory call at 4510), using GraphicContext::QueryLocalVideoMemory (vma.cpp:212-218, DXGI usage and budget):
   - set it to 1 when usage + 512 MiB > budget;
   - clear it when usage + 1280 MiB < budget;
   - print one line on each change.
3. Keep every guard in HoldFromBaseLevel unchanged (descriptors.cpp:597-635): BC only, at least 1024 texels on a side, not GPU-written, no GPU-written overlap.
4. A reduced image is superseded by the full one through the existing prefix path once the game samples a texture that is no longer reduced (descriptors.cpp:559-565).
5. Optional second step: while the flag is set and the collector is at its pressure level, evict idle large BC textures that are held whole (not GPU-modified), so they come back reduced.
Make auto the default on small cards only after the A/B.
  - Arquivos: src/graphics/host_gpu/renderer/pipeline/descriptors.cpp, src/graphics/host_gpu/renderer/cache/textureCache.cpp, run-windows.ps1
  - Efeito esperado: The ceiling is what -TextureReduce 1 does all the time:
- at 1-1 on a simulated 8 GB card, textures 4.1 -> 2.8 GiB and 1% low 37-49 -> 53-58 fps (run-windows.ps1:375-376);
- BC textures of 1024 texels and up: 3.7 -> 0.9 GiB (descriptors.cpp:576-577);
- per texture: a 2048² BC7 with mips goes from 5.33 to 1.33 MiB.
This session overshot the budget by 11-1196 MiB, so partial engagement should be enough to stay within about 300 MiB of the budget. Non-compile stalls ran 0.41 s/min under budget and 0.62 s/min near it, against 1.75 s/min over it plus the 8 s tail at about 7 fps. The visual cost (half resolution per side) only hits large textures first bound while the card is full; the rest of the session stays at full detail.
  - Risco: - Size flip-flop and re-uploads if usage hovers at the threshold; the 768 MiB hysteresis, and changing only new images, limit it.
- For a while one texture can be held at two sizes, costing memory briefly.
- Table-mode image sets keep the reduced image while it is held.
- A texture written partly on the GPU is excluded by the existing guards.
  - A/B: Two builds are not needed: the switch is read at start. Run two boots per side over the same route (tutorial from the start to the first boss), alternating:
- -Set KYTY_TEXTURE_REDUCE=auto
- -TextureReduce 0
- also -TextureReduce 1 as the static reference.
Compare: the count and size of over-budget 'Video memory (flip N)' samples; 'Vulkan: video memory full' lines; non-compile SLOW Frame seconds per minute; LIVE_MEASURE fps/low1 at a fixed spot via KYTY_LIVE_FILE; a screenshot of a close wall. To confirm no SPIR-V change, the SLOW Create*/Async*Pipeline count must not rise against the baseline at the same spots.
- **collector-os-usage** (small, muda SPIR-V: não): Collectors measure the same usage the OS budget is about (DXGI on Windows)
  - Mudança: 1. Add GraphicContext::GetCollectorMemoryUsage(uint64_t* budget) in vma.cpp. When the new switch kyty_local_gc_os_usage is 1 and QueryLocalVideoMemory succeeds (vma.cpp:212-218), it returns DXGI CurrentUsage and Budget; otherwise VMA's GetDeviceMemoryUsage and GetDeviceMemoryBudget.
2. Use it in TextureCache::RunGarbageCollector: for m_total_used_memory at 4457-4459, and for device_budget in the live critical level at 4531.
3. Use it in BufferCache::RunGarbageCollector: m_total_used_memory at 2821-2823 and the keep_came_back budget test at 2878.
4. Leave the plain-placement rule at vma.cpp:478 on VMA's number, as designed.
5. Register KYTY_GC_OS_USAGE in performance-switches.h so -Set works and the value can be poked live. Default 0 until the A/B.
  - Arquivos: src/graphics/host_gpu/vma.cpp, src/graphics/host_gpu/graphicContext.h, src/graphics/host_gpu/renderer/cache/textureCache.cpp, src/graphics/host_gpu/renderer/cache/bufferCache.cpp, src/local/performance-switches.h
  - Efeito esperado: Today the collectors compare VMA's heap-0 usage with the OS budget (VMA's budget 7047 vs DXGI's 7028). NVIDIA inflates that usage with the flagless type: streamBuffer.cpp:177-181 says so, and at the failure VMA read 9891 MiB against DXGI's 7939. If the gap holds through play, the collectors sat at their critical level whenever DXGI read above about 5 GB, which was most of the 888 s of play:
- textures idle 30 frames evicted, 80 per pass, textures that came back not kept;
- buffers idle 32 frames, buffers that came back not kept above budget.
The fork measured keeping textures that came back at a 1% low of 13-17 -> 21-31 fps and keeping buffers at +7.2%. With DXGI as the input, collection would stay below critical for the 162 s under and 527 s near the budget. The gain is unknown until vram-collector-line shows the gap and the eviction rate.
  - Risco: If VMA's surplus is evicted memory rather than double counting, the DXGI number underestimates real demand. The collectors would then hold more and paging would rise. Gate this on vram-collector-line, and stop if over-budget samples or 'video memory full' lines increase.
  - A/B: Same process. Set KYTY_LIVE_FILE, stand at a spot where DXGI reads 6.0-7.0 GB, and poke kyty_local_gc_os_usage 0/1 in four to six alternating rounds of 10 s (tools/local/live-bench.py 'sym'). Compare in LIVE_MEASURE and LIVE_COUNTERS: fps, low1, gc_image_deletes, gc_buffer_deletes, full_upload_bytes and buffer_registrations per frame. Then one route per side cross-boot (-Set KYTY_GC_OS_USAGE=1 against unset), comparing over-budget samples and non-compile SLOW seconds per minute.
- **residency-priorities** (medium, muda SPIR-V: não): Let the driver demote cold textures first: VK_EXT_memory_priority and pageable device-local memory
  - Mudança: 1. In vulkanWindow.cpp:1469-1474 and 1667-1673, enable VK_EXT_memory_priority whenever the device exposes it, not only under KYTY_SIMULATE_VRAM_MB. Also enable VK_EXT_pageable_device_local_memory with its pageableDeviceLocalMemory feature when present.
2. In vma.cpp:74-77, add VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT.
3. Create two VMA custom pools in CreateAllocator; VMA's default pools have a fixed priority of 0.5 and ignore the per-allocation value unless dedicated:
   - a texture pool on the flagless type, priority 0.1;
   - a pool for render targets, depth and storage images on the device-local type, priority 1.0.
4. In GraphicContext::CreateImage (vma.cpp:466-481), route images to these pools.
5. With pageable memory, also call vkSetDeviceMemoryPriorityEXT on texture blocks whose images the texture collector has seen idle past the pressure age.
Switch: KYTY_MEMORY_PRIORITY=0/1, read at device creation.
  - Arquivos: src/graphics/presentation/window/vulkanWindow.cpp, src/graphics/host_gpu/vma.cpp, src/graphics/host_gpu/graphicContext.h
  - Efeito esperado: Unknown. When the process exceeds the OS budget, which it did for about 199 s plus the 8 s tail, the aim is that the driver or OS pages out cold flagless textures (a slower sample now and then) rather than render targets and buffers (GPU stalls every frame). It only works if NVIDIA 617.42 on Windows exposes and honours these extensions; check with vulkaninfo first.
  - Risco: - The extra pools fragment memory: more 64 MiB blocks, slightly more VRAM.
- The driver may ignore priorities, giving no gain.
- Pageable memory changes driver behaviour globally.
No correctness risk to rendering.
  - A/B: Cross-boot over the same route, two boots per side: -Set KYTY_MEMORY_PRIORITY=1 against unset. Compare non-compile SLOW Frame seconds per minute while over budget (the band table from the log), the tail after the first 'video memory full', and LIVE_MEASURE fps/low1 at a spot pushed over budget.
- **free-replaced-pipelines** (medium, muda SPIR-V: não): Destroy unoptimized pipelines once their optimized replacement is in use and the GPU is done with them
  - Mudança: In PipelineCache::PromoteOptimized (pipelineCache.cpp:2855-2870), the old VkPipeline is pushed into m_replaced_pipelines and destroyed only with the cache (3352; comment 2852-2854). Instead:
1. Hand it to a deferred destroy that runs after the GPU has finished every command buffer that could reference it. Use CommandScheduler::DeferOperation (commandScheduler.h:50; also used for buffers in TextureCache::RestoreSwapped) on the submission that follows the swap.
2. Before deferring, make sure the Vulkan recording worker (KYTY_VULKAN_RECORDING) and deferred submits have no packet still holding the old handle: drain to the current recording serial, or tag the operation with it.
3. Keep the old behaviour behind KYTY_FREE_REPLACED_PIPELINES=0.
  - Arquivos: src/graphics/host_gpu/renderer/pipeline/pipelineCache.cpp, src/graphics/host_gpu/renderer/pipeline/pipelineCache.h
  - Efeito esperado: Frees the driver's copy (VRAM code and RAM) of every unoptimized pipeline that was replaced. The 'driver' part of video memory grew from 68 to 653 MiB in this first session; the share due to these pipelines is unknown. Measure it first:
- live 'objects' (LIVE_PIPELINE_OBJECTS replaced_unoptimized=N) against the driver MiB in the 'Video memory' lines;
- the driver MiB at the same spot in session 2, which has a warm driver cache and few unoptimized builds.
It matters on the first visit to each area.
  - Risco: Use-after-free (device lost) if any recorded but unsubmitted command buffer still binds the old pipeline. The deferral must cover the recording worker's queue as well as GPU completion.
  - A/B: Cross-boot with a cold driver cache on both sides: a new empty folder via -Set KYTY_DRIVER_CACHE_KEY=<new 64-hex value> for each run. Same route; -Set KYTY_FREE_REPLACED_PIPELINES=1 against 0. Compare the driver MiB in each 'Video memory (flip N)' line and replaced_unoptimized. No device-lost or validation errors are allowed.
### Ações do usuário sem código
- Before any A/B, play the same route once more with no switches as the baseline. The driver cache is now warm, and the first session's compile stalls distort every number.
- Try TextureReduce now: powershell -ExecutionPolicy Bypass -File .\run-windows.ps1 -Game F:\PPSA01342-app -TextureReduce 1. Check that the out.log has 'Textures: large block-compressed textures held without their 1 largest level(s)'; the code default is 0, and the help text saying 'default 1 below 12 GB' is stale. Compare with the baseline over the same route: over-budget 'Video memory (flip N)' samples, 'Vulkan: video memory full' lines, and non-compile SLOW Frame seconds (idle- or upload-dominated). Close surfaces get a little softer; -TextureReduce 2 is stronger and blurrier.
- Give the game more of the card. Close browsers with hardware acceleration, Discord, overlays and anything else using the GPU before launching. The budget in the line 'Video memory (start): ... of N MiB' was 7256 at start and fell to 6976 during play.
- Do not use -FsrFrameGen, or -Set KYTY_VRAM_RESERVE_MB=..., on this card. The reserve does not move the texture collector's live levels (textureCache.cpp:4530-4538) but does make buffer collection start earlier.
- Leave KYTY_SMALL_VIDEO_MEMORY unset (detected as 1). The fork measured 22 fps against about 45 fps on an 8 GB card with textures yielding (vma.cpp:470-476).
- Measure at a fixed spot without a rebuild:
1. Add -Set KYTY_LIVE_FILE=F:\dev\kyty-live.txt (the path must have no spaces).
2. In game, save this file with these lines:
   id 1
   measure 10 spot
   vma F:\dev\vma-1.json
   images F:\dev\images-1.tsv
   buffers F:\dev\buffers-1.tsv
   objects
3. Raise the id for each new request, and take one sample under budget and one near or over budget.
What you get:
- the out.log gets LIVE_MEASURE (fps, low1, p99) and LIVE_COUNTERS per frame (gc_image_deletes, gc_buffer_deletes, full_upload_bytes, buffer_registrations), a 'Video memory (live)' DXGI line, and LIVE_PIPELINE_OBJECTS (replaced_unoptimized);
- vma-1.json holds VMA's heap usage and budget, to compare with DXGI;
- images-1.tsv lists every image's bytes, usage kinds (t/s/r/d) and memory type (type 0 = plain), showing how much is textures and how much is render targets.
- Optional, for one diagnostic run only (not for benchmarks): copy run-windows.json to run-diag.json, change the value after "--printf-direction" from "Silent" to "Console", and launch with -Config .\run-diag.json. Every 10 s you get the collector line ('Video memory: device heap X of Y MiB; collections ...; swapped out ...; image GC trigger/pressure/critical ...'). The volume of the other log lines it enables is unknown.
- Optional, to confirm paging: run PresentMon (GPU Busy and frame time) next to the game. If GPU busy per frame jumps when the 'Video memory (flip N)' lines go over budget, while the emulator's SLOW frames show render CPU around 20 ms and idle above 100 ms, that is paging.
- Optional, to cut both GPU load and render-target VRAM: -RenderHeight 1080 (KYTY_RENDER_HEIGHT). It works on 01.005.000 because the patch pattern is found in both builds (demonsSoulsRenderSize.h:22-27). The 3D image gets softer; the VRAM saving is unknown, so measure it with images-1.tsv.

### Verificação (fix-1080p-other-builds, lighting-rule-105, file-follow-other-build, hints-105-coverage, hud-1080p-port-105)
- **fix-1080p-other-builds** => implement. Evidência: The boot abort is real in code. LaunchPlan.cpp:219-232 and run-windows.ps1:353-361 pass --game-patch for any ^PPSA0134[0-2]$. Both patches/1080p-*.json say "id":"PPSA01341","version":"01.007.000". ValidateGame then fails on the title (gamePatch.cpp:111-116), Apply returns false (gamePatch.cpp:203-205) and the emulator stops with EXIT("Failed to apply game cheat\n") (runtimeLinker.cpp:1641-1645). The launcher's warning still offers "Play anyway" (MainWindow.cpp:417-424). The user's session never chose these outputs: the log has no 'Game cheat' line. Correções: Build (b) and (d). Rework or drop (a) and (c) as follows.

(b) Gate on the exact build the patch file names, not on contentVersion alone:
- Condition: titleId == PPSA01341 && contentVersion == 01.007.000. Better, read "id" and "version" from the JSON.
- Every other Demon's Souls build, 1080f: set KYTY_RENDER_HEIGHT=1080 only when the user gave no render height (launcher render_height <= 0 or == 1440, LaunchPlan.cpp:201; run-windows $RenderHeight -le 0). Set no KYTY_RENDER_CONVARS and no --game-patch.
- 1080q: nothing (the 4K output).
- Print one line saying why, and fix the text of MainWindow.cpp:417-424 and 601-605.
- Why this gate: as proposed, PPSA01340/PPSA01342 01.007.000 still take the patch path.

(c) As written it gives TooShort, not NotFound. With CC excluded, checked = 3 < kMinSearchedBytes, so lines 76-78 return TooShort.
- Combined with (a) setting same_version=false for another region, the HUD mod is also skipped on an identical other-region 1.07 eboot.
- r_Wants4K=WantsBase is still injected, so the HUD is laid out off screen (demonsSoulsRenderSize.h:13-14).
- Exact rule: keep line 72's AtOffset test with every non-zero byte. Only for the search branch (lines 76-81), count bytes of writes whose 'off' is not one repeated 0x00/0xCC/0x90 value, require >= 8, and pick the anchor among those writes.
- Test in tests/GamePatchLocateTests.cpp: a CC-anchored mod at its offset is AtOffset; moved, it is TooShort.

(a) After (b) it only matters for a manual -Patch. If kept, set same_version = (version == plan->version) for two supported regions (1.05 regions are identical at the fixed RVAs of demonsSoulsGpuPages.h). Otherwise the 4-byte 1440p mod is always TooShort on another region's 1.07.

(d) Write the test as IsSupportedTitle("PPSA01342") && !IsSupportedTitle("PPSA01343").

Other facts:
- No fps gain over a hand-set -RenderHeight 1080, as the proposer says.
- No SPIR-V change.
- Check: the game boots with -Output 1080f and the log prints "Demon's Souls render size: performance mode renders 1920x1080 instead of 2560x1440 (output 3840x2160)".
- **lighting-rule-105** => needs_measurement_first. Evidência: The TitleLaneLocal list is keyed by 1.07 hashes only (Translate.cpp:959-981). The log has no 'LaneLocal' line (KYTY_LANE_LOCAL_LOG unset), so whether 1.05 has the same idiom, and its GPU share on the 3060 Ti, are unmeasured. The gain is derived from an RTX 5090 on a CPU-bound PC (general.md: 10.05 -> 9.07 ms), and it is unknown whether this PC is GPU-bound. StoreSwitches includes KYTY_LANE_LOCAL_TITLE (pipelineCache.cpp:935-944), so -Set KYTY_LANE_LOCAL_TITLE=1 forces a new translation store and full retranslation, which makes the log line print without a rebuild. Correções: Measure first, with no code change:
1. GPU-bound test: default vs -RenderHeight 1080, 2 boots per side, KYTY_FPS_HUD=2.
2. One boot with -Set KYTY_LANE_LOCAL_LOG=1,KYTY_LANE_LOCAL_TITLE=1 (about 2 min of retranslation). Keep the "LaneLocal CS <hash>: <reason>" refusals and inspect the refused wave64 CS for S_ORN2_SAVEEXEC plus V_READLANE 31/63. This replaces the hint/seed layout matching, which is heavier.

If the code is built:
- Also add KYTY_LANE_LOCAL_HASHES to the launcher's kTranslationSwitches (src/fork-launcher/src/core/Precompile.cpp:35-38), or a precompile and the game disagree on SPIR-V.
- Adding it to StoreSwitches rekeys every user's translation store once.
- Verification spots: only places the user has reached (tutorial, 1-1). Level travel and teleport are 1.07-only (demonsSoulsWarp.cpp), so 'Latria and Shrine' is not available. The A/B is cross-boot only.
- No static precompile re-run is needed; only the matched programs' pipelines miss the static cache.
- **file-follow-other-build** => needs_measurement_first. Evidência: There is supporting data the proposer did not cite:
- 23 'SLOW AprRead' lines. In 21 of them the open dominates (open=100.2..201.0 ms, read <= 2.8 ms, prio=-2), e.g. "SLOW AprRead 200.1 ms open=199.3 read=0.7 ... file=spot_hallway_01_rays_03_eac3b9f7_baked2dshadow.ctxr".
- 18 of the 23 basenames are in files-PPSA01341_01.007.000.files. Missing: obj_bg_tut.bnk and 4 others.
- The seed lookup is exact-version only (libAmpr.cpp:1589-1599, SeedFileRecording at 1500-1505).
- Still, open order across versions and the gain on this NVMe PC are unmeasured. Correções: Test first with the user action that needs no code (append the 1.07 list to _PipelineCache\files\PPSA01342_01.005.000.files with the emulator closed). Then compare a first visit of a world (e.g. Nexus -> 1-1) against -Set KYTY_FILE_FOLLOW=0. Reboot before each boot and restore the recording between boots. Metrics: seconds from load start to control, and the count and sum of 'SLOW AprRead ... open=' values. Build the code only if the append clearly shortens loading.

If built:
- Skip seeding when KYTY_FILE_FOLLOW=0.
- Only one other list ships, so 'same version first' is moot.

Also: the 'Ampr ... 9916.9 ms in all' figure is summed over every thread that ran commands. With KYTY_AMPR_SYNC on by default they run on the submitting thread (libAmpr.cpp:1440-1450, 1281-1297), so it does not mean saturated workers.
- **hints-105-coverage** => already_done. Evidência: precompile-windows.ps1:171-215 already merges three sources into recorded-play-<id>.hints, applied to hinted-<id>.seeds as the first tier: the shipped hints (falling back to another region's same-version file), every _PipelineCache\warmup-v2\*\<id>.shaders of this PC, and recorded-<id>.seeds. The launcher does the same (Precompile.cpp:97-112). At run time this PC's recording is used from the next launch (pipelineCache.cpp:2982-2990, KYTY_SHADER_WARMUP=1 in run-windows.json:4). So a default re-run already folds this PC's play in. Correções: Wrong claims:
- '-Quick may drop the non-recorded pipelines' is wrong. -Quick merges without --prune, and 'without --prune the store keeps its pipelines too' (precompile-windows.ps1:392-395; shader-precompile.cpp:23-24).
- stalls.py counts only TranslateProgram, CreateGraphicsPipeline and CreateComputePipeline (its PATTERN). Count 'SLOW Async*' with grep.

Ceiling, from the log:
- Frame-blocking compiles are the 143 sync Create* lines: 46.8 s summed, against 43.9 s of compile= in SLOW Frame lines.
- The 671 Async* lines (292.8 s of worker time) were compiled 'while its draws took another path' (pipelineCache.cpp:4137-4142).
- 298 of the 394 AsyncGraphics lines are 'native', which general.md says is built at run time.

Shipping a merged hints file made from tutorial-only play adds little. Do it after a full playthrough. First check on the user's PC whether the programs named by the sync Create* lines are already in released-PPSA01342_01.005.000.seeds: if they are, the misses are specializations, not tier size.
- **hud-1080p-port-105** => needs_measurement_first. Evidência: Even the proposer estimates the saving at under ~1 ms with no measurement. The method's 'ceiling first' rule applies. The ceiling does not need a 1.07 dump: -Set KYTY_RENDER_CONVARS=r_Wants4K=WantsBase on 1.05 runs the game's 1080p output tier (gameArgs.cpp:32-44 appends it to renderconfig_*.txt). Its GPU cost matches the patched mode; only the HUD scale getter differs and the HUD lands off screen (demonsSoulsRenderSize.h:13-14). Correções: Measure on the user's 1.05 now: -Set KYTY_RENDER_CONVARS=r_Wants4K=WantsBase (diagnostic only, HUD misplaced; first confirm a 'Game args: ...renderconfig' line) against -RenderHeight 1080. Use 2 boots per side, a fixed spot, and GPU ms and fps from KYTY_FPS_HUD=2. Port only if the gain is > ~3%.

Fix the stub constant's address: 0xDDA224 + 8 + 0x171F930 = 0x24F9B5C, so 0x9024F9B5C, not ...B5D.

The proposed b8 imm32 / c5 f9 6e c0 / c3 stub is valid (vmovd xmm0,eax zeroes the upper lanes, as vmovss from memory does).
- Erros no diagnóstico: 1. "The US eboot holds PPSA01341 1.05's code byte for byte" goes too far. What is proven: seven fingerprinted functions match at fixed RVAs (demonsSoulsGpuPages.h:43-51, about 3.6 KB), the signature sites match, and all 6,997 hint programs matched. That makes the same build likely; it does not prove the whole eboot is identical.
2. "Compile stalls 61% of 77.5 s": the compile= fields of the 219 SLOW Frame lines sum to 43.9 s, which is 57%.
   - That time comes from the 143 sync CreateCompute/CreateGraphics lines (46.8 s), not from the 814 lines as a whole.
   - The 671 Async* lines (292.8 s of background compile) did not block frames.
   - 298 of the 394 AsyncGraphics lines are 'native' variants, which a recorded tier does not cover.
   - So 'the recorded tier is the version-keyed part of the 61%' overstates its ceiling.
3. The latent-risk text in B.1: Place() is tried at every overlapping position inside each run of 9+ CC bytes (current = found + 1, gamePatchLocate.h:95), not once per run. The search stops at 2 hits (Ambiguous), so a false 'Moved' needs exactly one chance match of three 1-byte checks.
4. Proposal (c)'s outcome would be TooShort, not NotFound (gamePatchLocate.h:76-78).
5. 'Ampr workers ran ~9.9 s busy per 10 s' is a sum over every thread that executed commands. Commands run synchronously on the submitting thread by default (KYTY_AMPR_SYNC, libAmpr.cpp:1440-1450).
6. The proposer missed that the log already shows file-open cost on this PC: 21 of 23 SLOW AprRead lines are 100-201 ms opens.
7. User action 7's reason to avoid -Quick is wrong: -Quick does not prune the store. Default mode is also fine, since both skip pipelines already in the static cache.
8. The VRAM peak in the log is "heap 0: usage 10568 MiB of budget 7042 MiB" (log line 8547), just before "SLOW Frame 3679.6 ms". It is higher than the 8,250 MiB sample quoted. This is not version-related, but it shows memory pressure at the boss.
- Faltou: Log evidence for the file-follow lever on this PC: 21 of 23 'SLOW AprRead' lines are open-dominated (for example "SLOW AprRead 201.6 ms open=201.0 read=0.6 ... file=dcl_leak_water_damage_01_col_83a91ba0.chunk2.ctxc"), and 18 of the 23 basenames are in tools/local/static-precompile/files-PPSA01341_01.007.000.files. So user action 6 (append the 1.07 list) is the cheapest test of P3. | The 1080p output-side ceiling can be measured on 1.05 now, with no code and no 1.07 dump: -Set KYTY_RENDER_CONVARS=r_Wants4K=WantsBase (HUD misplaced, diagnostic only) against -RenderHeight 1080. | In proposal 1 (b), a render height the user set explicitly must win over the 1080f mapping (LaunchPlan.cpp:201, run-windows.ps1:322). The new gate must cover PPSA01340 and PPSA01342 01.007.000; as written, they still get the patch. | A new lane-local switch must also go into the launcher's kTranslationSwitches (src/fork-launcher/src/core/Precompile.cpp:35-38), not only StoreSwitches. | tools/local/static-precompile/stalls.py ignores 'SLOW AsyncComputePipeline' and 'SLOW AsyncGraphicsPipeline' (671 of the 814 lines). Any before/after compile A/B must grep them separately and report the frame-blocking Create* lines (143 lines, 46.8 s) separately from background compiles. | Before shipping more hints, check on the user's PC whether the programs of the 143 sync Create* lines (for example CS 0x43aa1fdf1cdc60d8, 4385 ms at boot) are already in released-PPSA01342_01.005.000.seeds. If they are, the misses are specialization or key mismatches, not a thin recorded tier.

### Verificação (covered-by-local-only, native-variants-recorded-tier, async-compute-table-fast-build, foreground-first-driver-compiles, table-forms-of-guesses)
- **covered-by-local-only** => needs_measurement_first. Evidência: precompile-windows.ps1:320 (--covered-by $Recorded); staticPrecompile.inc:678-691 (covered keys planned Skipped); plan-seeds.log 'plan saved ... (80568 pipelines, 64494 of them this seed file's)'; staticPrecompile.inc:318-323 (cs-first = PortableFormats of the spec); README.md:133-134 (numeric-class guesses miss ~10% of CS programs); precompile-windows.ps1:395-397 + staticPrecompile.inc:1084-1093 (--merge --prune keeps only this run's shards) Correções: The benefit (33 s) assumes the guesses cover 79 of the 80 hinted misses. Nothing measured shows that (see diagnosis error 3).

Cheap decisive measurement first, no 1-hour precompile needed:
(a) Play one session (it can be the second session) with -Set KYTY_PIPELINE_SOURCE_LOG=1. Collect the 'PIPESRC cs-normal|gfx-normal key=K static=0' lines. They are printed for driver-cache hits too (shaders.cpp:1296-1302, 1211-1219).
(b) Offline, run: kyty_shader_precompile --game F:\PPSA01342-app --seeds _Build\static-precompile\seeds-PPSA01342_01.005.000.seeds --dry-run --timings seeds.timings.txt. Before it, set $env:KYTY_EXEC_DEMAND=1 and the other translator switches that precompile-windows.ps1:76-82 copies from run-windows.json, or every identity will differ.
(c) The timings file's 6th column uses the same identity as PIPESRC key= (staticPrecompile.inc:740/790 vs shaders.cpp:1184/1284). The share of static=0 keys found there with kind cs-first/gfx-first is exactly what -Full would serve. Go to -Full or this script change only if that share is large.

Other corrections:
- Cost: roughly 30-50 min on this PC plus re-translation in all 582 shards (see diagnosis error 6), not 12-15 min. Store about +2.9 GB on the shared 480 GB SSD.
- Generality: on 1.07 hint-only installs the skip was measured nearly lossless (precompile-windows.ps1:15-18; general.md 'met 1 of 2,227'). Making local-only the rule costs those installs a fifth of a run for no measured gain. The new argument (the 1.05 hints are thin) is specific to this version.
- src/fork-launcher/src/core/Precompile.cpp:193 and :347 also pass --covered-by, and :358 prunes. The change must cover the launcher too.
- Any later run without -Full re-plans with --covered-by and merges with --prune, which deletes the guesses a -Full run added. That includes plain precompile-windows.ps1, the precompile that run-windows.ps1 -Prompt offers (run-windows.ps1:566), and the launcher. Until the script changes, every run must keep -Full (or -NoPrune).
- No SPIR-V change: correct.
- **native-variants-recorded-tier** => needs_measurement_first. Evidência: plan-seeds.log '... 0 skipped: recorded play's programs, planned; no native variants'; staticPrecompile.inc:816 (native_first needs --native-variants); pipelineCache.cpp:4104-4158 and table-xpr.inc:2573-2580 (the render thread never waits on native/table variants); session.log: 261 of the 283 unique 'SLOW AsyncGraphicsPipeline ... native' pairs have no 'SLOW CreateGraphicsPipeline' for the same VS/PS Correções: The assumption that the store would hit is reasonably supported: 261 of the 283 native-slow pairs had no slow normal compile, so their normal pipelines (same modules and state) probably came from the recorded tier, and gfx-first-native / gfx-exact-native would match.

But it removes no frame stall:
- The win is fewer normal-path draws for 0.1-0.9 s per pair, plus about 47 s less background driver work. Neither has been quantified on this PC.
- Its value depends on the contention question in the gate proposal.

Other corrections:
- src/fork-launcher/src/core/Precompile.cpp needs the same flag on plan-recorded and on the recorded-tier shards.
- Offline pre-check: compare the 'PIPESRC gfx-native static=0' keys from a KYTY_PIPELINE_SOURCE_LOG=1 session with 'kyty_shader_precompile --seeds _Build\static-precompile\released-PPSA01342_01.005.000.seeds --dry-run --native-variants --timings', looking at kinds gfx-first-native / gfx-exact-native, with the launch config's translator switches set.
- The A/B metric must be frame-level: SLOW Frame compile= sum and render ms in the first seconds after entering an area. Counting 'SLOW AsyncGraphicsPipeline native' lines is not enough.
- No SPIR-V change; only an incremental precompile rerun.
- **async-compute-table-fast-build** => needs_measurement_first. Evidência: session.log:216 'SLOW CreateComputePipeline 4385.0 ms CS=41 CS=0x43aa1fdf1cdc60d8' (unoptimized, normal module) vs session.log:504 'SLOW AsyncComputePipeline 5092.8 ms table CS=0x43aa1fdf1cdc60d8' (optimized, table module); pipelineCache.cpp:2802-2806 (OptimizeWorkers = 16/4 = 4 threads), 2826-2843 (BuildOptimized calls MainPipelineDone at 2840); docs/EXPERIMENTS.md:516-517 (89 s to 0.6 s was measured offline on another PC) Correções: The only shader seen on both paths in the log was unoptimized almost as slow as optimized: 4385 vs 5093 ms, 14% less (different modules, one sample). So the fork's 150x ratio is not what happens in this process. If that holds, the change:
- adds a near-full-cost compile ahead of each table compile;
- moves 209.7 s of optimized table work from the 8 TableWorkers onto the 4 OptimizeWorkers, which already serve every sync-path optimized build, so those get delayed.

What it trades: today, while a table compile runs, the dispatch takes the normal path, and for non-hinted programs that path usually has an optimized store pipeline. The change trades that render-thread CPU for GPU time spent on unoptimized 1,049-1,078-slot CS, at 85-90% GPU load.

The proposer's step 3 is needed: a table-cache variant of BuildOptimized's done-accounting.

Build it only if the contention test (see the gate proposal) shows that, with no background compiles running, unoptimized compiles on 617.42 are far cheaper than optimized ones. Then A/B with -Set KYTY_ASYNC_FAST_BUILD=0/1, including GPU ms over 10 s after entering an area. No SPIR-V change.
- **foreground-first-driver-compiles** => needs_measurement_first. Evidência: session.log:7304-7306 (three compiles of 15402.0/15429.9/15430.1 ms ending within ~1 ms); the 12.9 s frame: CS=4168 ends at tsc 159236549292198 and CS=4169 at 159237696233220 = 337 ms for a 307 ms compile (3.4 GHz tsc); pipelineCache.cpp:2826-2843 (an optimized build is queued for every unoptimized compile); m_pipelines_building incremented at 2832, 4154, 4317 Correções: The cause is unproven, and lines 7304-7306 point toward parallel execution rather than serialization (diagnosis error 2).

The proposed contention test is confounded:
- Run B (KYTY_TABLE_DISPATCH=0, KYTY_TABLE_XPR=0, KYTY_NATIVE_XPR=0) still runs every BuildOptimized job on OptimizeWorkers, which the gate also targets. It also changes the whole frame path.
- Run C (KYTY_PIPELINE_FAST_BUILD=0) removes the optimize workers but makes the sync compiles optimized, so A vs C cannot separate 'inherent' from 'contended'.

Cleaner, as an uncommitted diagnostic:
- add in_flight=%u (m_pipelines_building.load(), which counts queued and running background builds) to the SLOW CreateComputePipeline/CreateGraphicsPipeline printf (pipelineCache.cpp:4172-4177, 4245-4250);
- regress compile ms on in_flight over one cold run;
- or add a temporary switch that makes BuildOptimized return early, and compare 'PIPESRC cs-normal unoptimized=1 ms' under B's switches plus that switch (no background compiles at all) against defaults.

Design gaps if it is built:
- Only optimize jobs get the quiet period. Table and native jobs can start in the ~30 ms render-thread gaps between a burst's compiles (337 vs 307 ms above).
- An in-flight 1,078-slot table compile (up to 16.8 s here) cannot be preempted. Under serialization the worst wait is that, not one ordinary compile.
- The ceiling (-43 s) relies on the fork's offline 26 ms maximum.
- **table-forms-of-guesses** => wrong. Evidência: staticPrecompile.inc:395-414 (table form: byte_base_offset cleared, only 'portable' buffers set to kInvalid, no PortableFormats) vs 318-323 (cs-first goes through CanonicalizeSpecialization + PortableFormats); ResourceMaterialization.cpp:954-969 (PortableFormats erases buffer descriptor formats); staticPrecompile.inc:14-19 (SeedSpecializations guesses k32UInt for every formatted buffer); comment at staticPrecompile.inc:391-393; README.md:133 (only ~23% of integer-texture numeric classes predicted) Correções: The premise does not carry over:
- The 99.6% figure (itself unproven) is about portable normal modules. There PortableFormats erases the guessed buffer formats, so a wrong guess still matches.
- Table forms keep the specialization's formats exactly (table_portable 0). A guessed table form therefore matches only when the guess equals the runtime's exact formats: k32UInt for every formatted buffer, plus predicted image numeric classes. That is exactly why staticPrecompile.inc:391-393 leaves them out.
- 78 of the 84 hinted table misses are programs whose normal pipeline also missed, i.e. programs whose runtime specialization the hints lacked.

The expected hit rate is unknown and probably low. The cost is tens of minutes of heavy table CS compiles (15-17 s each optimized here). Re-propose only with a measured hit rate: PIPESRC cs-table static=0 keys vs a dry-run listing of such forms.
- Erros no diagnóstico: Mostly right on the mechanics. I checked these citations and they are correct: the WaitCompile scopes (pipelineCache.cpp:2291, 4178, 4251), FirstBuild only at 4184 and 4255, the async paths never waiting (table-xpr.inc:2573-2580), the worker counts 8/4/4 on 16 threads, the plan sizes and every count in the coverage table. I re-ran cover.py and cs.py and got the same numbers. Errors and overstatements:
(1) The 12,952.5 ms frame holds 51 CreateCompute calls, but they are CS ids 4163-4269 (session.log:5359-5518), 100.7-566.0 ms each, summing to 12,466 ms. They are not ids 4217-4269.
(2) Log lines 7304-7306 do not support hypothesis (a). Three compiles of the same size ran for 15.40-15.43 s and ended within about 1 ms of each other. That is what parallel execution looks like. A strict per-process driver lock would have spaced the endings about 5 s apart. Only fine-grained time-slicing would explain both. The better argument for (a) is tools/local/static-precompile/README.md:34 ("22 threads kept only 1.3 cores busy"), but that was measured on Full compiles on another PC.
(3) "1 of 278 guessed-tier programs had a slow normal compile" does not show that the store served them. SLOW lines start at 100 ms (run-windows.ps1:411), and this session has no PIPESRC lines. README:133-134 says guessed integer-texture numeric classes miss about 10% of CS programs. So "the guessed fallback serves 99.6%" is an inference, not a measurement. The supporting signal is real but indirect: 193 non-hinted programs had slow table compiles while only 1 had a slow normal compile.
(4) "4-spec programs missed 40% (8 of 20)": the 20 is every 4-spec program in the hints file (cs.py compares against all hinted programs). How many of them this route actually met is unknown.
(5) "This session was recorded (pipelineCache.cpp:4207)" is only true in memory. The session ended without any "Shader warmup: saved N inputs and M pipelines" line (pipelineCache.cpp:3744) and without any ReportSession line. The recording is written only by Save(), called at window.cpp:1031-1034. The periodic saver (pipelineCache.cpp:3585-3604) writes the driver caches only. So whether warmup-v2 was written at all is unknown.
(6) The -Full cost is underestimated. This PC stored about 29.7K seed-tier pipelines (45,735 minus 16,074 recorded-tier plan keys) in the roughly 45 min between "ready to play" (00:12:27) and "done in 00:57:35". Adding 34.8K more is about the same amount of work, not 10-20 min. Also, 34.8K is an upper bound, not a count: plan identities are not stored driver keys, and some keys may have been rejected.
(7) The diagnosis missed corroborating evidence. 78 of the 80 slow-normal CS programs also had a slow async table compile, and 261 of the 283 native-slow graphics pairs had no slow normal compile.
- Faltou: Session 1 probably never ran its exit save. There is no 'Shader warmup: saved ...' line and no ReportSession line in the log, even though the report says exit 0. Save() runs only at window.cpp:1031-1034, and the periodic saver (pipelineCache.cpp:3585-3604) saves only PPSA01342.bin and .table.bin. So _PipelineCache\warmup-v2\<16 hex>\PPSA01342_01.005.000.shaders may not exist. Ask the user to check that file, and to close with the window's close button and look for the 'Shader warmup: saved N inputs and M pipelines' line. | User action 1: 'Shader warmup: loaded N ... with N, M > 0' is not guaranteed, for the reason above. Also, replaying the tutorial-to-boss route needs a new character, because the save is past the tutorial; otherwise session 2 measures other areas. The driver-cache part of the prediction is sound: the main cache was saved at log line 8510 (36116165 bytes) and the table cache at 8503. | User action 4 is harmful as written, in three ways. (1) 'Afterwards, a plain precompile-windows.ps1 -Game ...' re-plans with --covered-by and merges with --prune (precompile-windows.ps1:395-397), which deletes every guess -Full added. The run-windows.ps1 -Prompt offer (run-windows.ps1:566) and the launcher (Precompile.cpp:193/347/358) do the same. Keep -Full, or -NoPrune, on every later run. (2) The 10-20 min estimate is unfounded: about 30-50 min by this PC's own rate. (3) Better to do the offline PIPESRC-vs-dry-run key check first. | User action 6 needs three fixes. (1) KYTY_SLOW_LOG_MS=30 does not lower the pipeline SLOW thresholds: they use KYTY_HITCH_LOG_MS, which run-windows.ps1:411 sets to 100. It only adds per-call slow lines elsewhere, so drop it; PIPESRC already gives ms for every pipeline. (2) The 'PIPEKEY miss key=' value is an XXH3 of the driver's key (shaders.cpp:447-458) and cannot be compared with the --timings key column, which is ComputePipelineIdentity/GraphicsPipelineIdentity. Compare 'PIPESRC key=' instead (same functions: shaders.cpp:1184/1284 vs staticPrecompile.inc:740/790). (3) The manual dry run needs KYTY_EXEC_DEMAND=1 and the other translator switches that precompile-windows.ps1:76-82 sets, and it should cover seeds-PPSA01342_01.005.000.seeds too, not only released-*.seeds, to answer whether the guesses cover the misses. | User action 7: the contention test leaves the optimize-build workers running in every run, and Run C changes the build type of the sync compiles. Use the in_flight diagnostic, or a no-BuildOptimized diagnostic switch, described under foreground-first-driver-compiles. | User actions 2, 3, 5 and 8 check out. Every switch name exists: KYTY_PIPELINE_FAST_BUILD (pipelineCache.cpp:2816); KYTY_SHADER_WARMUP (2982); KYTY_DRIVER_CACHE_KEY must be 64 lowercase hex (driverCachePolicy.h:9-13); KYTY_TABLE_DISPATCH / KYTY_TABLE_XPR / KYTY_NATIVE_XPR (performance-switches.h:124,138,140); KYTY_PIPELINE_SOURCE_LOG / KYTY_PIPELINE_KEY_LOG (shaders.cpp:604,417). -Set is applied after the fixed key (run-windows.ps1:389 then 415-419). Both signatures include driverVersion (pipelineCache.cpp:110-112, 3383-3384). -ExportHints throws if no warmup-v2 recording was saved. | KYTY_PIPELINE_SOURCE_LOG prints two stdout lines per pipeline (shaders.cpp:1296-1307), and the methodology measured about 6 ms per blocked stdout write. Keep it out of runs whose frame times are compared, or turn it on for both sides. | The release launcher src/fork-launcher/src/core/Precompile.cpp copies the script's precompile logic. Any precompile policy change (covered-by, native variants) must also change Precompile.cpp:193, :347 and :358.