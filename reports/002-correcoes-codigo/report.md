# 002 — correções de código (sem abrir o jogo)

**Status:** código pronto para a sessão 2. Nenhuma medição de fps neste relatório (o jogo não foi aberto: só será
aberto com autorização do usuário). Tudo abaixo foi compilado e validado sem o jogo; o que precisa do jogo está marcado.

- Data/hora: 2026-10-20, 00:50–06:30 (horário local do PC)
- Base: `fix/run-windows-brace` `91f388c2`. Branches novos (um por mudança, só locais até o push ser aprovado):

  | Branch | Commit | O que faz | Validação sem o jogo |
  |---|---|---|---|
  | `fix/1080p-other-builds` | `445022aa` | saídas 1080p não derrubam mais o PPSA01342; localizador de patch sem âncora de enchimento | dry-run nos dois builds, teste unitário novo |
  | `fix/bench-tools-1.05` | `dcdd138e` | `bench-windows.py`, `bench-run.ps1` e scripts que o chamam; `profreport` compila | teste em raiz falsa, parse, build |
  | `fix/vram-fallback-log` | `0d7b8911` | log de falta de VRAM continua depois da 8ª vez e diz qual imagem | build (só log) |
  | `fix/name-compile-threads` | `884c2ba3` | nomes nas threads de compilação (`Kyty.Compile`, `Kyty.TableCompile`, `Kyty.Optimize`) | build (só nomes) |
  | `test/integration` (local, não vai para o GitHub) | `87b625a4` | os quatro juntos sobre a base, para testar | build, `ctest` (abaixo) |

- `kyty_emulator.exe` do `test/integration`: SHA-256 `509DBDC3A6E239EA85B5839A0FAEBF8A3F6D55A3ED183C97E4A0724CB8F3758C`;
  `kyty_shader_precompile.exe` `63AED667F12C6CE764A44D28803947BF5C492646D9E6F6CBCCA52D3DD7247199`
- Velocidade da RAM: 2933 MT/s (estabilidade não testada).
- Nenhuma mudança toca `src/graphics/shader/` nem os headers da assinatura do recompilador
  (`src/generate_version.cmake:43-54`): a pré-compilação de ~1 h e o cache de tradução continuam valendo.

## 1. `fix/1080p-other-builds`

Problema (verificado no código): `run-windows.ps1 -Output 1080f/1080q` e o launcher passavam `patches\1080p-*.json`
(`"id": "PPSA01341"`, `"version": "01.007.000"`) para qualquer região. Em PPSA01342 o `ValidateGame`
(`src/loader/gamePatch.cpp:111-116`) recusa e o emulador para no boot com "Failed to apply game cheat".

Mudança:
- `run-windows.ps1` e `src/fork-launcher/.../LaunchPlan.cpp` leem `id` e `version` do arquivo de patch: só nesse build
  exato usam o patch e `KYTY_RENDER_CONVARS`. Em qualquer outro, 1080f = saída 4K com o 3D a 1080 linhas
  (`KYTY_RENDER_HEIGHT=1080`, a menos que `-RenderHeight` seja dado) e 1080q = saída 4K. Uma linha `output:` explica.
  `-Patch` manual mantém o comportamento antigo.
- `src/loader/gamePatchLocate.h`: numa busca (patch fora do próprio offset), gravações cujo byte original é um valor de
  enchimento repetido (0x00, 0xCC, 0x90) não contam para os 8 bytes mínimos nem servem de âncora. Evita que o mod de
  HUD da 1.07 (caverna de 9×0xCC + três edições de 1 byte) seja posto em qualquer sequência de int3 de outro build.
- Avisos do launcher, README e PORTABLE-README; teste do título (`IsSupportedTitle("PPSA01342")`).

Validação (saída bruta em `dryrun-1080p.txt`):
- PPSA01342 01.005.000: sem `--game-patch` e sem `KYTY_RENDER_CONVARS`; `-Output 1080f` → `KYTY_RENDER_HEIGHT=1080`;
  `-Output 1080f -RenderHeight 1296` → `1296`; `-Output 1080q` → só a nota.
- `param.json` falso de PPSA01341 01.007.000: patch e convars passados como antes, sem nota.
- `game_patch_locate_tests`: "GamePatchLocateTests: ok". Com o header antigo o teste novo falha
  ("FAILED: a padding run and 3 code bytes are not searched for"), ou seja, ele pega o problema.
- **Não compilado:** o launcher Qt (`KYTY_BUILD_FORK_LAUNCHER`, não há Qt neste PC).
- **Precisa do jogo:** confirmar que `-Output 1080f` abre e que o log mostra
  "Demon's Souls render size: performance mode renders 1920x1080 instead of 2560x1440".

## 2. `fix/bench-tools-1.05`

Auditoria das ferramentas (agente, conferida por mim nos pontos abaixo): nenhum script de medição usa endereço,
hash ou comando exclusivo da 1.07; o jogador é posicionado por save + teclas. Bugs encontrados e corrigidos:
- `tools/local/bench-windows.py`: `prepare` aceitava qualquer região (linha 55) e `reset` reinstalava o que houvesse em
  `KYTY_BENCH_BASELINE` no shell dele, dizendo "reinstalled" mesmo sem nada (74-87). Agora `prepare` exige a pasta do
  jogo de `game-path.txt` e grava o baseline; `reset` usa esse e falha se o save do jogo não voltou.
  Teste em raiz falsa (`bench-windows-test.txt`): a versão antiga instalou o save PPSA01341 (exit 0) e, sem a variável,
  disse "reinstalled" sem instalar nada (exit 0); a nova recusa o primeiro e instala certo no segundo.
- `tools/local/windows/bench-run.ps1:221`: sem HUD, saía deixando o emulador aberto, e `prof-spot`/`ab-spot`/`ab-exe`/
  `walk-run` mediam ou perfilavam um menu (`pgo-train` treinava nele). Agora fecha o jogo antes de sair; `pgo-train`
  para se o build instrumentado ou a partida falhar.
- `prof-spot.ps1` chamava `_Build\windows-tools\profreport.exe`, que nada compilava (a pasta não existia aqui). Novo
  alvo CMake `profreport`; `prof-spot` o compila antes de abrir o jogo se faltar. Compilado aqui sem avisos.
- `-StartupSeconds` repassado por `walk-run`, `ab-spot` e `prof-spot`.
- `run-windows.ps1`: a ajuda do `-TextureReduce` dizia "default 1 below 12 GB"; o padrão é 0
  (`src/graphics/host_gpu/renderer/pipeline/descriptors.cpp:572-595`).

## 3. `fix/vram-fallback-log` e `fix/name-compile-threads`

- Na sessão 1 o log mostrou 8 linhas "Vulkan: video memory full" num quadro (boss) e depois silêncio
  (`ReportMemoryFallback` só imprime as 8 primeiras, `src/graphics/host_gpu/vma.cpp:388-399`). Agora, depois da 8ª,
  no máximo uma linha por segundo com contagem e total, e a imagem é descrita (tamanho, camadas, níveis, formato, uso).
- As 3 pools de compilação não tinham nome de thread (`pipelineCache.cpp:2736-2741`); agora têm.

## 4. Achados da revisão do código (6 agentes só leitura; o que eu conferi está marcado ✔)

| Área | Achado | Decisão |
|---|---|---|
| Warmup/caches | ✔ A sessão 1 salvou o warmup ("Shader warmup: saved 3162 inputs and 3064 pipelines") e os caches do driver (36,6 + 31,9 MB). A 2ª sessão os carrega. | nada a mudar |
| Log da sessão 1 | ✔ Minha cópia do log foi feita 30 s antes do fim (895.720 de 933.115 bytes). Números corrigidos no 001: 313 quadros >100 ms, 89,7 s, compilação 53%. | corrigido |
| PGO | ✔ O perfil do repositório não casa as funções em namespace anônimo deste build: o nome leva um hash do caminho do arquivo (`vulkan-recording.cpp`: `?A0x1DABC782` no objeto daqui, `?A0xF397ABF2` no perfil, 0 ocorrências do daqui no perfil). 4.235 de 31.347 funções do perfil; 5,6% das chamadas registradas, entre elas algumas das mais quentes (`NormalizeRegisterOffset` 1,5 bi, `Writer::Allocate` 0,76 bi). Efeito em fps: não medido. | retreinar o PGO aqui (passo 9; precisa do jogo) |
| Convars 1.05 | ✔ Os nomes usados por `-Quality` e pelo padrão existem no eboot 1.05 (`doIndexBufferCulling` 1, `r_enableSpotShadows` 1, `r_enableAmbientShadowing` 4, `sunShadowMaxDistance` 1, `r_forceShadowLOD` 1, `r_forceViewportHeightForPixelSizeCalc` 1, `r_Wants4K` 2). | `-Quality` pode ser testado (passo 004) |
| Ryzen | ✔ Em CPU sem P/E-cores o `auto` não fixa nenhuma thread (log da sessão 1: "affinity: 0xFFFF (16 CPUs)" sem "render threads on CPUs"). ✔ `-Set KYTY_RECORDING_CPUS` sozinho não fixa a render (cópia em `run-windows.ps1:281` antes do `-Set` em :436). O ganho de +2,5% do guia foi medido num Intel híbrido com HT desligado. | medir antes (005): `KYTY_RECORDING_CPUS=2-15,KYTY_RENDER_CPUS=2-15,KYTY_BACKGROUND_CPUS=0-15` |
| Compilações | 143 compilações síncronas travaram quadros (81 compute + 62 graphics, 46,8 s pela análise anterior). O agente agrupou: 62 compute (19,75 s) são nova especialização de um programa já em modo tabela; os 2 piores quadros (51 e 10 compilações) estão nesse grupo. Proposta P1: compilar num worker e esperar no worker de gravação, não na render thread (médio-alto esforço, risco de deadlock). | medir antes: a sessão 2 (cache quente + `PIPESRC`) diz se ainda há travadas |
| VRAM | As 8 falhas do boss eram render targets/depth/storage (texturas não pedem VRAM depois da metade do budget, `vma.cpp:477-481`); a mesma imagem recriada em passos de 640 KiB. Propostas: `free-replaced-pipelines` errada (só 200 substituídos); `collector-os-usage` direção errada para esta placa; `texture-reduce-auto` e prioridades de residência: medir antes. | log novo (seção 3); A/B `-TextureReduce 1` depois |
| Testes | Nenhuma das 24 falhas indica crash/glitch no jogo com os switches padrão: são expectativas desatualizadas (mudanças de set–out) ou problemas do harness. Único bug real: `recv` com PEEK+WAITALL no Windows (`src/libs/network.cpp`), relevante só se o jogo usar isso (não verificado). | não prioritário |

## 5. Testes (`ctest`, branch `test/integration`)

`ctest --test-dir _Build/windows --output-on-failure --timeout 900` (ctest do VS Build Tools), working tree limpo:
**41 de 65 passam, 24 falham** (`ctest.log`). A linha de base (001) era 40/64.
- O teste a mais é `game_patch_locate`: existia mas não estava registrado no ctest (`add_test` e `kyty_tests` sem ele);
  registrado no `fix/1080p-other-builds` (commit `445022aa`). Passa.
- As 24 falhas são as mesmas da linha de base, comparadas pelo nome: nenhuma nova, nenhuma sumiu.
- `demons_souls_compute_chain` passa da checagem de título (corrigida) e agora para em "StreamingCompute failed at
  bounded asynchronous submissions", a mesma asserção de `compute_stream_submission` (expectativa antiga de lotes de
  dispatch, segundo o agente de testes).

## 6. Sessão 2 (precisa da autorização do usuário)

Comando (dry-run conferido: 60 switches, mesma chave de cache do driver da sessão 1, warmup ligado, limite de
travada 100 ms como na sessão 1):
```
powershell -ExecutionPolicy Bypass -File .\run-windows.ps1 -Game F:\PPSA01342-app -Set KYTY_PIPELINE_SOURCE_LOG=1,KYTY_PIPELINE_KEY_LOG=1,KYTY_FPS_HUD=2
```
- Rota: continuar do save atual até Boletaria 1-1 (o ponto do save de benchmark). Monitor a 60 Hz.
- Depois: copiar o log **só depois de o emulador fechar**; rodar
  `python -I -X utf8 reports/001-ambiente/session_analysis.py <log> F:\dev\tools\LibreHardwareMonitor` (agora com a
  seção PIPESRC: `static=1` vs `static=0` por tipo).
- Perguntas que ela responde: as travadas de compilação somem com o cache quente? As novas (áreas novas) vêm da
  pré-compilação (`static=1`) ou não? A VRAM falta de novo, e com qual imagem?

## Problemas vistos
- Erros meus, corrigidos: o `sed` de um teste temporário ficou no arquivo de teste e foi revertido antes do commit; um
  heredoc do shell engoliu barras invertidas (a edição falhou na asserção, nada foi gravado). A cópia antecipada do log
  da sessão 1 (acima).

## Arquivos anexos
- `commits.txt`: as mensagens dos 4 commits.
- `dryrun-1080p.txt`: dry-runs das saídas 1080p no PPSA01342.
- `bench-windows-test.txt`: `bench-windows.py` antigo e novo na raiz falsa.
- `ctest.log`: saída do `ctest` no `test/integration`.
