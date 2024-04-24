#ifndef SHORE_USER_INTERFACE_FUNCTIONS_H
#define SHORE_USER_INTERFACE_FUNCTIONS_H

// #define SHORE_DEBUG

int shore_user_add_task_to_cpu_list(int tid);
void shore_user_add_task_to_network_list(int tid);
void shore_user_restore_priority_to_cpu_task(int tid);
void shore_user_restore_priority_to_network_task(int tid);
void shore_user_remove_cpu_task_from_list(int tid);
void shore_user_remove_network_task_from_list(int tid);

#endif /* SHORE_USER_INTERFACE_FUNCTIONS_H */