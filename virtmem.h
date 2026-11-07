//
// Subsistema de memória virtual: tabelas de páginas, frame table, swap.
//

#ifndef VIRTMEM_H
#define VIRTMEM_H

#include "virtmem_types.h"
#include "pcb.h"

typedef enum {
    FIFO = 0,
    RANDOM,
    LRU,
    NRU,
    CLOCK
} eviction_algo_en;

extern const char* eviction_algo_str[];

int set_eviction_algo(const char *ev_str);   // -1 se o nome for inválido / não implementado
const char *get_eviction_algo_str(void);

int create_page_table(page_table_t *pt, int num_vpages);
frame_table_t *create_frame_table(int num_pframes);

pte_t *find_page(page_table_t *pt, int32_t vpn);
int is_active(pte_t *page);
int is_valid(pte_t *page);

int init_free_stack(free_stack_t *stack, int num_pframes);
int push_free_frame(free_stack_t *stack, int pfn);
int pop_free_frame(free_stack_t *stack);

int init_eviction_arr(array_t *array, int num_pframes);
int update_eviction_array(array_t *eviction_arr, int pfn, uint32_t current_time_ms);
void remove_from_eviction_array(array_t *eviction_arr, int pfn);
int evict(array_t *eviction_arr);

int swap_out(swap_hash_t *swap, frame_desc_t *fd);
int swap_in(swap_hash_t *swap, frame_desc_t *fd);
void destroy_swap(swap_hash_t *swap);

int page_eviction(frame_table_t *frame_table, swap_hash_t *swap, int32_t min_pages_threshold);
pte_t *page_request(pcb_t *pcb, frame_table_t *frame_table, swap_hash_t *swap, int vpn);
void free_pages(frame_table_t *frame_table, swap_hash_t *swap, pcb_t *pcb);
int free_frames_from_memory(frame_table_t *frame_table, int32_t pid);
int free_frames_from_swap(swap_hash_t *swap, int32_t pid, int32_t pages);

#endif //VIRTMEM_H
