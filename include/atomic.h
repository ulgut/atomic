#ifndef ATOMIC_H
#define ATOMIC_H

/* Shared memory byte offset for frontier */
#define FRONTIER_OFFSET(r) offsetof(typeof(*(r)->shared_mem), frontier)
/* Shared memory byte offset for {slot} */
#define SLOT_OFFSET(r, slot) (offsetof(typeof(*(r)->shared_mem), slots) + ((slot) * sizeof(uint64_t)))

#endif /* ATOMIC_H */
