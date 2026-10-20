# Contexto completo: Demon's Souls no KytyPS5, PC do Daniel (atualizado 2026-10-20)

Você assume tudo daqui em diante: medir no PC **e** mudar o código do emulador. Até agora um agente na nuvem fazia o
código e um agente local medía; o da nuvem saiu. Este arquivo tem tudo o que ele sabia.

## 0. Regras contra erro (as mais importantes)
- Só escreva números que saíram de um comando que você rodou, com a saída bruta anexada. Sem medição: "não medido".
- Antes de citar um script, switch `KYTY_*` ou função, abra o arquivo e confirme que existe.
- Comando falhou: cole o erro exato. Nunca invente o resultado.
- Uma mudança por branch, medida com A/B. Não ficou claramente melhor: apague o código (não acumule switches desligados).
- Diga o que vai fazer antes de cada passo e o que aconteceu de fato depois.

## 1. Objetivo
Demon's Souls a **30+ fps estáveis** (inclusive o 1% low) no PC abaixo, depois o máximo possível. Ordem de prioridade
(do guia dos autores): **crashes primeiro, depois quedas (1% low), depois fps médio**. Nunca troque estabilidade por fps.

## 2. O PC
| Item | Valor |
|---|---|
| CPU | AMD Ryzen 7 5700X, 8 núcleos / 16 threads (Zen 3, SMT, sem P/E-cores). Lógicos 0 e 1 = mesmo núcleo |
| GPU | RTX 3060 Ti 8 GB, driver 617.42. Budget Vulkan 6349–7256 MiB; o emulador a trata como "small" |
| RAM | 32 GB: 4×8 GB DDR4 de kits diferentes, rated 2666, **rodando a 2933** (estabilidade NÃO testada; TestMem5 interrompido aos 2–4 min, 0 erros). Crash/glitch aleatório: suspeitar da RAM primeiro |
| Placa-mãe | Gigabyte B450M DS3H V2, BIOS F67d |
| Windows | 11 Pro 25H2 (26200.9711), memória virtual gerenciada pelo sistema, plano de energia Equilibrado |
| Disco | Repositório, builds e dump em F: (NVMe). Clone em `F:\dev\KytyPS5` |
| Monitor | 2560×1440, 164 Hz. **60 Hz em toda medição** |
| PowerShell | Scripts bloqueados por política: sempre `powershell -ExecutionPolicy Bypass -File <script>` |
| Toolchain | VS Build Tools 2022 17.14, LLVM clang-cl 19.1.7 (`C:\Program Files\LLVM`), CMake 3.31, Ninja 1.12, Vulkan SDK 1.4.363, Python 3.12, Git 2.54 |

## 3. O jogo
- Dump: `F:\PPSA01342-app` (do PS5 do próprio usuário). **PPSA01342 (EUA), versão 01.005.000.**
- O fork foi ajustado na **PPSA01341 1.07**. Na 1.05 os modos 1080p e a regra de iluminação do fork não funcionam (ver §7).
  Se um dia o usuário atualizar o jogo para 1.07 no PS5 e refizer o dump, refazer a pré-compilação (~1 h).
- Save: `_SaveData\PPSA01342`. Save de benchmark: `F:\dev\bench-save` (ainda não criado).

## 4. Decisões já tomadas
- **AnyPS5** (boykopovar/AnyPS5) descartado para jogar: o Demon's Souls chega in-game mas a 0,1–1,3 fps até em RTX 5090.
- **KytyPS5 for Demon's Souls** (fork exploitz, base chenxiao07) escolhido. Autor mede 48–74 fps num i9-12900K + 3080 Ti.
  Os vídeos de "120 fps" do YouTube são KytyPS5 com frame generation.
- Fork do usuário: **https://github.com/dekuni/KytyPS5**. Nunca abra PR no upstream (exploitz/chenxiao07) sem pedir.

## 5. Branches do fork
| Branch | Commit | O que é |
|---|---|---|
| `demons-souls` | `dc29634` | release 1.0.0-beta.2 do exploitz, sem mudanças |
| `fix/windows-test-build` | `f225a62` | `kyty_tests` compila e linka no Windows (só testes/CMake) |
| `fix/bench-save-region` | `987dd7c` | + `tools/local/bench-windows.py` troca o save de PPSA01340/01341/01342 (antes só 01341: o benchmark rodaria no save real do usuário) |
| **`fix/run-windows-brace`** | **`91f388c`** | + `}` que faltava no bloco `-FsrFrameGen` do `run-windows.ps1` (bug da beta.2: o script não abria) |
| `fix/1080p-other-builds` | `445022a` | saídas 1080p em builds que não são PPSA01341 1.07 (1080f = 4K com 3D a 1080 linhas; 1080q = 4K) + localizador de patch sem âncora de enchimento + teste no ctest |
| `fix/bench-tools-1.05` | `dcdd138` | `bench-windows.py` exige o save do título do jogo e `reset` usa o baseline do `prepare`; `bench-run` fecha o jogo sem HUD; `profreport` é alvo CMake; `-StartupSeconds` repassado |
| `fix/vram-fallback-log` | `0d7b891` | log "video memory full" continua depois da 8ª vez e descreve a imagem |
| `fix/name-compile-threads` | `884c2ba` | nomes nas threads de compilação (`Kyty.Compile`, `Kyty.TableCompile`, `Kyty.Optimize`) |
| `test/integration` (só local) | `87b625a` | os quatro acima mesclados sobre `fix/run-windows-brace`: o exe para jogar e medir (`ctest` 41/65, mesmas 24 falhas da base) |
| `reports` | — | relatórios em `reports/NNN-<rótulo>/` e este arquivo em `handoff/` |
| `claude/push-test` | `dc29634` | branch de teste, sem conteúdo. O usuário pode apagá-lo no site |

**Base de todo trabalho novo: `fix/run-windows-brace`.** Para jogar e medir, use um exe com as correções acima
(`test/integration`, ou refaça a mescla dos `fix/*`). Relatório das correções: `reports/002-correcoes-codigo/`. Commits em Conventional Commits (`fix(...)`, `perf(...)`).
Um branch por mudança: `fix/<nome>` ou `perf/<nome>`. Push no fork.

## 6. Estado medido (relatório `reports/001-ambiente/`)
- Builds ok (`build-windows.cmd`: -O3 -march=native ThinLTO + PGO do repositório). `ctest`: **40/64 passam**; as 24 falhas
  são a linha de base (lista no report.md). Falha NOVA em um branch = regressão. O CI do projeto não roda testes.
- Pré-compilação: 58 min 55 s, `_PipelineCache\static\PPSA01342_01.005.000.binaries` (45.735 pipelines, 3.831 MiB).
  Hints da PPSA01341 1.05 usados e 100% compatíveis. 5 programas rejeitados no planejamento.
- **Primeira sessão** (25 min, tutorial até o primeiro boss, sem crash): 313 quadros >100 ms, 89,7 s travados; **53% por
  compilação de pipeline** (47,3 s; 822 linhas `SLOW *Pipeline`, das quais as 143 `SLOW Create*` síncronas travam o quadro;
  pior quadro 12,9 s). Os 152 quadros do minuto 23:34 são o fechamento do jogo. (Corrigido em 2026-10-20: a 1ª análise
  usou um log copiado 30 s antes do fim e dizia 219 / 77,5 s / 61%.) Ao fechar, o emulador salvou o warmup
  (`Shader warmup: saved 3162 inputs and 3064 pipelines`, 14 MB em `_PipelineCache\warmup-v2`) e os caches do driver
  (`local\<key>\PPSA01342.bin` 36,6 MB e `.table.bin` 31,9 MB): a 2ª sessão deve carregá-los. Log: `reports/001-ambiente/session-emulator.out.log.gz`;
  análise: `session-analysis.md` e `session_analysis.py`.
- **GPU quente**: hot spot médio 97,6 °C, máx 108 °C; diferença hot spot–núcleo 20–30 °C (pasta/pads ou poeira); clock cai de
  ~1.970 para ~1.750 MHz (~11%). Problema de hardware: limpar, curva de ventoinha, talvez trocar pasta. Recomendado ao usuário.
- **VRAM no limite**: 7,0–8,2 GB usados para 7,1 GB de budget (acima do budget em 9 de 40 amostras).
- Ainda **não há nenhuma medição de fps** (o log só registra quadros >100 ms).

## 7. Análise do código feita pelos agentes (íntegra em `handoff/analise-agentes.md`)
8 agentes só-leitura: 4 investigaram, 4 verificaram tentando refutar. Áreas Ryzen e VRAM ficaram **sem verificação**.

| Área | Proposta | Veredito |
|---|---|---|
| 1.05 | **fix-1080p-other-builds**: os modos de saída 1080p **abortam** com PPSA01342; tratar outra região do mesmo jogo como o mesmo título em `src/loader/gamePatch.cpp` (`ValidateGame`, como `OtherRegion()` em `shader-hints.cpp`), `same_version` só com mesmo título+versão, e mapear as saídas por build em `LaunchPlan.cpp`, `run-windows.ps1` (bloco `-Output`) e avisos do `MainWindow.cpp` | **implementar** (detalhes e correções do verificador no arquivo) |
| 1.05 | lighting-rule-105: achar os 4 shaders de iluminação/névoa da 1.05 e pôr na lista (muda SPIR-V: refazer pré-compilação) | medir antes |
| 1.05 | file-follow-other-build | medir antes |
| 1.05 | hints-105-coverage | já existe |
| 1.05 | hud-1080p-port-105 (HUD 1080p na 1.05 por assinatura mascarada) | medir antes (grande) |
| shaders | async-compute-table-fast-build: compilação rápida (sem otimizar) também nos pipelines compute assíncronos de tabela | medir antes |
| shaders | covered-by-local-only; native-variants-recorded-tier; foreground-first-driver-compiles | medir antes |
| shaders | table-forms-of-guesses | errada (descartar) |
| Ryzen | uniform-cpu-auto-placement: em CPU sem P/E-cores, `auto` = threads de render/gravação/upload em todos os CPUs menos o núcleo do CPU 0 | verificado 2026-10-20: medir antes (passo 8; ganho do guia era de Intel híbrido com HT off) |
| Ryzen | avoid-list-floor (desnecessária); background-compile-switches (medir antes); pgo-local-training (fazer: `pgo-train.ps1` não depende de 1.07) | verificado 2026-10-20 (relatório 002) |
| VRAM | free-replaced-pipelines e collector-os-usage: erradas; texture-reduce-auto e residency-priorities: medir antes; contador de falhas: implementado (`fix/vram-fallback-log`) | verificado 2026-10-20 (relatório 002) |

## 8. Plano, em ordem
1. **Hardware (usuário):** resfriamento da GPU. Registrar antes/depois (hot spot e clock) se mexer.
2. **Diagnóstico de shaders:** 2ª sessão, mesma rota do tutorial, com
   `powershell -ExecutionPolicy Bypass -File .\run-windows.ps1 -Game F:\PPSA01342-app -Set KYTY_PIPELINE_SOURCE_LOG=1,KYTY_PIPELINE_KEY_LOG=1,KYTY_FPS_HUD=2`
   com o exe do `test/integration` (dry-run conferido). O save já passou do tutorial: a rota é dele até Boletaria 1-1.
   Analisar com `reports/001-ambiente/session_analysis.py` (tem seção PIPESRC), com o log copiado depois de o emulador fechar.
   Comparar com a 1ª: número de `SLOW *Pipeline`, e contar linhas `PIPESRC ... static=0` vs `static=1` (código em
   `src/graphics/host_gpu/renderer/pipeline/shaders.cpp`, `PipelineSourceLog`). Se as travadas sumirem, era só a 1ª execução.
   Se muitas vierem `static=0`, implementar as propostas de shaders do §7 (uma por branch, A/B).
2b. **FEITO 2026-10-20 (relatório 003):** sessão 2 mostrou 104 compilações na render thread (53,8 s) de pipelines que a pré-compilação padrão pula (`--covered-by`). Próximo: `precompile-windows.ps1 -Full` (e daí em diante sempre `-Full`, senão a poda os remove).
3. **`fix-1080p-other-builds`: IMPLEMENTADO** (branch acima; dry-run e teste unitário ok). Falta no jogo: Testar "1080p Performance" e "1080p Quality" na 1.05: o jogo abre?
   HUD no lugar? Screenshot. Medir fps contra a saída 4K (a 3060 Ti deve ganhar: ~44% menos pixels).
4. **Save de benchmark** em Boletaria 1-1 (ponto onde andar 10 s sai do túnel para as ruínas). `KYTY_BENCH_BASELINE=F:\dev\bench-save`;
   `python tools\local\bench-windows.py prepare` antes e `restore` depois (tem que dizer "verified").
5. **Relatório 004-base** (002: correções de código; 003: sessão 2): `tools\local\windows\bench-run.ps1 -Label base` e `walk-run.ps1 -Label base`.
6. **005-perfil:** `tools\local\windows\prof-spot.ps1 -Label base -Seconds 8 -Top 80` (o `prof.txt` diz onde a thread de render gasta).
7. **006-cpu-vs-gpu:** (os convars de `-Quality` existem no eboot 1.05: conferido) mesma cena variando Output e Graphics detail ("Standard" vs "Fewer shadows").
8. **007-afinidade Ryzen:** `tools\local\windows\ab-exe.ps1` com o mesmo exe,
   `-SetB 'KYTY_RECORDING_CPUS=2-15,KYTY_RENDER_CPUS=2-15,KYTY_BACKGROUND_CPUS=0-15'` (as duas primeiras juntas: o
   `run-windows.ps1` copia RECORDING para RENDER antes do `-Set`; sem a terceira o teste fica confundido). Se ganhar,
   verificar e implementar `uniform-cpu-auto-placement` (§7) em `run-windows.ps1` (`Get-PerformanceCpus`) e no launcher.
   O guia diz: render nunca no CPU 0 (no Intel deu +2,5%), nunca sozinha num núcleo (piorou as quedas).
9. **PGO local** (conferido: o perfil do repositório não casa 4.235 funções em namespace anônimo deste build, porque o
   nome leva um hash do caminho do arquivo; 5,6% das chamadas do perfil, incluindo funções quentes) (o perfil do repositório foi treinado num Intel; retreinar deu +3% ao autor): `tools\local\windows\pgo-train.ps1`,
   `tools\local\PGO.md`. Conferir antes se ele aceita PPSA01342.
10. **VRAM:** verificar as propostas do §7 no código; testar `run.cmd`/`run-windows.ps1 -TextureReduce 2` contra o padrão.
11. **Iluminação 1.05** (`lighting-rule-105`): só depois, precisa dos hashes dos shaders da 1.05 e refazer a pré-compilação.
12. Opcional: investigar as 24 falhas de teste.

## 9. Como medir e decidir (resumo de `.claude/skills/ps5-emu-optimization/references/methodology.md`)
- Leia antes de mexer em desempenho: `.claude/skills/ps5-emu-optimization/SKILL.md`, `references/general.md` (o que deu certo e
  a **tabela de becos sem saída: não repita sem argumento novo**), `references/demons-souls.md`, `docs/BENCHMARKING.md`.
- A/B no mesmo processo quando houver switch (`ab-spot.ps1`), alternando A B A B, 4–6 rodadas; senão dois exes (`ab-exe.ps1`),
  ≥2 boots por lado (variação entre boots ±3–5%). Para efeitos <3%, compare ms/frame da render thread, não fps.
- Só otimização de lógica que comprovadamente remove trabalho ou espera, para o jogo inteiro. Nada de limiar por cena.
- **Não mude o SPIR-V emitido** sem avisar: obriga a refazer a pré-compilação (~1 h neste PC).
- Um emulador por vez; nunca compile com o jogo aberto; nada pesado durante medição.
- Nunca rode benchmark no save real; nunca apague `_SaveData`, `_PipelineCache` ou `F:\dev\bench-save`.

## 10. Relatórios (`reports/NNN-<rótulo>/report.md`, branch `reports`)
Campos: data/hora; branch, commit e SHA-256 do exe; velocidade da RAM; switches; configuração do jogo (Output, 3D resolution,
Graphics detail, AA, frame generation, limite, VSync, janela/tela cheia); tabela (cena, variante, fps médio, 1% low, p99 ms,
render ms/frame, rodadas); conclusão; problemas vistos; anexos com saída bruta.
O fork é **público**: nunca envie arquivos do jogo, saves, `seeds-*.seeds`, `hinted-*.seeds`, dumps de shader, `_PipelineCache`
nem caminhos com o nome de usuário do Windows (troque por `<user>`).

## 11. Pendências que dependem do usuário
- Resfriamento da GPU; decidir se testa a RAM a fundo (MemTest86 por boot) ou volta para 2666.
- Apagar o branch `claude/push-test` (https://github.com/dekuni/KytyPS5/branches), se quiser.
- Se quiser, avisar o exploitz do bug da `}` em `run-windows.ps1` (https://github.com/exploitz/KytyPS5/issues).
