# Pipelines static=0 da sessao 2 (PIPESRC, sem build=2) contra o plano da pre-compilacao (kyty_shader_precompile --dry-run --native-variants --timings, KYTY_EXEC_DEMAND=1)
| Tipo | Onde compilou | No plano | Pipelines | ms somados |
|---|---|---|---:|---:|
| cs-table | segundo plano | ausente | 434 | 151551 |
| cs-normal | render thread | seeds:cs-first | 104 | 53772 |
| cs-normal | segundo plano | ausente | 94 | 49277 |
| cs-normal | render thread | ausente | 82 | 43423 |
| gfx-native | segundo plano | released:gfx-first-native | 185 | 30791 |
| gfx-normal | segundo plano | ausente | 218 | 16912 |
| gfx-native | segundo plano | released:gfx-exact-native | 760 | 14519 |
| gfx-table | segundo plano | ausente | 114 | 10786 |
| gfx-native | segundo plano | ausente | 59 | 7244 |
| gfx-normal | render thread | ausente | 78 | 6042 |
| gfx-normal | render thread | seeds:gfx-first | 3 | 576 |
