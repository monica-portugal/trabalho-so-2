# Sistemas Operativos 2026/2027 — Trabalho Prático 2

## Simulação de algoritmos de substituição de páginas

## 1. Descrição

Este projeto simula algoritmos de substituição de páginas e tem as seguintes componentes:

- **Aplicações** (`application`): indicam o tempo de execução no CPU, o tempo de bloqueio em
  Entrada/Saída, a prioridade do processo e as páginas virtuais acedidas neste período de
  utilização do CPU.
- **Simulador** (`ossim`): recebe os pedidos das aplicações e simula a sua execução, aplicando
  um algoritmo de escalonamento e um algoritmo de substituição de páginas.

Quando uma aplicação termina, escreve na sua saída o tempo real de execução (`Elapsed`), o
tempo em que esteve a utilizar o CPU (`CPU`) e o tempo em que esteve bloqueada (`BLOCKED`).

Algoritmos de substituição de páginas a simular:

| Algoritmo | Descrição | Estado |
|---|---|---|
| `VM_FIFO` | First-In, First-Out | **Fornecido** |
| `VM_RANDOM` | Substitui uma página escolhida aleatoriamente | A implementar |
| `VM_LRU` | Substitui a página não acedida há mais tempo (`last_accessed` mais antigo) | A implementar |
| `VM_NRU` | Variante prática do LRU: usa um ou mais bits *referenced* para saber há quantos períodos a página não é acedida | A implementar |
| `VM_CLOCK` | Variante do NRU: lista circular, a pesquisa recomeça onde a anterior terminou | A implementar |

Estes algoritmos estão descritos no livro e nos slides das aulas teóricas.

## 2. Código fornecido

- Código base do simulador, com o escalonador **FIFO** (o mesmo do Trabalho 1) e o algoritmo
  de substituição **FIFO**.
- Código base da aplicação (**não alterar**).
- Um ficheiro de input básico (`scenarios/basico.csv`).
- Um script para executar o simulador e a aplicação (`run.sh`; `./run.sh` mostra a ajuda).

O código deve ser compreendido pelos estudantes, analisando a documentação e o código
(`README.md`, `FORMATO_CSV.md`), para perceberem como atuam o escalonador e a substituição
de páginas.

## 3. Trabalho a realizar

1. Acrescentar ao simulador os algoritmos `VM_RANDOM`, `VM_LRU`, `VM_NRU` e `VM_CLOCK`. O
   algoritmo é escolhido por parâmetro: `--vm FIFO|RANDOM|LRU|NRU|CLOCK`.
2. Conceber e criar ficheiros de input que evidenciem o correto funcionamento dos vários
   algoritmos, com 1 ou mais aplicações.
3. Executar a simulação dos vários algoritmos de substituição com o escalonador FIFO e
   1 aplicação.
4. Demonstrar que o simulador executa corretamente o algoritmo escolhido: acrescentar
   `printf`'s adequados ou usar o debugger.
5. *(Opcional)* Acrescentar o escalonador **RR** do Trabalho 1, com um *time-slice* apropriado
   (ex.: 500 ms), escolhido por parâmetro: `--sched FIFO` ou `--sched RR`.
6. *(Opcional)* Executar a simulação dos vários algoritmos de substituição com o escalonador
   RR e 3 aplicações em simultâneo.
7. Produzir um **relatório** que compare as execuções dos vários algoritmos relativamente a:
   - número de faltas de página (*page faults*);
   - número de operações de *swap* (escritas e leituras em disco).

As tarefas 5 e 6 são opcionais.

### Pontos de partida no código

- `virtmem.c` / `virtmem.h`: `eviction_algo_str[]`, `set_eviction_algo()`, `evict()`,
  `fifo_eviction()` (modelo para as restantes). Os totais são impressos em `free_pages()`.
- `virtmem_types.h`: `pte_t` (bits `referenced`, `dirty`, `last_accessed`) e `array_elem_t`
  (`use_bit`, `last_accessed`) para a ordem de evicção.
- `ossim.c`: `parse_args()` e `handle_pages()` (atualiza R e D: leitura → R=1; escrita,
  página negativa no `.csv` → R=1, D=1).
- `scheduler.c`: escalonador FIFO.

Uma página **suja** tem de ser gravada em *swap* quando é expulsa; uma limpa pode ser descartada.

## 4. Execução

```sh
cmake -S . -B build && cmake --build build

# terminal 1
./build/ossim --vpages 30 --pframes 20 --threshold 4 --vm FIFO

# terminal 2
./build/application scenarios/basico.csv

# ou, tudo de uma vez, com logs (ossim-*.log e basic-*.log):
./run.sh --vpages 30 --pframes 20 --threshold 4 --scen scenarios/basico.csv
```

Quando o número de *frames* livres desce abaixo de `--threshold`, começa a substituição.
Formato dos `.csv`: ver `FORMATO_CSV.md`.

## 5. Entrega e avaliação

O trabalho é feito individualmente ou por grupos de 2 estudantes.

A entrega consiste num ficheiro **zip** com o código (código base estendido com as
funcionalidades pedidas e os ficheiros de input usados) e um **relatório** com:

- identificação dos autores;
- identificação do repositório GitHub;
- tabelas com o número de *page faults* e de operações de *swap* (escritas e leituras)
  obtidas em cada simulação;
- comparação do funcionamento dos algoritmos relativamente a estas métricas, com base nas
  simulações efetuadas.

O código deve estar no repositório GitHub dos estudantes; **os professores têm de ser
convidados previamente** como participantes.

## 6. Defesa

Presencial, oral e **individual**, mesmo que o trabalho seja de grupo. O estudante tem de
demonstrar domínio completo do trabalho, explicá-lo e justificar os resultados,
independentemente de ter usado ferramentas de IA. Deve ser capaz de:

- executar o simulador e as aplicações para os vários algoritmos e cenários;
- fazer alterações aos algoritmos pedidas pelo docente na altura;
- explicar e justificar os ficheiros de input entregues;
- demonstrar a correção dos algoritmos, evidenciando os momentos em que são tomadas decisões
  de substituição e que são as decisões corretas;
- justificar as comparações entre os algoritmos.

## 7. Datas

- **Entrega** no Moodle: até às 23h59 de domingo, **8 de novembro de 2026** (hora de Lisboa).
- **Defesa**: aula prática da semana de **9 a 12 de novembro de 2026**.
