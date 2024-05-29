#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
    if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
    if (proc == NULL || q == NULL|| q->size == MAX_QUEUE_SIZE) return;
	q->proc[q->size] = proc;
	q->size += 1;
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
		* in the queue [q] and remember to remove it from q
		* */
	uint32_t min_prio = UINT32_MAX;
	int index = -1;
	if (!empty(q)) {
		for (int i = 0; i < q->size; i++) {
			if (q->proc[i]->priority < min_prio) {
				min_prio = q->proc[i]->priority;
				index = i;
			}
		}
		struct pcb_t *min_proc = q->proc[index];
		for (int i = index; i < q->size - 1; i++) {
			q->proc[i] = q->proc[i + 1];
		}
		q->proc[q->size - 1] = NULL;
		q->size -= 1;
		return min_proc;
	}
	return NULL;
}

