#!/bin/sh
# Corre o ossim e a(s) application, cada um com o output para um .log.
#
#   ./run.sh [argumentos do ossim] --scen ficheiro.csv [--scen ficheiro2.csv ...]
#
# Todos os argumentos que nao sao --scen sao passados ao ossim (ver ./build/ossim --help).
# Cria ossim-<datetime>.log e basic-<datetime>.log na pasta atual.
# BUILD_DIR (por omissao: build ou cmake-build-debug) indica onde estao os binarios.

usage() {
    cat <<USAGE
Uso: $0 [argumentos do ossim] --scen ficheiro.csv [--scen ficheiro2.csv ...]

Arranca o ossim, corre uma application por cada --scen (em simultaneo), termina o
ossim no fim e guarda o output em:
  ossim-<data-hora>.log   output do simulador (inclui a linha "stats")
  basic-<data-hora>.log   output da(s) aplicacao(oes)

Opcoes:
  --scen FICHEIRO     perfil .csv de uma aplicacao (obrigatorio; pode repetir)
  --vpages N           paginas virtuais por processo       (omissao: 16)
  --pframes N          frames fisicas                      (omissao: 10)
  --threshold N       frames livres minimas               (omissao: 3)
  --vm ALGO           FIFO | RANDOM | LRU | NRU | CLOCK   (omissao: FIFO)
  --sched ALGO        FIFO | RR                           (omissao: FIFO)
  -h, --help          mostra esta ajuda

Qualquer outro argumento e passado tal e qual ao ossim.
Variavel BUILD_DIR: pasta dos binarios (omissao: build ou cmake-build-debug).

Exemplos:
  $0 --vpages 30 --pframes 20 --threshold 4 --scen scenarios/basico.csv
  $0 --vpages 30 --pframes 20 --threshold 4 --vm LRU --scen scenarios/basico.csv
  $0 --vpages 30 --pframes 20 --threshold 4 --scen scenarios/basico.csv
USAGE
}

[ $# -eq 0 ] && { usage; exit 0; }

# Separa --scen FICHEIRO (guardados em SCENS) dos restantes argumentos (para o ossim).
SCENS=""
n=$#
while [ $n -gt 0 ]; do
    a="$1"; shift; n=$((n - 1))
    case "$a" in
        -h|--help) usage; exit 0 ;;
        --scen)
            [ $n -gt 0 ] || { echo "Erro: --scen precisa de um ficheiro." >&2; usage >&2; exit 1; }
            SCENS="$SCENS $1"; shift; n=$((n - 1)) ;;
        *) set -- "$@" "$a" ;;   # fica para o ossim (rodado para o fim da lista)
    esac
done
# Os argumentos do ossim ficaram em "$@".
[ -n "$SCENS" ] || { echo "Erro: indique pelo menos um --scen ficheiro.csv." >&2; usage >&2; exit 1; }
for f in $SCENS; do
    [ -f "$f" ] || { echo "Erro: ficheiro nao encontrado: $f" >&2; exit 1; }
done

if [ -z "$BUILD_DIR" ]; then
    for d in build cmake-build-debug; do
        [ -x "$d/ossim" ] && [ -x "$d/application" ] && BUILD_DIR="$d" && break
    done
fi
if [ -z "$BUILD_DIR" ] || [ ! -x "$BUILD_DIR/ossim" ] || [ ! -x "$BUILD_DIR/application" ]; then
    echo "Nao encontrei ossim e application compilados (build/ ou cmake-build-debug/)." >&2
    echo "Compile os dois alvos primeiro (ver README.md)." >&2
    exit 1
fi

STAMP=$(date +%Y%m%d-%H%M%S)
OSSIM_LOG="ossim-$STAMP.log"
APP_LOG="basic-$STAMP.log"

"$BUILD_DIR/ossim" "$@" > "$OSSIM_LOG" 2>&1 &
OSSIM_PID=$!

# Espera que o ossim crie o socket (ou falhe a arrancar).
i=0
while [ ! -S /tmp/scheduler.sock ]; do
    if ! kill -0 "$OSSIM_PID" 2>/dev/null; then
        echo "O ossim terminou ao arrancar; ver $OSSIM_LOG" >&2
        cat "$OSSIM_LOG" >&2
        exit 1
    fi
    i=$((i + 1))
    [ $i -gt 50 ] && { echo "Timeout a esperar pelo ossim" >&2; kill "$OSSIM_PID"; exit 1; }
    sleep 0.1
done

echo "ossim:       $BUILD_DIR/ossim $*  ->  $OSSIM_LOG"
echo "application:$SCENS  ->  $APP_LOG"

# Uma application por ficheiro, em simultaneo; todas escrevem em $APP_LOG.
for f in $SCENS; do
    "$BUILD_DIR/application" "$f" >> "$APP_LOG" 2>&1 &
done
wait $(jobs -p | grep -v "^$OSSIM_PID$") 2>/dev/null

# Pequena pausa para o ossim imprimir os totais e termina-o (Ctrl-C).
sleep 1
kill -INT "$OSSIM_PID" 2>/dev/null
wait "$OSSIM_PID" 2>/dev/null

echo "Concluido."
grep -a "stats" "$OSSIM_LOG"
