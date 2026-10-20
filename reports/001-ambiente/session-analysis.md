frames SLOW: 219, com hora: 219
primeiro 2026-10-19 23:10:29.680000  ultimo 2026-10-19 23:34:09.582000
ms: min 100.2 mediana 147.8 p90 486.1 max 12952.5; soma 77.5 s

### Causa dominante (>= 50% do quadro)
| Causa | Quadros | Tempo total (s) | Pior (ms) |
|---|---:|---:|---:|
| compile | 75 | 47.3 | 12952.5 |
| ociosa (esperando o jogo) | 109 | 21.0 | 1647.6 |
| misto/sem causa >50% | 26 | 7.5 | 3679.6 |
| readback_wait | 3 | 0.8 | 572.0 |
| download_wait | 4 | 0.6 | 174.9 |
| upload_wait | 1 | 0.2 | 249.5 |
| gpu_wait | 1 | 0.1 | 134.5 |

Soma de cada componente em todos os SLOW Frame (s): compile 43.9, readback_wait 1.4, gpu_wait 0.6, download_wait 0.7, upload_wait 1.0, record_wait 0.3, idle 19.5, cpu 48.9

### Compilações de pipeline lentas (SLOW *Pipeline)
- AsyncCompute: 277, mediana 308 ms, max 16790 ms
- AsyncGraphics: 394, mediana 185 ms, max 860 ms
- CreateCompute: 81, mediana 305 ms, max 4385 ms
- CreateGraphics: 62, mediana 170 ms, max 768 ms

### LHM na sessao: 1521 amostras de 23:09:17 a 23:34:39
- tctl: media 70.4, max 78.9
- clk: media 4428.8, max 4650.0
- cpuload: media 26.4, max 97.3
- pkg: media 59.8, max 77.9
- gpu_t: media 76.9, max 85.0
- gpu_hot: media 97.6, max 108.2
- gpu_load: media 70.2, max 100.0
- gpu_clk: media 1728.1, max 2010.0
- vram_used: media 6415.2, max 7810.0
  (smalldata/1 = GPU Memory Used )
- amostras com Tctl >= 85 C: 0; Tctl >= 90 C: 0

### Por minuto
| Minuto | SLOW Frame | Travado (s) | Causa principal | Pipelines lentos | Tctl média/máx °C | Clock CPU médio MHz | Carga CPU % | GPU °C | Carga GPU % |
|---|---:|---:|---|---:|---|---:|---:|---:|---:|
| 23:09 | 0 | 0.0 | - | 0 | 66/72 | 4455 | 22 | 54 | 24 |
| 23:10 | 12 | 3.7 | ociosa (esperando o jogo) | 13 | 72/74 | 4511 | 35 | 68 | 27 |
| 23:11 | 1 | 0.8 | ociosa (esperando o jogo) | 0 | 72/74 | 4480 | 30 | 73 | 40 |
| 23:12 | 0 | 0.0 | - | 0 | 74/76 | 4471 | 30 | 70 | 39 |
| 23:13 | 4 | 2.4 | ociosa (esperando o jogo) | 12 | 73/76 | 4480 | 30 | 74 | 42 |
| 23:14 | 14 | 8.6 | compile | 42 | 73/75 | 4427 | 25 | 80 | 86 |
| 23:15 | 6 | 1.3 | compile | 3 | 67/74 | 4372 | 16 | 74 | 61 |
| 23:16 | 0 | 0.0 | - | 0 | 61/64 | 4407 | 13 | 67 | 39 |
| 23:17 | 0 | 0.0 | - | 0 | 60/62 | 4395 | 13 | 66 | 40 |
| 23:18 | 3 | 0.8 | compile | 6 | 68/74 | 4442 | 24 | 76 | 76 |
| 23:19 | 10 | 2.1 | compile | 83 | 73/75 | 4433 | 29 | 81 | 85 |
| 23:20 | 11 | 3.4 | compile | 73 | 71/75 | 4425 | 25 | 81 | 81 |
| 23:21 | 7 | 1.4 | compile | 48 | 69/72 | 4329 | 26 | 81 | 85 |
| 23:22 | 15 | 2.2 | compile | 30 | 70/73 | 4487 | 24 | 82 | 88 |
| 23:23 | 28 | 4.4 | ociosa (esperando o jogo) | 42 | 72/78 | 4384 | 27 | 82 | 88 |
| 23:24 | 11 | 17.0 | compile | 157 | 72/78 | 4345 | 25 | 73 | 63 |
| 23:25 | 5 | 8.2 | compile | 36 | 69/77 | 4371 | 25 | 80 | 87 |
| 23:26 | 1 | 0.1 | misto/sem causa >50% | 13 | 67/69 | 4400 | 23 | 82 | 90 |
| 23:27 | 2 | 0.9 | compile | 14 | 69/72 | 4406 | 24 | 82 | 85 |
| 23:28 | 5 | 0.7 | compile | 44 | 68/71 | 4403 | 24 | 82 | 85 |
| 23:29 | 4 | 1.1 | compile | 24 | 73/75 | 4431 | 29 | 82 | 87 |
| 23:30 | 8 | 1.3 | compile | 79 | 74/79 | 4445 | 36 | 82 | 89 |
| 23:31 | 4 | 1.7 | compile | 75 | 75/79 | 4440 | 31 | 81 | 84 |
| 23:32 | 6 | 1.2 | readback_wait | 5 | 69/75 | 4376 | 24 | 81 | 85 |
| 23:33 | 4 | 2.2 | ociosa (esperando o jogo) | 1 | 75/77 | 4524 | 39 | 82 | 79 |
| 23:34 | 58 | 11.9 | ociosa (esperando o jogo) | 1 | 74/79 | 4426 | 27 | 69 | 62 |

### Memoria de video (amostras do emulador)
| Hora (aprox.) | flip | Em uso MiB | Budget MiB | Tipo "plain" na VRAM MiB | Memoria de sistema da GPU MiB |
|---|---:|---:|---:|---:|---:|
| antes do 1o SLOW | 0 | 956 | 7256 | 0 | 703 |
| 23:10:32 | 60 | 2599 | 7256 | 0 | 746 |
| 23:10:32 | 840 | 2977 | 7256 | 0 | 746 |
| 23:10:58 | 1560 | 3327 | 7256 | 0 | 746 |
| 23:11:16 | 3060 | 3649 | 7256 | 0 | 746 |
| 23:13:51 | 11820 | 5646 | 7068 | 93 | 794 |
| 23:14:24 | 13020 | 5928 | 7082 | 1094 | 1074 |
| 23:14:32 | 13260 | 6228 | 7082 | 1606 | 1393 |
| 23:14:32 | 13860 | 5772 | 7096 | 1222 | 1393 |
| 23:15:25 | 15120 | 6660 | 7122 | 1190 | 1330 |
| 23:15:26 | 15240 | 5498 | 7122 | 1126 | 1456 |
| 23:15:26 | 15780 | 4666 | 7256 | 378 | 1456 |
| 23:18:18 | 25500 | 5255 | 7256 | 806 | 1456 |
| 23:18:20 | 25620 | 5863 | 7082 | 806 | 1456 |
| 23:18:37 | 26760 | 5597 | 7082 | 955 | 1520 |
| 23:19:14 | 28020 | 5911 | 7082 | 1083 | 1520 |
| 23:19:24 | 28680 | 6214 | 7082 | 1787 | 1855 |
| 23:19:35 | 28860 | 6534 | 7122 | 2105 | 1857 |
| 23:19:35 | 29160 | 6790 | 7122 | 2361 | 1857 |
| 23:19:51 | 30060 | 7051 | 7122 | 2617 | 1857 |
| 23:22:34 | 34560 | 7571 | 7122 | 3065 | 1857 |
| 23:22:37 | 35340 | 7005 | 7122 | 2617 | 1857 |
| 23:23:03 | 35580 | 7477 | 7122 | 3065 | 1857 |
| 23:24:05 | 37440 | 7133 | 7122 | 2681 | 1857 |
| 23:24:05 | 37560 | 6718 | 7122 | 2617 | 1857 |
| 23:24:32 | 37680 | 7063 | 7122 | 2485 | 1861 |
| 23:28:48 | 47340 | 7425 | 7116 | 2891 | 1861 |
| 23:30:55 | 51540 | 5539 | 7256 | 1022 | 1863 |
| 23:30:55 | 52080 | 5025 | 7256 | 723 | 1863 |
| 23:30:55 | 52320 | 5345 | 6976 | 723 | 1863 |
| 23:31:31 | 53580 | 5702 | 6990 | 979 | 1863 |
| 23:31:39 | 54660 | 6148 | 6990 | 1619 | 1863 |
| 23:32:32 | 55500 | 6818 | 7109 | 1939 | 1863 |
| 23:32:53 | 56160 | 7164 | 7088 | 1875 | 1800 |
| 23:32:53 | 56280 | 6813 | 7086 | 1939 | 1800 |
| 23:33:54 | 58500 | 7939 | 7028 | 1939 | 1546 |
| 23:34:05 | 58560 | 8250 | 7054 | 2124 | 1546 |

acima do budget: 7 de 37

### Prefetch
2026-10-19 23:11:16.207000 Shader prefetch: all 20374 programs done, 0 from the translation store (0 rejected), 5001 MiB of SPIR-V translated (5054 MiB in the scratch file)

### 10 piores quadros
- 23:24:28 12952 ms: idle=17 cpu=12624 compile=12854 upload_wait=15 record_wait=4
- 23:25:01 7081 ms: idle=6 cpu=6914 compile=7050 upload_wait=1 record_wait=0
- 23:14:02 4754 ms: idle=548 cpu=3968 compile=3952 gpu_wait=196 upload_wait=23 record_wait=6
- 23:34:01 3680 ms: idle=0 cpu=1921 upload_wait=10
- 23:33:54 1648 ms: idle=1216 cpu=46 upload_wait=16
- 23:24:30 1336 ms: idle=0 cpu=1138 compile=1201 readback_wait=55 download_wait=55 upload_wait=7 record_wait=4
- 23:13:51 1063 ms: idle=601 cpu=391 compile=426 upload_wait=73 record_wait=57
- 23:24:31 989 ms: idle=55 cpu=866 compile=878 upload_wait=3 record_wait=2
- 23:31:31 857 ms: idle=11 cpu=781 compile=759 upload_wait=2 record_wait=3
- 23:13:56 838 ms: idle=822 cpu=16 compile=6 upload_wait=2 record_wait=1
