#ifndef RDMA_H
#define RDMA_H

#include <infiniband/verbs.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"

/* Remote memory attributes.
 * Exchanged over TCP during the RDMA handshake */
struct remote_attr {
  uint64_t addr;
  uint32_t rkey;
  uint16_t lid;
  uint32_t qpn;
  uint32_t psn;
  uint8_t gid[16];
} __attribute__((packed));

/* Prepare phase result */
struct prep_res {
  uint64_t ballot;
  uint8_t success;
};

/* Per-node RDMA context */
struct rdma_ctx {
  struct ibv_context *ctx;
  uint16_t lid;
  uint8_t gid[16];
  struct ibv_pd *pd;
  struct ibv_mr *mr[2];
  struct ibv_cq *cq;   // CQ for consensus operations
  struct ibv_cq *fcq;  // CQ for frontier operations
  struct ibv_qp **qp;  // QPs for consensus operations
  struct ibv_qp **fqp; // QPs for frontier FAA
  struct {             // Shared (RDMA accessible)
    uint64_t frontier;
    uint64_t slots[MAX_SLOTS];
  } *shared_mem;
  uint64_t *results;
  struct prep_res *prepares;
  struct remote_attr *ra;
  int max_inline;
  struct config *c;
};

/* Initialize RDMA context */
int rdma_init(struct rdma_ctx *r, struct config *c);

/* Destroy RDMA context */
void rdma_destroy(struct rdma_ctx *r);

/* Get next slot from frontier node */
uint64_t rdma_get_next_slot(struct rdma_ctx *r);

/* Fast path operations */
int rdma_bcas(struct rdma_ctx *r, uint32_t slot, uint64_t swp);
#define rdma_btas(r, slot) rdma_bcas(r, slot, 1)

/* Slow path */
int rdma_slow_path(struct rdma_ctx *r, uint32_t slot, uint64_t ballot,
                   uint64_t proposed_value);

/* Timestamp function */
static inline uint64_t ts_us(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL;
}

/* Generate ballot number: (timestamp << 16) | node_id */
static inline uint64_t gen_ballot(uint16_t node_id) {
  uint64_t ts = ts_us() & 0xFFFFFFFFFFFFULL;
  if (ts == 0)
    ts = 1;
  return (ts << 16) | node_id;
}

/* Wait for {num_posted} RDMA operations to complete */
int rdma_await_completions(struct rdma_ctx *r, int num_posted, int min_required, bool require_success, struct ibv_wc *results);
/* Generic RDMA WRITE to a replica's shared memory at {offset}, stored in {result_buf}.*/
int rdma_write(struct rdma_ctx *r, int remote_idx, size_t offset, uint64_t value, uint64_t *local_buf);

/* Generic RDMA CAS on a replica's shared memory at {offset}, stored in {result_buf}.*/
int rdma_cas(struct rdma_ctx *r, int remote_idx, size_t offset, 
             uint64_t expected, uint64_t swap, uint64_t *result_buf);

/* Generic RDMA READ from a replica's shared memory at {offset}, stored in {result_buf}.*/
int rdma_read(struct rdma_ctx *r, int replica_idx, size_t offset, uint64_t *result_buf);

#endif /* RDMA_H */
