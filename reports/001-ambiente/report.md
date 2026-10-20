# 001 — ambiente

**Status: PARCIAL.** Cobre instalação, build, testes, versão do jogo (PPSA01342 01.005.000), build de
`fix/bench-save-region`, RAM e pré-compilação (seção "Complemento"). Ainda faltam a entrada no jogo e o save de
benchmark, num commit seguinte, na mesma pasta.

- Data/hora: 2026-10-19, 20:01–20:41 (horário local do PC)
- Branch e commit testados: `fix/windows-test-build` `f225a627efe635f05ba6bc6d1e5dc7cd12ef915f`
  (= `demons-souls` `dc296347` do exploitz, a base da beta.2, + correções de build dos testes, ver abaixo)
- Build: dev (`-march=native`, PGO do repositório `tools/pgo/kyty.profdata`), ThinLTO, mimalloc, Release
- Switches extras (-Set ...): nenhum
- Configuração do jogo: — (ainda sem dump)
- `kyty_emulator.exe` (de `f225a62`): SHA-256 `B7670CF1DACCDC90B57E9F73C1C1E6AA73EB0CC52510B44CF95CC45FF08D890B`

## Sistema

| Item | Valor |
|---|---|
| CPU | AMD Ryzen 7 5700X, 8 núcleos / 16 threads (Zen 3), sem P/E-cores |
| GPU | NVIDIA GeForce RTX 3060 Ti, 8 GB (heap device-local 8024 MiB; budget Vulkan 6349 MiB) |
| Driver NVIDIA | 617.42 (atualizado depois do build; build e ctest rodaram com o 610.88) |
| RAM | 32 GB: 4 × 8 GB DDR4 Kingston de kits diferentes, rated 2666, a **2933 MT/s** (estabilidade não testada, ver "Complemento") |
| Placa-mãe / BIOS | Gigabyte B450M DS3H V2, BIOS F67d |
| Windows | Windows 11 Pro 25H2, build 26200.9711 |
| Memória virtual | Gerenciado pelo sistema (`C:\pagefile.sys`, 2 GB alocados agora) |
| Plano de energia | Equilibrado |
| Disco | Repositório e build em F: (NVMe WD WDS480G2G0C, NTFS, mesmo SSD do C:). O dump também vai para F: |
| Monitor | 2560×1440 a **164 Hz** agora; será posto a 60 Hz antes das medições (BENCHMARKING.md) |
| Versão do jogo | **PPSA01342 (EUA) 01.005.000**, em `F:\PPSA01342-app` (ver "Complemento") |

## Ferramentas

| Ferramenta | Versão |
|---|---|
| Visual Studio Build Tools 2022 | 17.14.42, MSVC 14.44.35207, Windows SDK 10.0.26100.0 |
| LLVM (clang-cl, lld-link) | 19.1.7, em `C:\Program Files\LLVM` (o build-windows.cmd usa este antes do clang do VS) |
| CMake / Ninja | 3.31.6-msvc6 / 1.12.1 (do VS) |
| Vulkan SDK | 1.4.363.0 |
| Python / Git | 3.12.10 / 2.54.0 |

Flags efetivas do emulador (build.ninja): `-O3 -march=native -flto=thin -fprofile-instr-use=.../tools/pgo/kyty.profdata`.

## Build

Rodadas em sequência, sem nada pesado em paralelo:

| Alvo | Commit | Tempo | Resultado |
|---|---|---:|---|
| `kyty_emulator` (primeiro build: configure + 3rdparty + emulador) | dc29634 | 4,8 min | ok |
| `kyty_shader_precompile` | dc29634 | 2,0 min | ok |
| `kyty_tests` | dc29634 | 2,2 min | **falhou** (compilação, ver abaixo) |
| `kyty_tests` com as correções (`ninja -k 0`, duas rodadas) | dc29634 + correções | 5,5 + 0,6 min | ok |
| `kyty_emulator` (relink no commit limpo) | f225a62 | 0,8 min | ok |
| `kyty_shader_precompile` | f225a62 | 2,2 min | ok |

## Correções necessárias para compilar os testes

Em `dc29634`, `kyty_tests` não compila nem linka no Windows. O emulador e o pré-compilador compilam sem mudanças.
As correções estão no branch **`fix/windows-test-build`** (commit `f225a62`, 5 arquivos, +36/−3). Só tocam
alvos e arquivos de teste:

1. `kernel_file_system_tests` e `fiber_migration_tests` compilam as fontes do emulador sem
   `KYTY_LOCAL_VULKAN_RECORDING` / `KYTY_LOCAL_NATIVE_RESOURCES` (`LocalVulkanRecording` e `DeviceFault` não
   declarados). Adicionados às duas listas do CMakeLists, como o `adfde2e8` fez para `virtual_memory_allocation_tests`.
2. `vulkan_recording_tests` e `event_queue_lifetime_tests`: faltava `local/local-platform.cpp`
   (`ThreadId`, `SetThreadName`, `PinThreadToCpuList`).
3. `KernelFileSystemTests.cpp` inclui `SDL.h`, que no Windows renomeia `main` para `SDL_main`: `SDL_MAIN_HANDLED`
   só nesse arquivo.
4. Símbolos que o link do Linux não alcança, mas o lld-link exige. Stubs nos arquivos de teste, cada um retornando
   o que a função real retorna sem fita de entrada, sem espaço de endereços do jogo e sem dispositivo de áudio:
   `Controller::RecordingTapePath/ReplayTapePath/NoteMarker` (ImeDialogTests), `Memory::RestoreGuestWritable`
   (PageManagerTests, MemoryTrackerTests), `kyty_local_range_set_fast_mode` / `kyty_local_async_reprotect_mode`
   com 0 (MemoryTrackerTests) e `AudioInternal::AudioOutQueuedMicros` (AudioOut2PortTests).

Os erros originais estão em `build-errors-before-fix.txt`.

**Pedido ao agente da nuvem:** crie os branches `perf/*` a partir de `fix/windows-test-build`, ou faça merge dele,
para que o `kyty_tests` compile aqui.

## Testes (ctest, código de f225a62)

Rodado antes do commit, com o working tree idêntico ao de `f225a62` (ver `build-summary.txt`).

`ctest --test-dir _Build/windows --output-on-failure --timeout 900`: **40 de 64 passam, 24 falham** (85,6 s).
As falhas são asserções dos próprios testes, não falhas de build. Esta lista é a **linha de base**: cada branch
`perf/*` será comparado com ela (falha nova = regressão).

| # | Teste | Resultado | Mensagem |
|---:|---|---|---|
| 7 | shader_cfg | abort (0xc0000409) | buffer load bounds check was emitted outside EXEC guard |
| 10 | memory_tracker | abort | GPU changes or another region invalidated a local CPU epoch |
| 19 | resource_tracking | Failed | invariant indirect images: indirect image mapping retained worst-case padding |
| 21 | kernel_file_system | abort | unsatisfied wait blocks later buffers at its priority without publishing completion |
| 23 | fiber_migration | Failed | Fatal: `(g_guest_address_space == nullptr \|\| g_virtual_ranges == nullptr)` em `memory.cpp:1122` |
| 25 | audio_out2_port | abort | full async queue accepted another buffer |
| 26 | shader_recompiler_compute | abort | SampledColorViews at host: volume-mip-count component mapping did not report a fatal error |
| 28 | cpu_write_window | SegFault | depois de `[gpu] CpuWriteWindow ok` |
| 34 | demons_souls_compute_chain | abort | DemonsSoulsProfile at title isolation: another title enabled the profile |
| 35 | compute_stream_submission | abort | StreamingCompute: the recorded dispatch batch did not submit at its fixed bound |
| 38 | gpu_command_lane | abort | GpuCommandLane: unmap returned before an earlier native guest-memory callback |
| 41 | texture_resolution_cache | abort | TextureResolutionCache: BC3 sampling replaced the GPU atlas on its next raw-block write |
| 47 | compute_meta_clear_classification | abort | HTile fixture: test HTile entries were not registered in their initial state |
| 48 | buffer_cache_dirty_gc | abort | CPU invalidation retired a reusable Buffer owner |
| 49 | texture_cache_image_overlap | abort | RenderExecutorStencilBindingDiscovery: BC3 sampling replaced the GPU atlas on its next raw-block write |
| 50 | texture_cache_htile_clear | abort | UnifiedTextureCacheFlow: formatted Buffer read bypassed authoritative native image mip data |
| 51 | buffer_cache_ranges | abort | idem (UnifiedTextureCacheFlow) |
| 56 | binding_scratch | abort | idem (UnifiedTextureCacheFlow) |
| 59 | shader_upload_ring | abort | idem (UnifiedTextureCacheFlow) |
| 60 | host_backing_read | abort | idem (UnifiedTextureCacheFlow) |
| 61 | buffer_cache_guest_readback | SegFault | sem mensagem |
| 62 | copy_feedback | abort | CompletedCopyFeedback at mapping epoch fallback: mapping change reused an old snapshot |
| 63 | write_window_handoff | SegFault | depois de `[gpu] CpuWriteWindow ok` |
| 64 | async_write_readback | abort | AsyncWriteReadback at pending: the write did not wait for the download |

Vários subtestes do `shader_recompiler_compute_tests` param na mesma asserção (UnifiedTextureCacheFlow). Não sei se
essas falhas também acontecem no Linux ou no PC do autor; não investiguei nem alterei o código do emulador.

## Resultados

Sem medições de desempenho neste relatório.

## Conclusão

O ambiente de build funciona: emulador e pré-compilador compilam com o toolchain testado (clang-cl 19.1.7), e
`kyty_tests` compila depois de `fix/windows-test-build`. A linha de base dos testes no Windows é 40/64.

## Observações para o agente da nuvem

- Placa de 8 GB: o emulador registra "largest device-local heap 8024 MiB, budget 6349 MiB: treated as small (textures
  may leave video memory past half the budget)".
- O exe guarda o nome git no binário (`KYTY_GIT_HASH`, com `-dirty` se houver mudanças locais). Por isso o SHA-256 muda
  a cada commit, mesmo sem mudança de código, e o cache de pipelines do driver (`_PipelineCache\local\<sha>`) começa
  vazio a cada exe novo.
- A política de execução do Windows aqui é a padrão (scripts bloqueados): `powershell -File x.ps1` falha. Os scripts
  `.ps1` do projeto rodam pelo meu shell (Bypass só no processo); um comando novo deve usar
  `powershell -ExecutionPolicy Bypass -File ...`.
- Afinidade: o CURRENT-PLAYABLE.md diz que, no Windows, a thread de render no CPU 0 corta o fps pela metade. No Ryzen,
  os CPUs 0 e 1 são o mesmo núcleo físico. O teste 005 vai usar `KYTY_RENDER_CPUS=2-15` e `2,4,...,14`.

## Problemas vistos

- Nenhum problema do projeto além dos testes acima.
- Erros meus, sem efeito nos resultados: a primeira chamada do build falhou na hora porque o `cmd` não achou
  `build-windows.cmd` pelo caminho relativo (corrigido com caminho absoluto). O log completo do primeiro build foi
  sobrescrito por engano; os tempos e códigos de saída daquela rodada foram anotados antes.

## Complemento (2026-10-19, 21:40–, horário local)

### Jogo

- Dump: `F:\PPSA01342-app` (tem `eboot.bin` e `sce_sys`). **PPSA01342 = versão americana**, não a PPSA01341 dos testes
  do fork.
- `sce_sys\param.json`: `"contentVersion": "01.005.000"`. `gameversion.txt`: `+BuildVersion=2024-10-13.856931`.
- **Não é a 1.07 (01.007.000).** Os modos 1080p e a regra de iluminação do fork foram feitos para a 1.07.
  `src/graphics/host_gpu/renderer/demonsSouls.h:13` cita "PPSA01342 01.005.000" entre as versões vistas.
- `tools/local/static-precompile/` só tem hints de PPSA01341 (`hints-PPSA01341_01.005.000.hints`,
  `hints-PPSA01341_01.007.000.hints`). Nenhum de PPSA01342.

### Build de `fix/bench-save-region`

- Commit `987dd7c460af7c89f800af9450c723ac1a027511`, working tree limpo. O `bench-windows.py` desse commit tem
  `TITLES = ('PPSA01340', 'PPSA01341', 'PPSA01342')`.
- `build-windows.cmd`: exit 0, 56,8 s (relink: o commit só muda um `.py`). `build-windows.cmd kyty_shader_precompile`:
  exit 0, 60,0 s. 3 warnings `-Wreturn-type` (`bufferCache.h:86`, `pipelineCache.cpp:97`), 0 erros.
- `kyty_emulator.exe`: SHA-256 `BF834938C2F6724F953EAB581B677C5146346C6C9DAA29DD709AA04B664D1253`
- `kyty_shader_precompile.exe`: SHA-256 `78A0E07737E98AE646315FB27493ADFADAB02E063242235855C485E12BBF146C`
- De novo a primeira chamada falhou por caminho relativo do `build-windows.cmd` (erro meu); refeita com caminho absoluto.

### RAM

- **Velocidade usada em todas as medições: 2933 MT/s (rated 2666). Estabilidade não testada.**
- TestMem5 0.13.1 (CoolCmd, `TestMem5.7z` SHA-256 `05ffb5bbaa0c2243bf25664049c7aa3615a77bd5036497ef845fc8ed50128c12`),
  config `1usmus v3 @ 1usmus`. Duas rodadas, ambas paradas pelo usuário antes do fim do primeiro ciclo, 0 erros:
  51% da RAM por 2min15s e 56% (1152 MB × 16) por 3min40s. Dentro do Windows ele não passou de 56%
  ("WARNING! AWE is not available": sem o privilégio "Bloquear páginas na memória"). O usuário decidiu pular o teste.
- Se aparecerem crashes ou glitches aleatórios, a RAM a 2933 é suspeita até prova em contrário.

### Pré-compilação

- `powershell -ExecutionPolicy Bypass -File .\precompile-windows.ps1 -Game F:\PPSA01342-app`, modo padrão (nem
  `-Quick` nem `-Full`), exe `78A0E077…146C`, sem nada pesado em paralelo.
- Início 22:00:20, fim 22:59:14: **58 min 55 s** no total (o script mede 00:57:35 depois de gerar as seeds). Exit 0.
- Mensagem final: `done in 00:57:35: F:\dev\KytyPS5\_PipelineCache\static\PPSA01342_01.005.000.binaries (3.838 MB; 0 shards split)`.
  O arquivo `.binaries` tem 4.024.416.066 bytes e o `.shaders` 225.637.396 bytes.
- Sem seeds de PPSA01342, o script gerou `seeds-PPSA01342_01.005.000.seeds`: 20.382 programas (10.692 compute, 2.427
  vertex, 7.263 pixel) e 55.382 pipelines gráficos, em 67 s.
- Sem hints de PPSA01342, usou `hints-PPSA01341_01.005.000.hints`: 6.997 programas e 6.991 pipelines, com "0 programs,
  0 pipelines" sem código no seed. Ou seja, os hints da PPSA01341 1.05 bateram inteiros com o dump americano 1.05.
  "ready to play" saiu em 00:12:27.
- 582 shards (70 dos hints + 512 das seeds), 6 por vez, 0 divididos, todos os `.err.log` vazios.
- **5 programas rejeitados no planejamento** (`plan-seeds.out.log`), dos 28.483:
  - 4: `shader binding layout failed: image 0 has an invalid binding class` (`BindingLayout.cpp:408`)
  - 1: `shader resource tracking: hash=0x0733bcf0f4626e8e stage=pixel pc=0x000002d0 GetBufferResource dword 0 is not
    a valid runtime value: Phi merges 2 unequal values: [0] ReadConst slot=20 [1] GetUserData s0` (`ResourceTracking.cpp:173`)
- CPU durante a pré-compilação (LibreHardwareMonitor 0.9.6, amostras entre 22:28 e 22:54): Tctl 78–80 °C, package
  cerca de 78 W, clock médio 4,1–4,6 GHz.

## Pendente para completar o 001

1. ~~Dump e versão~~ feito: `F:\PPSA01342-app`, 01.005.000.
2. ~~Atualização do driver NVIDIA~~ feita: 617.42.
3. ~~Pré-compilação~~ feita (ver "Pré-compilação" no Complemento).
4. `.\run-windows.ps1 -Game <dump>`: confirmar a entrada no jogo.
5. Save de benchmark em Boletaria 1-1 (`F:\dev\bench-save` em vez de `C:\dev\bench-save`, por espaço em disco no C:).

## Arquivos anexos

- `ctest.log`: saída completa do ctest.
- `build-errors-before-fix.txt`: erros de compilação e link do `kyty_tests` em dc29634.
- `build-summary.txt`: commits, estado do working tree, tempos e SHA-256 dos builds.
- `build-987dd7c-summary.txt`: build de `fix/bench-save-region`, SHA-256 e versão do jogo.
- `tm5.log`: log do TestMem5 (as duas rodadas interrompidas).
- `precompile.log`: saída do `precompile-windows.ps1` (seeds, hints, progresso, mensagem final).
- `precompile-plan-seeds.log`: planejamento das seeds, com os 5 programas rejeitados.
