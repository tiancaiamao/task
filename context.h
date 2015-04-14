#ifndef _CONTEXT_H
#define _CONTEXT_H

struct Context {
    uint64_t rsp;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
};

void SwapContext(struct Context *old, struct Context *new);

#endif