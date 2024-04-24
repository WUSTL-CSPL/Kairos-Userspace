#pragma once

#include "Shore-common.h"

#include <cstdlib>
#include <iostream>

// For LLVM instrumentation
extern "C" {
    // Function to add function pointers to the map
    void registerROSCallbackFunctionPtr(int id, void* func);
}


// We may need to return different types according to the function
// Currently, this is fine because ros callback function typically return void
// type
#define RETURN_INT 0
#define RETURN_FLOAT 0.0f
#define RETURN_NULLPTR nullptr
#define RETURN_VOID void

// Main macro definition that checks predefined macros
#define DO_ABORT_POLICY return;

/*
#define DO_ABORT_POLICY \
    { \
        #ifdef ABORT_TYPE_INT \
        return RETURN_INT; \
        #elif defined(ABORT_TYPE_FLOAT) \
        return RETURN_FLOAT; \
        #elif defined(ABORT_TYPE_VOID) \
        return; \
        #else \
        return RETURN_NULLPTR; \
        #endif \
    }
*/

#define DO_PRIORITIZE_POLICY(id) \
    { TCViolationHandler::proritization(id); }

#define DO_SKIP_NEXT_POLICY(skip_flag_ptr) \
    { *skip_flag_ptr = true; }

#define SKIP_IF_NEEDED(skip_flag_ptr)      \
    {                                      \
        if (skip_flag_ptr) return nullptr; \
    }

enum class HandlingPolicy {
    NONE = 0,
    ABORT = 1,
    PRIORITIZATION = 2,
    SKIP_NEXT = 3
};

/**
 * @brief The TCViolationHandler class represents a handler for timing
 * constraint violations.
 *
 * It provides methods to handle violations, set handling policies, prioritize
 * violations, reset priority, abort handling, and skip the next violation.
 */
class TCViolationHandler {
   public:
    // This is shared with the vertex
    struct Shoreline_Info* _shoreline_info;

    HandlingPolicy _policy;       /**< The handling policy for the violation handler. */
    int* _skip_flag;   /**< Pointer to the skip flag indicating whether to skip
                          the next violation. */
    int _Shoreline_id; /**< The ID of the Shoreline. */
    int _is_prioritizing = 0; /**< Flag indicating whether prioritization is enabled. */
    int _is_aborting = 0; /**< Flag indicating whether aborting is enabled. */

    /**
     * @brief Default constructor for the TCViolationHandler class.
     */
    TCViolationHandler();

    /**
     * @brief Constructor for the TCViolationHandler class.
     * @param policy The handling policy for the violation handler.
     */
    TCViolationHandler(HandlingPolicy policy) : _policy(policy) {}

    /**
     * @brief Destructor for the TCViolationHandler class.
     */
    ~TCViolationHandler();

    void setShorelineInfo(struct Shoreline_Info* shoreline_info) {
        _shoreline_info = shoreline_info;
    }

    /**
     * @brief Handles the timing constraint violation.
     */
    int handle();

    /**
     * @brief Sets the handling policy for the violation handler.
     * @param policy The handling policy to be set.
     * @return True if the policy was set successfully, false otherwise.
     */
    bool setPolicy(HandlingPolicy policy);

    /**
     * @brief Enables prioritization of violations.
     */
    void prioritization();

    /**
     * @brief Resets the priority of violations.
     */
    void resetPriority();

    /**
     * @brief Aborts the handling of violations.
     */
    void abort();

    /**
     * @brief Skips the next violation.
     */
    void skipNext();

    void resetSkipFlag();
    /**
     * @brief Gets the pointer to the skip flag.
     * @return Pointer to the skip flag.
     */
    int* getSkipFlag() { return _skip_flag; }

    /**
     * @brief Checks if the next violation should be skipped.
     * @return 1 if the next violation should be skipped, 0 otherwise.
     */
    int shouldSkip() { return *_skip_flag; }

    /**
     * @brief Checks if prioritization is enabled.
     * @return 1 if prioritization is enabled, 0 otherwise.
     */
    int isPrioritizing() { return _is_prioritizing; }

    /**
     * @brief Sets the value of the prioritization flag.
     * @param val The value to be set.
     * @return The new value of the prioritization flag.
     */
    int setIsPrioritizing(int val) { return _is_prioritizing = val; }

    /**
     * @brief Checks if aborting is enabled.
     * @return 1 if aborting is enabled, 0 otherwise.
     */
    int isAborting() { return _is_aborting; }
};
