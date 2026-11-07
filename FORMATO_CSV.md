# Formato dos ficheiros de *bursts* (`.csv`)

Este documento é **igual nos Trabalhos 1, 2 e 3**. O programa `application` lê um destes
ficheiros e, linha a linha, pede ao simulador (`ossim`) que execute cada *burst*.

## Estrutura

Uma linha por *burst*. Campos separados por vírgula. Linhas em branco e linhas começadas
por `#` são ignoradas (comentários).

```
tempo_cpu_ms , tempo_bloqueio_ms , nice , [lista_de_paginas]
```

Os campos são posicionais e **da esquerda para a direita**: para usar o campo `nice` é
preciso preencher também `tempo_bloqueio_ms`; para usar a lista de páginas é preciso
preencher os três anteriores.

## Campos

| # | Campo | Tipo | Obrigatório? | Significado |
|---|---|---|---|---|
| 1 | `tempo_cpu_ms` | inteiro ≥ 0 | **Sim (sempre)** | Tempo, em milissegundos, que a aplicação quer executar no CPU neste *burst*. |
| 2 | `tempo_bloqueio_ms` | inteiro ≥ 0 | Não (assume `0`) | Tempo, em milissegundos, que a aplicação fica bloqueada (E/S) **depois** do *burst* de CPU. `0` = sem bloqueio. |
| 3 | `nice` | inteiro | Não (assume `0`) | Prioridade da aplicação. |
| 4 | `[lista_de_paginas]` | lista entre `[ ]` | Não (assume lista vazia) | Páginas virtuais acedidas neste *burst*. Numeradas **a partir de 1**. Um número **negativo** = escrita (página fica *dirty*); **positivo** = leitura. |

Se uma linha só tiver o primeiro campo, os restantes assumem os valores por omissão. Por
isso **o mesmo ficheiro `application.c` serve os três trabalhos** — só muda quais as
colunas que têm efeito.

## Que colunas interessam em cada trabalho

| Trabalho | Obrigatória | Também usadas | Ignoradas |
|---|---|---|---|
| **1 — Escalonamento** | `tempo_cpu_ms` | `tempo_bloqueio_ms` (cenários com E/S), `nice` (apenas o **MLFQ**) | `[lista_de_paginas]` |
| **2 — Paginação de memória** | `tempo_cpu_ms` | `tempo_bloqueio_ms`, `[lista_de_paginas]` | `nice` |
| **3 — Simulador concorrente** | `tempo_cpu_ms` | `tempo_bloqueio_ms`, `[lista_de_paginas]` | `nice` |

## Exemplos

### Trabalho 1 (só CPU)

```
# aplicacao A: um unico burst de 10 s de CPU
10000,0,0
```

### Trabalho 1 (CPU + E/S, para o MLFQ)

```
# alterna 300 ms de CPU com 800 ms bloqueada, 4 vezes
300,800,0
300,800,0
300,800,0
300,800,0
```

### Trabalhos 2 e 3 (com páginas)

```
# 200 ms de CPU, 2 ms bloqueada; le as paginas 1, 3, 5 e escreve nas paginas 2 e 4
200,2,0,[1,-2,3,-4,5]
# segundo burst: 200 ms de CPU, sem bloqueio; le 1 e 2, escreve na 6
200,0,0,[1,2,-6]
```

Interpretação da lista `[1,-2,3,-4,5]`:

| Valor | Página | Acesso |
|---|---|---|
| `1` | 1 | leitura |
| `-2` | 2 | escrita |
| `3` | 3 | leitura |
| `-4` | 4 | escrita |
| `5` | 5 | leitura |

## Como usar

Com o simulador já a correr noutro terminal:

```sh
./build/application caminho/para/ficheiro.csv          # uma aplicação
./run.sh --scen f1.csv --scen f2.csv                    # simulador + várias aplicações, com logs
```
