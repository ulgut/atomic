#ifndef NODE_H
#define NODE_H

#include "rdma.h"

/* Per-node context */
struct node_ctx {
  uint16_t id;
  uint32_t seed;
  struct rdma_ctx r;
#ifdef TRACK_SLOTS
  struct {
    uint64_t lat;
    uint16_t path;
    uint16_t won;
  } *s;
#endif
};

/* Initialize node context */
int node_init(struct node_ctx *ctx, struct config *c);

/* Destroy context */
void node_destroy(struct node_ctx *ctx);

/* Distributed atomic operations */
int64_t fetch_and_add(struct node_ctx *ctx);
int64_t test_and_set(struct node_ctx *ctx, uint32_t slot);
int64_t reset(struct node_ctx *ctx);

/* Performs a quorum read across replicas to fetch maximum frontier slot. */
uint32_t get_frontier_slot(struct node_ctx *ctx);
/* Attempts to advance the frontier slot on all replicas to {new_slot}. */
int advance_frontier(struct node_ctx *ctx, uint32_t new_slot);
/* Runs FastPaxos on slot {slot} at the node referenced by {ctx}. */
int run_fast_paxos(struct node_ctx *ctx, uint32_t slot);

#ifdef TRACK_SLOTS
#include <stdio.h>
#define DUMP_CSV(fp, ctx)                                                      \
  do {                                                                         \
    fprintf(fp, "Host ID,Slot,Latency,Path,Won\n");                            \
    for (int i = 0; i < MAX_SLOTS; ++i)                                        \
      if ((ctx)->s[i].lat)                                                     \
        fprintf(fp, "%hu,%d,%lu,%hu,%hu\n", (ctx)->id, i, (ctx)->s[i].lat,     \
                (ctx)->s[i].path, (ctx)->s[i].won);                            \
  } while (0)
#endif

/* Fetches minimum quorum size for FastPaxos fast-path (≥75%) */
#define FAST_QUORUM(c) ((c->n * 3 + 3) / 4)
/* Fetches minimum quorum size for FastPaxos slow-path (>50%) */
#define CLASSIC_QUORUM(c) (((c)->n / 2) + 1)

#endif /* NODE_H */
