# Análise dos agentes, 2ª rodada (2026-10-20): verificação e achados

Seis agentes só-leitura sobre `fix/run-windows-brace` (91f388c, worktree `F:\dev\KytyPS5-ro`), com o log completo da
sessão 1 (`reports/001-ambiente/session-emulator.out.log.gz`). Texto condensado dos relatórios; números de linha são do
91f388c. O que foi conferido à mão está marcado ✔; o resto é afirmação do agente, conferir antes de agir.
O que já virou código está em `reports/002-correcoes-codigo/`.

## 1. Persistência do warmup e dos caches (✔ conferido)
- A sessão 1 salvou tudo ao fechar a janela: `warmup-v2\cc41836a2868fd05\PPSA01342_01.005.000.shaders` (14.184.032 B,
  23:34:18), `local\1f3c29b7…\PPSA01342.bin` (36.585.154 B) e `.table.bin` (31.889.264 B). Log final: "Shader warmup:
  saved 3162 inputs and 3064 pipelines", "Vulkan pipeline cache: saved 36585007 bytes".
- A conclusão anterior ("o warmup talvez não tenha sido salvo") veio de uma cópia do log feita 30 s antes do fim.
- Caminho do fechamento: SDL_QUIT → `need_exit` (window.cpp:598, 232-236) → `StopWarmup`, `ReportSession`, `Save`
  (window.cpp:1031-1034) → `std::quick_exit(0)` (emulator.cpp:180-181). Há também um gravador do warmup a cada 5 s
  (shader-warmup-cache.h:265-289) e o salvamento periódico dos caches do driver depois de 20 s sem compilar
  (pipelineCache.cpp:3575-3606). `EXIT()`/fatal perde no máximo os últimos ~5 s / ~20 s.
- A 2ª sessão carrega os caches: chave fixa `KYTY_DRIVER_CACHE_KEY` (run-windows.ps1:389), assinatura com
  vendor/device/driver (0x9a4a8000 = 617.42; atualizar o driver invalida). Esperado no log: "loaded 36585007 bytes",
  "Shader warmup: loaded 3162 shader inputs and 3064 pipeline recipes", e **não** "no recorded play on this PC".
- `PPSA01342.bin` / `.table.bin` na raiz de `_PipelineCache` (153 B) são caches vazios do passo plan-recorded da
  pré-compilação; inofensivos.

## 2. Ferramentas de medição na 1.05 (✔ conferido; corrigido em `fix/bench-tools-1.05`)
- Nenhum script usa endereço/hash/comando exclusivo da 1.07 (gamecmd, glide, warp, teleport: 0 ocorrências). O jogador é
  posicionado por save + Continue (máquina de estados por pixels, bench-run.ps1:184-216) + teclas.
- Bugs: `bench-windows.py` aceitava baseline de qualquer região (55) e `reset` usava o `KYTY_BENCH_BASELINE` do shell e
  dizia "reinstalled" mesmo sem save (74-87); `bench-run.ps1:221` saía sem HUD deixando o jogo aberto; `prof-spot.ps1`
  usava `_Build\windows-tools\profreport.exe` que nada compilava. Todos corrigidos.
- Restam (não corrigidos): `ab-spot.ps1` repete a partida se o ponto passar de 55 fps (use `-MaxFps 0`);
  `tools/local/demons-souls-save.py` tem PPSA01341 fixo (47, 68, 119, 148); `docs/BENCHMARKING.md` descreve o fluxo Linux
  (`benchmark-entry.py`, só PPSA01341); `bench-run.ps1:152` roda `-Precompile` antes de cada partida.
- Antes de cada passo: baseline em `F:\dev\bench-save\PPSA01342\` (+ `.kyty-capacity\PPSA01342`), `game-path.txt`
  apontando para `F:\PPSA01342-app`, e na primeira rodada `$env:KYTY_BENCH_SHOTS=<dir>` para ver a detecção de telas.
  Conferir no `.out.log` as linhas de patch: "Demon's Souls copy: installed", "GPU buffer pages: … installed",
  "idle wait: installed", "scene guard: installed", "touch guard: installed".

## 3. Ryzen 5700X: posicionamento de threads e PGO
- ✔ Em CPU sem P/E-cores, `auto` não fixa nada: `Get-PerformanceCpus` (run-windows.ps1:434-466) devolve vazio com menos
  de 2 classes de eficiência e o bloco auto (467-473) remove as chaves. Sessão 1: "affinity: 0xFFFF (16 CPUs)" sem sufixo.
- Prioridades: render (Kyty.Gpu) HIGHEST (performance-switches.h:193-194); Record/Upload/Present NORMAL; prefetch,
  follow, warmup e as 3 pools de compilação BELOW_NORMAL (local-platform.cpp:850-853); job workers do jogo LOWEST.
- O +2,5% do guia (CURRENT-PLAYABLE.md:56-57, 219-220) foi medido num 14900K com HT desligado, lista "1,2,3,6,7",
  comparado com posicionamento livre que incluía E-cores: não transfere para o 5700X. Sem medição em CPU SMT uniforme.
- Propostas: uniform-cpu-auto-placement → medir antes; avoid-list-floor → desnecessária (`KYTY_BACKGROUND_CPUS=0-15`
  resolve, local-platform.cpp:835-840); background-compile-switches → medir antes (já existem
  `KYTY_SHADER_PREFETCH_THREADS` :1447, `KYTY_SHADER_FOLLOW_THREADS` :1463, `KYTY_SHADER_WARMUP_BACKGROUND_THREADS`
  :2377; faltam switches para as pools e a prioridade). Evidência no código: pipelineCache.cpp:1469-1473 (8 threads em
  irmãos SMT esticaram o carregamento de 20 s para 120+ s num 8C/16T) e TableWorkers = 16/2 = 8 aqui (:2796).
- ✔ Armadilha do teste 006: `KYTY_RECORDING_CPUS` é copiado para `KYTY_RENDER_CPUS` (run-windows.ps1:281) antes do
  `-Set` (:436). Usar `KYTY_RECORDING_CPUS=2-15,KYTY_RENDER_CPUS=2-15,KYTY_BACKGROUND_CPUS=0-15`.
- A/B no mesmo processo (sem rebuild): comando live `pin 0-15` / `pin 2-15` (live-control.h:556-559, só Kyty.Gpu) e
  `pinthread <tid> 2-15` (:393-398) para Record/Upload; ABBA, 4 rodadas, `measure 8`; ignorar pontos com render_busy
  bem abaixo de ~90% (GPU-bound).
- **PGO (✔ conferido):** o perfil `tools/pgo/kyty.profdata` tem 31.347 funções; 4.235 com `?A0x<hash>` (namespace
  anônimo, hash do caminho do arquivo principal no clang-cl). Ex.: `vulkan-recording.cpp` = `?A0x1DABC782` no objeto
  daqui, `?A0xF397ABF2` no perfil (0 ocorrências do daqui). São 5,6% das contagens de entrada do perfil, incluindo
  `pm4Handlers.cpp NormalizeRegisterOffset` (1,5 bi), `Program.cpp EquivalentValue` / `IsRuntimeRead` (~1,1 bi cada),
  `LaneLocal.cpp ScalarIndex` (1,06 bi), `vulkan-recording.cpp Writer::Allocate` (0,76 bi). Funções `static` usam só o
  nome do arquivo e casam. Agente: o perfil foi treinado em 6ce72d35 (commit 6bae2db5, chenxiao07, notebook RTX 3080 +
  16 GB) e o código mudou 120 arquivos / +10.782 linhas desde então (não conferido). `pgo-train.ps1` não depende de 1.07
  e usa a rota do prólogo. Passos: baseline save + `prepare`; copiar o exe atual para `_Build\ab\base\`;
  `pgo-train.ps1`; juntar com o do repositório (`llvm-profdata merge -o _Build\pgo\windows\kyty.profdata
  <novo>.profraw tools\pgo\kyty.profdata`); `build-windows.cmd` (usa `_Build\pgo\windows\kyty.profdata` se existir);
  `ab-exe.ps1 -A _Build\ab\base\kyty_emulator.exe -B _Build\windows\kyty_emulator.exe -Rounds 2 -StartupSeconds 240`.

## 4. Memória de vídeo (8 GB)
- Placa "small" (heap < 11,5 GiB, vma.cpp:152-160): budget dos caches = budget VMA − min(budget/8, 1 GiB) −
  `KYTY_VRAM_RESERVE_MB` (vma.cpp:318-325) = 6349 MiB. Texturas só amostradas/copiadas vão para o tipo de memória sem
  flags depois de metade do budget (vma.cpp:477-481); render targets, depth e storage pedem device-local e, se falhar,
  esvaziam o pool e vão para a memória do sistema (vma.cpp:510-521) sem nunca voltar.
- ✔ `KYTY_TEXTURE_REDUCE` padrão 0 (descriptors.cpp:572-595); a sessão 1 não o usou.
- Coletores: texturas 3789/4813/~5735 MiB (textureCache.cpp:350-352), crítico vivo = max(budget VMA − 256, estático);
  buffers 4813/~5735 (bufferCache.cpp:1474-1489). Custos fixos: tabela BDA 512 MiB, pool de imagens até 396 MiB, warmup
  de pipelines compute até budget/16 (~396 MiB; a sessão 2 vai usar, a 1 não: comparar só sessões ≥2 entre si).
- Sessão 1 (log completo): 40 amostras, 9 acima do budget. No boss: quadro de 348,9 ms com 839 MB de upload, quadro de
  1647,6 ms (idle 1216), salto para 7939 de 7028 MiB, **8 falhas device-local num quadro** (imagens de 36.044.800 a
  40.632.320 B em passos de 640 KiB: a mesma imagem recriada maior), quadro de 3679,6 ms (cpu 1921 ms, 1,66 GB de upload),
  depois ~20,8 s a ~7 fps (151 quadros SLOW, idle médio 106 ms), recuperação quando o uso caiu. Sem crash.
- VMA vs DXGI: a diferença (~1952–2318 MiB) é o total do tipo "plain"; o driver contabiliza esse tipo no heap 0; a linha
  "plain type X MiB in video memory" é um limite superior.
- Caminhos de abort se a VRAM acabar (nenhum trata VK_ERROR_OUT_OF_DEVICE_MEMORY): imagem cujo fallback também falha
  (image.cpp:874-879), buffer que falha 2× (streamBuffer.cpp:243-248), criação de pipeline (shaders.cpp:677-679,
  1198-1199, 1295-1296), submit (commandScheduler.cpp:625-628), device lost (masterSemaphore.cpp:34-35, 76-85). Com 32 GB de
  RAM o fallback em memória do sistema dificilmente falha; crash em Boletaria possível, não demonstrado; o risco
  prático é o 1% low.
- Propostas: vram-collector-line → só o contador de falhas (feito: `fix/vram-fallback-log`; o comando live `vma <path>`
  já dá VMA+DXGI no mesmo instante); texture-reduce-auto → medir antes (falha de projeto: ao desligar, a textura
  reduzida é recriada inteira — descriptors.cpp:611-612 retorna antes do lookup de 620-627 — onda de re-upload);
  collector-os-usage → direção errada nesta placa (proxy sem rebuild: poke `kyty_local_texture_gc_critical_mb`,
  textureCache.cpp:42-47); residency-priorities → medir antes (precisaria de VK_EXT_pageable_device_local_memory; ver no
  vulkaninfo do 617.42); free-replaced-pipelines → errada (só 200 substituídos, ~14 MiB).
- A/B depois: `-TextureReduce 1` vs nada, 2 boots por lado, mesma rota; conferir a linha "Textures: large
  block-compressed…"; comparar amostras acima do budget, contagem "video memory full", cauda de quadros lentos.
  Candidato geral futuro (medir antes): na 1ª falha device-local, no próximo flip `TrimImagePool()` + `collect(true)`.

## 5. Compilações que travam o quadro
- ✔ 143 compilações síncronas (81 `SLOW CreateComputePipeline` + 62 `SLOW CreateGraphicsPipeline`). Fim da sessão:
  "Pipelines compiled unoptimized first: 200", "3064 pipelines (54207 ms)" feitos na render thread: store/cache do driver
  serviram 2.864 de 3.064 (93,5%).
- Só os dois caminhos **normais** são síncronos: `CreateComputePipeline` (renderCompute.cpp:943-944, depois que
  `TableDispatch` recusa em 676-679) e `CreateGraphicsPipeline` (renderDraw.cpp:1547), dentro de `WaitScope(WaitCompile)`
  (pipelineCache.cpp:4178, 4251). Native XPR e tabela são assíncronos.
- Grupos (agente): 62 compute (19,75 s) = nova especialização de imagem de um programa já em modo tabela, enquanto a
  compilação de tabela dele está pendente (`TableSet` devolve null, table-xpr.inc:2595; caminho "another specialization"
  683-697) — inclui os 2 piores quadros (51 e 10 compilações); 7 compute (5,6 s) antes do modo tabela; 12 compute (8,23 s)
  no boot; 55 graphics (12,05 s) primeiro draw de permutação nova; 7 graphics (1,11 s) estado/par novo.
- Distribuição: 131 no jogo em 52 quadros; 6 quadros com ≥4 compilações somam 25,75 s; 39 quadros com exatamente 1.
- Unoptimized vs optimized do mesmo hash (módulos diferentes, confundido): p50 1,53× (n=78) — o 150× do fork não se
  reproduz aqui. O tempo não cresce ao longo de uma rajada (fraca evidência contra serialização no driver).
- Propostas: **P1** bind adiado: compilar num worker de primeiro plano e fazer o worker de gravação (não a render thread)
  esperar no bind (pipelineCache.h `Pipeline` com handle pendente; `CreatePipelineInternal` dividido; pacote de bind que
  espera `done`) — nada é pulado; teto ~−20 s nas rajadas, ~0 nos quadros de 1 compilação; risco de deadlock (worker de
  compilação nunca pega o lock de render nem espera o gravador). **P2** exact-on-miss (só se PIPESPV mostrar módulos
  portáveis muito maiores). **P3** ponte de tabela para o grupo de 62. Rejeitado: pular draws/dispatches. Decisão: medir
  antes (sessão 2 com cache quente).
- Sessão 2: `KYTY_PIPELINE_SOURCE_LOG=1,KYTY_PIPELINE_KEY_LOG=1` (shaders.cpp:604, 417). Linhas `PIPESRC kind key static
  build unoptimized ms` (shaders.cpp:1214, 1302; build=2 é a reconstrução otimizada), `PIPESPV key spirv`, `PIPEKEY miss`.
  Para decidir se `-Full` serviria: `kyty_shader_precompile --game F:\PPSA01342-app --seeds
  _Build\static-precompile\seeds-PPSA01342_01.005.000.seeds --dry-run --timings t.txt` com `KYTY_EXEC_DEMAND=1` e os
  switches de tradução de precompile-windows.ps1:76-82; a coluna 6 é a mesma identidade do `key=`.

## 6. As 24 falhas do ctest
Nenhuma indica crash/glitch no jogo com os switches padrão. Únicos pontos de atenção:
- Bug real, só Windows: `recv` com PEEK+WAITALL (`src/libs/network.cpp` `ConvertMessageFlags` ~1008-1016) dá
  WSAEOPNOTSUPP. Correção proposta: no Windows não pôr MSG_WAITALL quando há MSG_PEEK (aproximação). Só importa se o jogo
  usar isso (não verificado).
- `FindView` não aborta mais numa view incompatível (imageView.cpp:345-381): procurar "incompatible view(s) replaced" nos
  logs do jogo (possível glitch visual).
- Por teste: shader_cfg (`KYTY_LANE_LOCAL=0` passa), memory_tracker (época move de propósito, 75aa4b9e), resource_tracking
  (`KYTY_PORTABLE_SHADERS=0` passa), kernel_file_system (`KYTY_AMPR_SYNC=0` + o bug do recv), fiber_migration (teste não
  inicializa a memória do kernel), audio_out2_port (`KYTY_AUDIO2_DEVICE_CLOCK=0` passa), shader_recompiler_compute (casos
  volume-mip-count/slice-range não abortam mais), cpu_write_window e write_window_handoff (crash no fim: `SetGpu(nullptr);
  ShutdownGpu();` antes dos locais morrerem), demons_souls_compute_chain (título corrigido; agora para na mesma asserção
  de compute_stream_submission: lote fica aberto até 256 dispatches desde 0c29dbf9), gpu_command_lane, texture_resolution_
  cache e texture_cache_image_overlap (imagens separadas de propósito, 6690b30c), compute_meta_clear_classification
  (fixture sem `MetaFilterAdd`), buffer_cache_dirty_gc (buffers deletados no unmap, 61cc543a), os 5 de
  UnifiedTextureCacheFlow (falta `Drain()` da gravação no teste; já falhavam no Linux em setembro), buffer_cache_guest_
  readback (ordem unmap/Finish no teste), copy_feedback (snapshot sobrevive desde 09bb6560), async_write_readback (só 100
  polls).
