#include "handling-policy.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <unordered_map>

#include "Shore-user-interface-functions.h"

#define ROS_ENABLED

TCViolationHandler::TCViolationHandler() : _policy() {
    _skip_flag = new int(0);
}

TCViolationHandler::~TCViolationHandler() {}

bool TCViolationHandler::setPolicy(HandlingPolicy policy) {
    _policy = policy;
    _skip_flag = new int(0);
    return true;
}

#ifdef ROS_ENABLED
namespace ros {
int setShorePrioritybyCallbackFuncPtr(void* callback_func_ptr, int priority);
}
#endif

void TCViolationHandler::prioritization() {
    // printf("Hello, in prioritization \n");
    // Write to the shared memory file, content vertex id.
    int ret;

    if (isPrioritizing() == 1) {
        return;
    }

    pid_t tid = gettid();  // Fix the undefined identifier problem
    // pid_t tid = getpid();  // Fix the undefined identifier problem
    
    _shoreline_info->setKernelTaskID(tid);

    /*
     *   This is the function to call to prioritize the callback
     *   int setShorePrioritybyCallbackFuncPtr(void* callback_func_ptr, int
     * priority);
     */

#ifdef ROS_ENABLED
    if (_shoreline_info->hasROSCallback()) {
        // printf("Hello, I am calling the ROS callback function\n");
        void* callback_ptr = _shoreline_info->getROSCallbackFuncPtr();
        ret = ros::setShorePrioritybyCallbackFuncPtr(callback_ptr, 99);

        if (ret == -1) {
            printf("[Error] Failed to elevate task in ROS callback.\n");
        }
    }
    printf("[Shore-Debug] ROS callback is prioritized \n");
#endif

    // This will also impact on Network packet scheduler and then the middleware
    // FIXME do it via Shoreline ID
    ret = shore_user_add_task_to_cpu_list(tid);

// #ifdef Shore_DEBUG
    printf("[Shore-Debug] Task %d added to CPU list\n", tid);
// #endif

    if (ret == 0) {
        setIsPrioritizing(1);
    } else {
        printf(
            "[Error] Failed to elevate task in CPU scheduler. Please enable "
            "the interface\n");
    }

    return;
}

void TCViolationHandler::resetPriority() {
    // Write to the shared memory file, content vertex id.

    pid_t tid = getpid();  // Fix the undefined identifier problem

    shore_user_restore_priority_to_cpu_task(tid);

    return;
}

void TCViolationHandler::skipNext() {
    // FIXME
    // printf("Hello, in SKIP-NEXT \n");
    // Write to the shared memory file, content vertex id.
    *(_skip_flag) = 1;
    return;
}

void TCViolationHandler::resetSkipFlag() {
    *(_skip_flag) = 0;
    return;
}

int TCViolationHandler::handle() {
    int ret = 0;

    if (shouldSkip()) {
        setAborting(1);
        resetSkipFlag();
        goto early_exit;
    }

    switch (_policy) {
        case HandlingPolicy::NONE:
            // Handle no policy
            printf("[Warning] Timing violated, but no policy is set\n");
            break;
        case HandlingPolicy::ABORT:
            // This will trigger abort
            setAborting(1);
            break;
        case HandlingPolicy::PRIORITIZATION:
#ifdef Shore_DEBUG
            // Handle prioritize policy
            printf("[Shore-Debug] Prioritizing the task\n");
#endif
            prioritization();
            break;
        case HandlingPolicy::SKIP_NEXT:
            // Handle skip-next policy
            skipNext();
            break;
        default:
            /*
                * User can deploy their own policy
                TODO . . . .
            */
            break;
    }

early_exit:

    return ret;
}
