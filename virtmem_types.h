#ifndef VIRTMEM_TYPES_H
#define VIRTMEM_TYPES_H

#include <stdint.h>
#include "uthash.h"

#define INVALID_PFN -1

/* Page table entry (uma por página virtual do processo) */
typedef struct pte_st {
    int32_t  pfn;        // pfn (physical frame number): frame física onde está mapeada, ou INVALID_PFN
    uint8_t  present:1;       // presente em RAM?
    uint8_t  referenced:1;    // acedida desde a última limpeza (CLOCK)
    uint8_t  dirty:1;         // escrita desde que foi carregada (precisa de swap-out)
    uint32_t last_accessed;   // instante do último acesso (LRU)
} pte_t;

/* Tabela de páginas por processo */
typedef struct page_table_st {
    uint32_t nvalid;         // nº de páginas virtuais válidas
    pte_t   *ptes;               // índice = nº da página virtual - 1
} page_table_t;

/* Descritor de uma frame física */
typedef struct frame_desc_st {
    pte_t    *pte;               // PTE (page table entry) que ocupa esta frame
    int32_t   pid;            // processo dono
    uint32_t  vpn;            // página virtual que aqui está
} frame_desc_t;

/* Pilha de frames livres */
typedef struct free_stack_st {
    uint32_t *ids;
    int       max_size;
    int       top;
} free_stack_t;

/* Um elemento da estrutura de ordem de evicção (cobre FIFO, LRU e CLOCK) */
typedef struct array_elem_st {
    uint32_t pfn;
    uint8_t  use_bit;         // CLOCK
    uint32_t last_accessed;   // LRU
} array_elem_t;

typedef struct array_st {
    array_elem_t *ids;
    int       max_size;
    int       last_index;     // ponteiro do relógio (CLOCK)
    int       top;            // índice do último elemento válido
} array_t;

/* Tabela de frames física */
typedef struct frame_table_st {
    int           num_pframes;
    frame_desc_t *frames;
    free_stack_t  free_stack;
    array_t       eviction_order;
} frame_table_t;

/* Uma página guardada em swap (backing store) */
typedef struct swapped_page_st {
    uint64_t page_id;         // chave: (pid << 32) | vpn
    uint8_t  dirty:1;
    uint32_t last_accessed;
    UT_hash_handle hh;
} swapped_page_t;

typedef struct swap_hash_st {
    int      num_swapped;
    uint32_t last_swap_time_ms;
    swapped_page_t *pages;   // hash table (uthash)
} swap_hash_t;

#endif
