
#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

enum HandlingPolicy { NONE = 0, ABORT = 1, PRIORITIZATION = 2, SKIP_NEXT };

/*
 * User Accessible Timing Correctness Annotation API
 */

// The unit of threshold is in seconds for simplifying the specification

#define FRESHNESS(x, t, policy) \
    freshness(x, t, policy);    \
    \  
    if (shouldAbort == 1) {     \
        shouldAbort = 0;        \
        return;                 \
    }

#define FRESHNESS(x, t, policy) \
    freshness(x, t, policy);    \
    \  
    if (shouldAbort == 1) {     \
        shouldAbort = 0;        \
        continue;               \
    }

#define STABILITY(var, thres, policy) \
    stability(var, thres, policy);    \
    \  
    if (shouldAbort == 1) {           \
        shouldAbort = 0;              \
        return;                       \
    }

#define STABILITY_C(var, thres, policy) \
    stability(var, thres, policy);      \
    \  
    if (shouldAbort == 1) {             \
        shouldAbort = 0;                \
        continue;                       \
    }

#define CONSISTENCY(x, y, t, policy) \
    consistency(x, y, t, policy);    \
    \  
    if (shouldAbort == 1) {          \
        shouldAbort = 0;             \
        return;                      \
    }

#define CONSISTENCY_C(x, y, t, policy) \
    consistency(x, y, t, policy);      \
    if (shouldAbort == 1) {            \
        shouldAbort = 0;               \
        printf("Frame skipped.\n");    \
        \    
        continue;                      \
    }

#define CONSISTENCY3_C(var1, var2, var3, thres, policy) \
    consistency3(var1, var2, var3, thres, policy);      \
    if (shouldAbort == 1) {                             \
        shouldAbort = 0;                                \
        continue;                                       \
    }


// This is not used in the current implementation
#define INPUT(x) input(x->get());


/*
 * Function declarations
 */

void freshness(void* var, double threshold, int violation_policy);

void consistency(void* var1, void* var2, double threshold,
                 int violation_policy);
void consistency3(void* var1, void* var2, void* var3, double threshold,
                  int violation_policy);

void stability(void* var, double threshold, int violation_policy);

void input(void* var);

// End of Annotation API
