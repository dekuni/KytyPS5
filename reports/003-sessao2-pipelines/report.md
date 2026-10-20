# 003 — sessão 2: de onde vêm os pipelines (diagnóstico, não é benchmark)

- Data/hora: 2026-10-20, 10:30:13–10:55:53 (25 min 40 s), saída normal (exit 0), `.err.log` vazio.
- Branch `test/integration` `87b625a4` (os 4 `fix/*` do relatório 002 sobre `fix/run-windows-brace`), working tree
  limpo. `kyty_emulator.exe` SHA-256 `509DBDC3A6E239EA85B5839A0FAEBF8A3F6D55A3ED183C97E4A0724CB8F3758C`.
- RAM 2933 MT/s (estabilidade não testada). Configuração do jogo: a padrão (não alterada). Janela 2560×1440.
  O Windows informou 164 Hz no início (`Win32_VideoController.CurrentRefreshRate`), embora o usuário tenha dito 60 Hz.
- Switches extras: `-Set KYTY_PIPELINE_SOURCE_LOG=1,KYTY_PIPELINE_KEY_LOG=1,KYTY_FPS_HUD=2` (60 switches no total).
- Rota: do save da sessão 1 (depois do primeiro boss do tutorial) em direção a Boletaria 1-1; houve uma pausa no jogo
  (10:47–10:50 sem quadros lentos). Até onde o jogador chegou: não registrado.
- Temperaturas: não medidas (LibreHardwareMonitor aberto, mas sem "Log Sensors").

## Começo da sessão (caches da sessão 1)
- `Vulkan pipeline cache: loaded 36585007 bytes` e `loaded 31889117 bytes` (os tamanhos salvos ao fim da sessão 1).
- `Shader warmup: loaded 3162 shader inputs and 3064 pipeline recipes`; compilado em 3435 ms ao lado do jogo
  ("2611 from the translation store"). A linha "no recorded play on this PC" não aparece.
- Todos os patches da 1.05 instalados (idle/job wait, memmove, GPU buffer pages, scene e touch guards).
- `Shader prefetch: all 20374 programs done, 17752 from the translation store (0 rejected), 1225 MiB of SPIR-V translated`.
- O `run-windows.ps1` refez a lista de seeds uma vez ("listing the game's shaders from its files again (another
  precompile program)": o exe do pré-compilador mudou), por isso o emulador abriu 68 s depois do comando.
- Ao fechar: `Shader warmup: saved 4669 inputs and 4610 pipelines`; caches do driver salvos com 75.581.454 e 57.021.420 bytes.

## Resultados (análise completa em `session-analysis.md`)

| | Sessão 1 (tutorial, cache vazio) | Sessão 2 (áreas novas, cache da 1) |
|---|---:|---:|
| Quadros > 100 ms | 313 (89,7 s) | 186 (136,6 s) |
| … com compilação ≥ 50% do quadro | 75 (47,3 s) | 56 (101,4 s) |
| `compile=` somado | 43,9 s | 98,4 s |
| Pior quadro | 12.952 ms | **21.499 ms** |
| `SLOW CreateCompute` / `CreateGraphics` (travam o quadro) | 81 / 62 | 163 / 23 |
| `SLOW AsyncCompute` / `AsyncGraphics` (segundo plano) | 277 / 402 | 330 / 233 |
| Amostras de VRAM acima do budget | 9 de 40 | 11 de 34 |
| Linhas "video memory full" | 8 | 0 |

10 piores quadros da sessão 2: 21.499, 16.578, 9.816, 9.638, 8.061, 6.155, 5.224, 3.971, 2.510 e 1.571 ms; em todos,
`compile` é quase o quadro inteiro (ex.: 21.499 ms com `compile=21424`).

### Origem dos pipelines (`PIPESRC`)
8.749 linhas. `static=1` (vieram da pré-compilação): 6.390. `static=0`: 2.359 (2.131 sem as reconstruções otimizadas,
1.775 chaves distintas).

| Tipo | static=1 | static=0 | static=0 ≥ 100 ms | ms somados static=0 |
|---|---:|---:|---:|---:|
| cs-normal | 1705 | 455 | 251 | 147.660 |
| cs-table | 1307 | 434 | 330 | 151.551 |
| gfx-native | 0 | 1004 | 200 | 52.554 |
| gfx-normal | 1910 | 352 | 77 | 23.755 |
| gfx-table | 1468 | 114 | 33 | 10.786 |

Os que travaram o quadro são os do caminho normal compilados na render thread (build=1): **267 pipelines (186 compute +
81 gráficos), 103,8 s, 186 deles ≥ 100 ms** — exatamente as 186 linhas `SLOW Create*`.

### Cruzamento com o plano da pré-compilação (`plan-join.md`)
`kyty_shader_precompile --game F:\PPSA01342-app --seeds <seeds|released> --dry-run --native-variants --timings`, com
`KYTY_EXEC_DEMAND=1` (o único switch de tradução do `run-windows.json`, como faz o `precompile-windows.ps1`). A identidade
(coluna 6) é a mesma do `key=` do `PIPESRC`.

| Render thread | Pipelines | ms | No plano |
|---|---:|---:|---|
| cs-normal | **104** | **53.772** | seeds, `cs-first` |
| cs-normal | 82 | 43.423 | ausente |
| gfx-normal | 78 | 6.042 | ausente |
| gfx-normal | 3 | 576 | seeds, `gfx-first` |

Em segundo plano: 945 `gfx-native` estão no plano do tier gravado (`gfx-exact-native` 760, `gfx-first-native` 185),
mas a pré-compilação roda sem variantes nativas; 434 `cs-table`, 218 `gfx-normal` build 0, 114 `gfx-table` e 94
`cs-normal` build 0 estão ausentes.

**Por que os 104 não estavam no cache estático:** os logs dos 512 shards de seeds da pré-compilação somam 45.515
pipelines planejados: 24.484 "made by another work item" (estão no cache), **20.562 "skipped: recorded play's
programs"** (o `--covered-by` do modo padrão: `precompile-windows.ps1:318-320`), 0 deixados de fora. Como os 104 estão
no plano e não vieram do cache, eles estão entre os pulados — inferência, não conferida chave por chave (o plano é
binário; um `--dry-run --covered-by` lista os mesmos 107, então o arquivo de timings não mostra o que é pulado).
O próprio script justifica o padrão com a 1.07 ("two 1-1 walks met 1 of 2,227 such pipelines"); na 1.05, com hints
mais fracos, esta sessão encontrou 104 com 53,8 s de travada.

## Conclusão
- O cache quente funcionou: o tutorial já visitado não voltou a travar; as travadas vieram das áreas novas.
- Nas áreas novas, 27% dos pipelines não vinham da pré-compilação, e as compilações na render thread (compute no
  caminho normal) causaram quadros de até 21,5 s.
- **Próximo passo sem código:** `precompile-windows.ps1 -Full`, que inclui as especializações adivinhadas que o padrão
  pula. Deve cobrir os 104 (53,8 s dos 103,8 s travados), não os 82+78 ausentes. Ele também junta a gravação deste PC
  (4.669 entradas) ao tier gravado. Custo: não medido; a fase de seeds levou ~45 min para 45.515 pipelines, e o
  `-Full` acrescenta até 20.562. Atenção (agente, `analise-agentes.md`): qualquer pré-compilação seguinte sem `-Full`
  (inclusive a oferecida pelo `run-windows.ps1 -Prompt`) volta a podar esses pipelines.
- Os ausentes (82 compute + 78 gráficos na render thread) só saem com mudança de código (proposta P1, bind adiado) ou
  se ficarem gravados neste PC (o warmup já gravou a sessão).

## Problemas vistos
- Nenhum crash, nenhum erro no `.err.log`.
- Quadros de 21,5 s e 16,6 s (compilação), percebidos como travadas longas.

## Arquivos anexos
- `session-analysis.md`: análise por minuto, VRAM, PIPESRC e 10 piores quadros (`session_analysis.py` com início e fim).
- `plan-join.md`: o cruzamento com o plano.
- `session-emulator.out.log.gz`: log completo do emulador (copiado depois de o processo sair, mesmo tamanho: 3.162.306 bytes).
- `run.log`: saída do `run-windows.ps1`.
