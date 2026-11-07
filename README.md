# OSSIM — Simulador de Paginação de Memória

Sistemas Operativos 2026/2027 — Universidade Lusófona.

Simulador de um sistema operativo simplificado. Dois programas comunicam por
*socket* UNIX:

- **`application`** — processo de utilizador. Lê um perfil de *bursts* (tempo de
  CPU, E/S e páginas acedidas) de um ficheiro `.csv` e envia pedidos `RUN` /
  `BLOCK` ao simulador.
- **`ossim`** — o "SO": servidor de *sockets*, filas de processos, escalonador e
  gestão de memória virtual com paginação e *swap*.

O enunciado está em **[`enunciado.md`](enunciado.md)** e o formato dos ficheiros
`.csv` em **[`FORMATO_CSV.md`](FORMATO_CSV.md)**.

## Páginas e frames

- **Página (virtual)** — bloco de memória do ponto de vista do processo. O espaço
  de endereços de cada processo é dividido em páginas, numeradas a partir de ****.
  São os números que aparecem nos `.csv`. O argumento `--vpages` define quantas
  páginas tem cada processo.
- **Frame (física)** — bloco da RAM real, do mesmo tamanho que uma página. O
  argumento `--pframes` define quantas frames a RAM simulada tem.

Uma página só pode ser usada se estiver numa frame. A tabela de páginas de cada
processo guarda em que frame está cada página. Se não estiver em nenhuma, dá-se
uma **falta de página** (*page fault*): a página é trazida do *swap* (disco) ou
alocada pela primeira vez. Quando já não há frames livres, o **algoritmo de
substituição** (`--vm`) escolhe uma frame ocupada e expulsa a página que lá está.
Se essa página estiver **suja** (foi escrita), é gravada em *swap* (*swap-out*);
se estiver limpa, é simplesmente descartada. Trazer uma página de volta do disco
é um *swap-in*.

Normalmente há mais páginas do que frames (por omissão, 16 páginas para 10
frames). É por isso que é preciso substituir páginas: se `--pframes` fosse igual ou
maior que `--vpages`, nunca haveria substituições.

No `.csv`, uma página positiva (`3`) é uma **leitura** e uma negativa (`-3`) é uma
**escrita** na página 3 (fica suja).

## Compilar e correr no CLion com o Docker DOLOS

O código compila e corre dentro do contentor **DOLOS** (Linux), ao qual o CLion se
liga por SSH (por omissão `aluno@localhost`, porta `2222`).

### 1. Configurar o CLion (uma vez)

1. Ter o contentor DOLOS a correr.
2. **Settings → Build, Execution, Deployment → Toolchains**: acrescentar uma
   *toolchain* **Remote Host** (SSH) para o DOLOS e escolhê-la como predefinida.
3. **Settings → Build, Execution, Deployment → Deployment**, selecionar o servidor
   SFTP do DOLOS:
   - separador **Connection** → *Root path*: `/home/aluno` (botão *Autodetect*);
   - separador **Mappings** → *Deployment path*: `trabalho2`, ou seja
     `/home/aluno/trabalho2`.
4. **Settings → Build, Execution, Deployment → CMake**, no perfil que usa a
   *toolchain* remota, pôr *Build directory* = `build` (caminho **relativo**, não
   absoluto nem em `/tmp`).

Resultado: fontes em `/home/aluno/trabalho2` e binários em
`/home/aluno/trabalho2/build`.

### 2. Compilar

**É preciso compilar os dois programas, `ossim` e `application`, antes de
executar.** São dois alvos separados: se compilar só um, o outro não existe em
`build/` e a execução falha. No CLion, fazer *Build* de cada alvo (selecionar o
alvo na lista de configurações e *Build*) ou usar *Build → Build All*. Ou, num
terminal do DOLOS (por exemplo `ssh -p 2222 aluno@localhost`), o comando abaixo
compila ambos:

```sh
cd /home/aluno/trabalho2
cmake -S . -B build && cmake --build build
```

### 3. Executar (precisa de dois terminais no DOLOS)

Confirme que `build/ossim` e `build/application` existem (passo 2). O simulador
tem de estar a correr **antes** da aplicação.

**Terminal 1** — o simulador:

```sh
cd /home/aluno/trabalho2
./build/ossim --vm FIFO
```

**Terminal 2** — a aplicação (uma por perfil `.csv`):

```sh
cd /home/aluno/trabalho2
./build/application scenarios/basico.csv
```

Para várias aplicações em simultâneo, lance cada uma em segundo plano (`&`) e
espere por todas:

```sh
./build/application scenarios/basico.csv &
./build/application scenarios/basico.csv &
wait
```

Para correr o simulador e as aplicações de uma só vez, com logs, use o `run.sh`
(ver abaixo).

(No CLion pode abrir dois terminais em *Terminal* → *+*, ou criar duas
configurações de execução, `ossim` e `application`, esta última com o
`scenarios/basico.csv` como argumento.)

`Ctrl-C` no terminal 1 termina o simulador. Cada `application` imprime `Elapsed`,
`CPU` e `BLOCKED` (segundos) quando termina, e o `ossim` imprime os totais:

```
FIFO stats: accesses=37 faults=30 swap-in=14 swap-out=22 fault-rate=0.811
```

## Argumentos do `ossim`

| Argumento | Valor por omissão | Significado |
|---|---|---|
| `--vpages N` | `16` | Nº de páginas virtuais de cada processo (≥ 1). As páginas dos `.csv` têm de estar entre 1 e `N`. |
| `--pframes N` | `10` | Nº de frames físicas da RAM simulada (≥ 1). |
| `--threshold N` | `3` | Nº mínimo de frames livres a manter (≥ 0). Quando o nº de frames livres desce abaixo de `N`, antes do pedido seguinte começa a substituição de páginas e continua até voltar a haver `N` livres. |
| `--vm ALGO` | `FIFO` | Algoritmo de substituição de páginas: `FIFO`, `RANDOM`, `LRU`, `NRU` ou `CLOCK`. No ponto de partida só o `FIFO` está implementado; os restantes são o trabalho a fazer. |
| `--sched ALGO` | `FIFO` | Algoritmo de escalonamento: `FIFO`. O `RR` é a tarefa opcional 5. |
| `--help` | — | Mostra a utilização e termina. |

Exemplos:

```sh
./build/ossim                                          # tudo por omissão
./build/ossim --vm FIFO --pframes 8 --threshold 2
./build/ossim --vpages 30 --pframes 20 --threshold 4     # para scenarios/basico.csv
```

Com `--vpages 30 --pframes 20 --threshold 4` e o `scenarios/basico.csv`, a primeira
substituição acontece no 3.º burst, quando passa a haver mais de 16 páginas em memória.

### Script `run.sh`: correr tudo de uma vez

O `run.sh` arranca o `ossim`, corre a(s) `application`, termina o simulador no fim e
guarda o output em ficheiros de log (na pasta atual):

- `ossim-<data-hora>.log` — output do simulador (inclui a linha `stats`);
- `basic-<data-hora>.log` — output da(s) aplicação(ões).

Qualquer argumento do `ossim` (ver a tabela abaixo) pode ser passado ao script; cada
`--scen` indica o `.csv` de uma aplicação (pode repetir). Sem argumentos (ou com
`--help`) mostra a ajuda.

```sh
./run.sh                                                                  # mostra a ajuda
./run.sh --vpages 30 --pframes 20 --threshold 4 --scen scenarios/basico.csv
./run.sh --vpages 30 --pframes 20 --threshold 4 --vm LRU --scen scenarios/basico.csv
./run.sh --vpages 30 --pframes 20 --threshold 4 --scen scenarios/basico.csv --scen scenarios/basico.csv   # várias aplicações
```

Procura os binários em `build/` e depois em `cmake-build-debug/`; para usar outra
pasta, `BUILD_DIR=minha-pasta ./run.sh ...`. Os dois programas têm de estar
compilados (ver acima). No fim, o script mostra a linha `stats` no terminal.

### Cenários fornecidos

| Ficheiro | Argumentos | Descrição |
|---|---|---|
| `scenarios/basico.csv` | `--vpages 30 --pframes 20 --threshold 4` | 11 bursts, páginas 1–30, em várias fases (carga inicial, substituição, localidade, varrimento). |

Pode criar os seus próprios (ver `FORMATO_CSV.md`).

### Exemplo: cenário `basico.csv`

O `basico.csv` usa as páginas 1 a 30, por isso o simulador tem de ser lançado com
`--vpages 30` (com os valores por omissão, `--vpages 16`, os pedidos às páginas 17–30
são inválidos e dão `ERROR: invalid virtual page`, o que falseia os totais).

**Terminal 1** — simulador (a pasta de `build` pode chamar-se `build` ou
`cmake-build-debug`, conforme a configuração do CLion):

```sh
./build/ossim --vpages 30 --pframes 20 --threshold 4 --vm FIFO
```

```
OSSIM: scheduler=FIFO, eviction=FIFO, vpages=30, pframes=20, threshold=4
Scheduler server listening on /tmp/scheduler.sock...
[/home/aluno/trabalho2/queue.c:213] [Scheduler] New client connected: fd=4
[/home/aluno/trabalho2/queue.c:232] Connection closed by client (fd=4)
Process 529 terminated: freed ... pframes from RAM, ... vpages from swap
FIFO stats: accesses=66 faults=52 swap-in=22 swap-out=36 fault-rate=0.788
```

**Terminal 2** — aplicação (depois de o simulador estar a correr):

```sh
./build/application scenarios/basico.csv
```

```
Enqueued burst: pid=529, time=300 ms, block=50 ms, nice=0, vpns=[1, -2, 3, -4, 5, 6]
Enqueued burst: pid=529, time=300 ms, block=50 ms, nice=0, vpns=[7, -8, 9, 10, -11, 12]
...
Enqueued burst: pid=529, time=200 ms, block=0 ms, nice=0, vpns=[30, 29, -1, 2, 28, -3]
[application.c:55] Application basico (PID 529) sent RUN request for 300 ms
[application.c:69] Received ACK from scheduler for application basico (PID 529) at time 36058 ms
[application.c:55] Application basico (PID 529) sent BLOCK request for 50 ms
...
Application basico (PID 529) finished at time 40522 ms, Elapsed: 4.464 seconds, CPU: 3.200 seconds, BLOCKED: 0.500 seconds
```

Os tempos absolutos (`at time ...`) e o PID variam de execução para execução (o
relógio do simulador começa quando o `ossim` arranca). O que deve ser estável é o
`CPU` (3,2 s: 10 bursts de 300 ms mais um de 200 ms), o `BLOCKED` (0,5 s) e, para
o mesmo algoritmo `--vm`, os totais da linha `stats`. Com o FIFO e estes
argumentos: 66 acessos, 52 faltas de página, 22 *swap-in* e 36 *swap-out*.

## Entrega

Gerar o `entrega.zip` (fontes, `CMakeLists.txt`, scripts e cenários) e submeter esse
ficheiro no Moodle, juntamente com o relatório:

```sh
cmake -S . -B build
cmake --build build --target archive     # cria build/entrega.zip
```

O zip fica na pasta de *build* (`build/entrega.zip`, ou `cmake-build-debug/entrega.zip`
se for essa a pasta que o CLion usa). Como o comando corre no DOLOS, o ficheiro fica
lá, não no seu computador. Para o trazer para o seu computador, a partir de um
terminal local:

```sh
scp -P 2222 aluno@localhost:trabalho2/build/entrega.zip .
```

Também pode descarregá-lo pelo CLion: **Tools → Deployment → Browse Remote Host**,
abrir `trabalho2/build`, botão direito em `entrega.zip` → *Download to...*.

Confira o conteúdo antes de submeter: `unzip -l entrega.zip`.

## Protocolo `application` ⇄ `ossim`

```
Simulador                           Aplicações
   |                                    |
   | <---- App1 RUN (tempo, páginas) -- |
   | ----- App1 ACK (relógio) --------> |
   | <---- App2 RUN (tempo, páginas) -- |
   | ----- App2 ACK (relógio) --------> |
   | ----- App1 DONE (relógio) -------> |
   | <---- App1 BLOCK (tempo) --------- |
   | ----- App1 ACK (relógio) --------> |
   | ----- App2 DONE (relógio) -------> |
   | <---- App2 BLOCK (tempo) --------- |
   | ----- App2 ACK (relógio) --------> |
```

A mensagem `RUN` transporta a lista de páginas virtuais acedidas no *burst*
(ver `FORMATO_CSV.md`).
