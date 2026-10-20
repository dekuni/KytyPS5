frames SLOW: 186, com hora: 186
primeiro 2026-10-20 10:31:43.707000  ultimo 2026-10-20 10:55:53.154000
ms: min 101.4 mediana 200.6 p90 754.8 max 21498.9; soma 136.6 s

### Causa dominante (>= 50% do quadro)
| Causa | Quadros | Tempo total (s) | Pior (ms) |
|---|---:|---:|---:|
| compile | 56 | 101.4 | 21498.9 |
| ociosa (esperando o jogo) | 102 | 28.0 | 935.8 |
| misto/sem causa >50% | 18 | 3.0 | 443.8 |
| readback_wait | 6 | 2.9 | 1068.3 |
| gpu_wait | 2 | 0.9 | 716.2 |
| download_wait | 1 | 0.4 | 354.5 |
| upload_wait | 1 | 0.1 | 132.7 |

Soma de cada componente em todos os SLOW Frame (s): compile 98.4, readback_wait 2.9, gpu_wait 1.1, download_wait 0.5, upload_wait 1.0, record_wait 0.3, idle 25.0, cpu 101.2

### Compilações de pipeline lentas (SLOW *Pipeline)
- AsyncCompute: 330, mediana 353 ms, max 4446 ms
- AsyncGraphics: 233, mediana 210 ms, max 1508 ms
- CreateCompute: 163, mediana 377 ms, max 4861 ms
- CreateGraphics: 23, mediana 144 ms, max 1493 ms

### Origem dos pipelines (PIPESRC: KYTY_PIPELINE_SOURCE_LOG=1)
| Tipo | static=1 (pré-compilação) | static=0 | static=0 >= 100 ms | ms somados static=0 | com blob |
|---|---:|---:|---:|---:|---:|
| cs-normal | 1705 | 455 | 251 | 147660 | 0 |
| cs-table | 1307 | 434 | 330 | 151551 | 0 |
| gfx-native | 0 | 1004 | 200 | 52554 | 0 |
| gfx-normal | 1910 | 352 | 77 | 23755 | 0 |
| gfx-table | 1468 | 114 | 33 | 10786 | 0 |

build (0 full, 1 fast, 2 optimize), unoptimized: 0/0: 6186, 1/0: 2107, 1/1: 228, 2/1: 228

### LHM na sessao: nenhuma amostra (LibreHardwareMonitor sem Log Sensors): temperaturas nao medidas
  (smalldata/1 = GPU Memory Used )
- amostras com Tctl >= 85 C: 0; Tctl >= 90 C: 0

### Por minuto
| Minuto | SLOW Frame | Travado (s) | Causa principal | Pipelines lentos | Tctl média/máx °C | Clock CPU médio MHz | Carga CPU % | GPU °C | Carga GPU % |
|---|---:|---:|---|---:|---|---:|---:|---:|---:|
| 10:30 | 0 | 0.0 | - | 0 | nan/nan | nan | nan | nan | nan |
| 10:31 | 11 | 4.0 | ociosa (esperando o jogo) | 0 | nan/nan | nan | nan | nan | nan |
| 10:32 | 14 | 10.1 | compile | 7 | nan/nan | nan | nan | nan | nan |
| 10:33 | 3 | 0.9 | compile | 5 | nan/nan | nan | nan | nan | nan |
| 10:34 | 4 | 10.6 | compile | 67 | nan/nan | nan | nan | nan | nan |
| 10:35 | 13 | 5.6 | compile | 128 | nan/nan | nan | nan | nan | nan |
| 10:36 | 8 | 6.9 | compile | 74 | nan/nan | nan | nan | nan | nan |
| 10:37 | 19 | 20.7 | compile | 59 | nan/nan | nan | nan | nan | nan |
| 10:38 | 27 | 10.5 | compile | 59 | nan/nan | nan | nan | nan | nan |
| 10:39 | 10 | 5.5 | compile | 33 | nan/nan | nan | nan | nan | nan |
| 10:40 | 16 | 5.2 | compile | 42 | nan/nan | nan | nan | nan | nan |
| 10:41 | 5 | 1.6 | ociosa (esperando o jogo) | 5 | nan/nan | nan | nan | nan | nan |
| 10:42 | 11 | 3.0 | compile | 57 | nan/nan | nan | nan | nan | nan |
| 10:43 | 3 | 3.1 | compile | 64 | nan/nan | nan | nan | nan | nan |
| 10:44 | 0 | 0.0 | - | 0 | nan/nan | nan | nan | nan | nan |
| 10:45 | 3 | 1.4 | ociosa (esperando o jogo) | 14 | nan/nan | nan | nan | nan | nan |
| 10:46 | 6 | 1.3 | ociosa (esperando o jogo) | 4 | nan/nan | nan | nan | nan | nan |
| 10:47 | 0 | 0.0 | - | 0 | nan/nan | nan | nan | nan | nan |
| 10:48 | 0 | 0.0 | - | 0 | nan/nan | nan | nan | nan | nan |
| 10:49 | 0 | 0.0 | - | 0 | nan/nan | nan | nan | nan | nan |
| 10:50 | 0 | 0.0 | - | 0 | nan/nan | nan | nan | nan | nan |
| 10:51 | 6 | 2.2 | ociosa (esperando o jogo) | 27 | nan/nan | nan | nan | nan | nan |
| 10:52 | 5 | 1.4 | ociosa (esperando o jogo) | 6 | nan/nan | nan | nan | nan | nan |
| 10:53 | 6 | 1.8 | ociosa (esperando o jogo) | 11 | nan/nan | nan | nan | nan | nan |
| 10:54 | 8 | 11.9 | compile | 33 | nan/nan | nan | nan | nan | nan |
| 10:55 | 8 | 28.9 | compile | 43 | nan/nan | nan | nan | nan | nan |

### Memoria de video (amostras do emulador)
| Hora (aprox.) | flip | Em uso MiB | Budget MiB | Tipo "plain" na VRAM MiB | Memoria de sistema da GPU MiB |
|---|---:|---:|---:|---:|---:|
| antes do 1o SLOW | 0 | 990 | 7256 | 0 | 716 |
| 10:31:47 | 60 | 2713 | 7256 | 0 | 752 |
| 10:32:05 | 1140 | 3183 | 7256 | 0 | 752 |
| 10:32:05 | 2040 | 3503 | 7256 | 0 | 752 |
| 10:32:23 | 2400 | 3778 | 7256 | 0 | 752 |
| 10:32:38 | 2580 | 4455 | 7256 | 498 | 758 |
| 10:32:41 | 2700 | 5309 | 7055 | 562 | 758 |
| 10:32:50 | 3000 | 5660 | 7055 | 626 | 774 |
| 10:33:03 | 3300 | 5954 | 7055 | 1392 | 800 |
| 10:33:04 | 3420 | 5686 | 7055 | 1456 | 800 |
| 10:33:33 | 4140 | 6254 | 7055 | 1648 | 832 |
| 10:33:33 | 5100 | 5985 | 7084 | 1712 | 1145 |
| 10:34:12 | 5760 | 6264 | 7091 | 1712 | 896 |
| 10:34:25 | 5820 | 5923 | 7088 | 1712 | 896 |
| 10:34:25 | 6420 | 5587 | 7086 | 1584 | 896 |
| 10:34:25 | 7020 | 5182 | 7256 | 837 | 896 |
| 10:34:25 | 7560 | 5466 | 7086 | 965 | 896 |
| 10:35:07 | 8040 | 5977 | 7086 | 1281 | 900 |
| 10:35:07 | 8460 | 6239 | 7086 | 1409 | 900 |
| 10:35:35 | 8880 | 5877 | 7086 | 1473 | 964 |
| 10:35:47 | 9060 | 6135 | 7086 | 1665 | 964 |
| 10:35:52 | 9180 | 6519 | 7130 | 2049 | 964 |
| 10:35:53 | 9360 | 6839 | 7112 | 2369 | 964 |
| 10:36:11 | 9720 | 7601 | 7109 | 2625 | 964 |
| 10:36:32 | 10320 | 7177 | 7108 | 2817 | 1220 |
| 10:37:15 | 11220 | 7532 | 7113 | 2561 | 1028 |
| 10:37:20 | 11280 | 7981 | 7080 | 2945 | 1028 |
| 10:38:10 | 12480 | 8248 | 7186 | 3127 | 1038 |
| 10:38:21 | 12660 | 7978 | 7142 | 2935 | 1038 |
| 10:43:11 | 20220 | 7668 | 7142 | 2615 | 1166 |
| 10:55:37 | 43620 | 7392 | 7127 | 2229 | 1168 |
| 10:55:37 | 43740 | 7776 | 7127 | 2613 | 1168 |
| 10:55:37 | 43800 | 8043 | 7127 | 2549 | 1168 |
| 10:55:37 | 43860 | 7711 | 7127 | 2549 | 1168 |

acima do budget: 11 de 34

### Prefetch
2026-10-20 10:33:04.794000 Shader prefetch: all 20374 programs done, 17752 from the translation store (0 rejected), 1225 MiB of SPIR-V translated (6333 MiB in the scratch file)

### 10 piores quadros
- 10:55:15 21499 ms: idle=16 cpu=20676 compile=21424 upload_wait=3 record_wait=1
- 10:37:11 16578 ms: idle=8 cpu=15995 compile=16502 readback_wait=0 upload_wait=1 record_wait=3
- 10:34:25 9816 ms: idle=13 cpu=9449 compile=9729 upload_wait=4 record_wait=3
- 10:54:36 9638 ms: idle=6 cpu=9346 compile=9599 upload_wait=1 record_wait=0
- 10:32:38 8061 ms: idle=17 cpu=7570 compile=7710 gpu_wait=268 upload_wait=33 record_wait=9
- 10:55:34 6155 ms: idle=9 cpu=6002 compile=6085 download_wait=0 upload_wait=4 record_wait=4
- 10:38:21 5224 ms: idle=4 cpu=5013 compile=5190 upload_wait=2 record_wait=0
- 10:36:50 3971 ms: idle=7 cpu=3887 compile=3947 upload_wait=2 record_wait=0
- 10:35:06 2510 ms: idle=4 cpu=2294 compile=2306 upload_wait=16 record_wait=19
- 10:43:11 1571 ms: idle=10 cpu=1558 compile=1536 upload_wait=1 record_wait=0
