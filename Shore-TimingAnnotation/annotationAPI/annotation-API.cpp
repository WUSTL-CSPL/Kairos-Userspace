#include "annotation-API.h"

/*
 * User Accessible Timing Correctness Annotation API
 */

// The unit of threshold is in seconds for simplifying the specification

void freshness(void* var, double threshold, int violation_policy) {
    // printf("FRESHNESS is called\n");
    return;
}

// FIXME, maybe define this as variadic function
void consistency(void* var1, void* var2, double threshold,
                 int violation_policy) {
    // printf("CONSISTENCY is called\n");
    return;
}
void consistency3(void* var1, void* var2, void* var3, double threshold,
                  int violation_policy) {
    // printf("CONSISTENCY is called\n");
    return;
}

void stability(void* var, double threshold, int violation_policy) {
    // printf("STABILITY is called\n");
    return;
}

void input(void* var) {
    // printf("INPUT is called\n");
    return;
}
// End of Annotation API