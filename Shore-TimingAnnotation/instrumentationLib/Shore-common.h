#pragma once

#include <chrono>
#include <cstdint>
#include <string>

struct Shore_Time {
    uint32_t sec;
    uint32_t nsec;
};

// This is derived from the ROS message header.
// The Interface for ROS

struct Shore_Header {
    // Define the sequence type as a simple unsigned integer.
    typedef uint32_t SeqType;
    SeqType seq;

    struct Shore_Time stamp;

    // Define the frame ID as a standard string.
    std::string frame_id;

    double timestampInSec() {
        return static_cast<double>(stamp.sec) +
               1e-9 * static_cast<double>(stamp.nsec);
    };

    uint64_t timestampInNSec() {
        return static_cast<uint64_t>(stamp.sec) * 1000000000ull +
               static_cast<uint64_t>(stamp.nsec);
    };
};

struct Shoreline_Info {
    int shoreline_id;
    Shore_Header header;
    // pid_t = int
    int kernel_task_id;
    int has_ros_callback = 0;
    void* ros_callback_func_ptr;
    int priority;

    int vertex_id;

    Shoreline_Info(int shoreline_index) {
        shoreline_id = shoreline_index;
        ros_callback_func_ptr = nullptr;
        priority = 0;
    }
    int getShorelineID() { return shoreline_id; }
    int setShorelineID(int id) {
        shoreline_id = id;
        return 0;
    }
    int getKernelTaskID() { return kernel_task_id; }
    int setKernelTaskID(int task_id) {
        kernel_task_id = task_id;
        return 0;
    }


    int hasROSCallback() { return has_ros_callback; }
    void* getROSCallbackFuncPtr() { return ros_callback_func_ptr; }
    int setROSCallbackFuncPtr(void* callback_func_ptr) {
        ros_callback_func_ptr = callback_func_ptr;
        has_ros_callback = 1;
        return 0;
    }
    int getPriority() { return priority; }
    int setPriority(int priority) {
        this->priority = priority;
        return 0;
    }

    int setVertexID(int id) {
        vertex_id = id;
        return 0;
    }
    int getVertexID() { return vertex_id; }
};