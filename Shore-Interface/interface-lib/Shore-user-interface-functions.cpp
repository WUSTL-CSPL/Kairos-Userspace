

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "Shore-user-interface-functions.h"

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

const char* FILE_PATH = "/dev/shore_kernel_module";

int shore_user_add_task_to_cpu_list(int tid) {
    FILE *fp;
    
    fp = fopen(FILE_PATH, "r+");
    if (fp != NULL) {
        fprintf(fp, "%i,%d,%d", tid, 0, 0);
#ifdef SHORE_DEBUG
        printf("Task %d added to CPU list\n", tid);
#endif
        fclose(fp);
        return 0;
    } else {
        printf("[Handling] Failed to open %s file\n", FILE_PATH);
        return -1;
    }

    return 0;
}

void shore_user_add_task_to_network_list(int tid) {
    FILE *fp;

    fp = fopen(FILE_PATH, "r+");
    if (fp != NULL) {
        fprintf(fp, "%i,%d,%d", tid, 0, 1);
        fclose(fp);
    } else {
        printf("[Handling] Failed to open %s file\n", FILE_PATH);

    }
}

void shore_user_restore_priority_to_cpu_task(int tid) {
    FILE *fp;

    fp = fopen(FILE_PATH, "r+");
    if (fp != NULL) {
        fprintf(fp, "%i,%d,%d", tid, 2, 2);
        fclose(fp);
    } else {
        printf("[Handling] Failed to open %s file\n", FILE_PATH);
    }
}

void shore_user_restore_priority_to_network_task(int tid) {
    FILE *fp;

    fp = fopen(FILE_PATH, "r+");
    if (fp != NULL) {
        fprintf(fp, "%i,%d,%d", tid, 2, 3);
        fclose(fp);
    } else {
        printf("[Handling] Failed to open %s file\n", FILE_PATH);
    }
}

void shore_user_remove_cpu_task_from_list(int tid) {
    FILE *fp;

    fp = fopen(FILE_PATH, "r+");
    if (fp != NULL) {
        fprintf(fp, "%i,%d,%d", tid, 0, 4);
        fclose(fp);
    } else {
        printf("[Handling] Failed to open %s file\n", FILE_PATH);
    }
}

void shore_user_remove_network_task_from_list(int tid) {
    FILE *fp;

    fp = fopen(FILE_PATH, "r+");
    if (fp != NULL) {
        fprintf(fp, "%i,%d,%d", tid, 0, 5);
        fclose(fp);
    } else {
        printf("[Handling] Failed to open %s file\n", FILE_PATH);
    }
}