/**
 * @file shore_kernel_support_module.c
 * @brief Kernel module to support shore kernel.
 *
 *
 */

/**
 *
 * Load the kernel using insmod (sudo privileges required):
 * Check dmesg for kernel module loading information, create a device file using
 * mknod following the instructions in the kernel module loading information
 * (sudo privileges required).
 *
 * Writing to the device file to set the Shore_id, priority_level, and
 * operation_mode (sudo privileges required):
 *
 * Set the priority level or remove the shore enforced program:
 * echo "shore_id,priority_level,operation_mode" > /dev/shore_kernel_module (1
 * comma)
 *
 * or use fprintf to write to the device file.
 *
 * @shore_id: The shore ID to set the priority level for.
 *
 * @priority_level: The priority level to set for the shore ID.
 * The priority level must be between 0 and 2.
 * 0 is PRIOTIZED,
 * 2 is recover to normal priority.
 * 1 is reserved to the system, does not prioritize nor recover to normal
 * priority. DO NOT USE -1.
 *
 * @operation_mode: The operation mode to perform. 0 is to set the priority
 * level for the shore ID. 1 is to remove the shore ID from the Shore enforced
 * program list.
 *
 * Operation mode 0: add to cpu task list
 * Operation mode 1: add to network task list
 *
 * Operation mode 2: set cpu task priority
 * Operation mode 3: set network task priority
 *
 * Operation mode 4: remove cpu task from task list
 * Operation mode 5: remove network task from task list
 */

#include <asm/uaccess.h> /* for put_user */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>

/*
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

extern int shore_set_cpu_task_priority(int shore_id, int shore_priority);
extern void shore_add_cpu_task_to_dfa_list(int new_shore_id,
                                           int new_shore_priority);
extern void shore_remove_cpu_task_from_dfa_list(int new_shore_id);
extern void shore_remove_all_cpu_tasks_from_dfa_list(void);

extern void shore_add_network_task_to_dfa_list(int shore_net_id,
                                               int shore_net_priority);
extern int shore_set_network_task_priority(int shore_net_id,
                                           int shore_net_priority);
extern void shore_remove_network_task_from_dfa_list(int shore_net_id);
extern void shore_remove_all_network_tasks_from_dfa_list(void);

#define SUCCESS 0
#define DEVICE_NAME \
    "shore_kernel_module" /* Dev name as it appears in /proc/devices */
#define BUF_LEN 80        /* Max length of the message from the device */

/*
 * Global variables are declared as static, so are global within the file.
 */

static int Major;           /* Major number assigned to our device driver */
static int Device_Open = 0; /* Is device open?
                             * Used to prevent multiple access to device */
static char msg[BUF_LEN];   /* The msg the device will give when asked */
static char *msg_Ptr;
static char *received_msg = NULL;

static struct file_operations fops = {.read = device_read,
                                      .write = device_write,
                                      .open = device_open,
                                      .release = device_release};

/*
 * Utility functions
 */
// This works for both pid and tid
struct task_struct *get_task_by_pid(int pid) {
    struct pid *pid_struct;
    struct task_struct *task = NULL;

    pid_struct = find_get_pid(pid);
    if (!pid_struct) {
        printk(KERN_INFO "Could not find pid: %d\n", pid);
        return NULL;
    }

    task = pid_task(pid_struct, PIDTYPE_PID);
    put_pid(
        pid_struct);  // Decrement the reference count obtained by find_get_pid
    if (!task) {
        printk(KERN_INFO "Could not find task for pid: %d\n", pid);
        return NULL;
    }

    return task;
}

/*
 * This function is called when the module is loaded
 */
int init_module(void) {
    Major = register_chrdev(0, DEVICE_NAME, &fops);

    if (Major < 0) {
        printk(KERN_ALERT "Registering char device failed with %d\n", Major);
        return Major;
    }

    received_msg = (char *)kmalloc(1024, GFP_KERNEL);

    if (received_msg != NULL)
        printk("malloc allocator address: 0x%p\n", received_msg);

    printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
    printk(KERN_INFO "the driver, create a dev file with\n");
    printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
    printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
    printk(KERN_INFO "the device file.\n");
    printk(KERN_INFO "Remove the device file and module when done.\n");

    return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void) {
    /*
     * Unregister the device
     */

    if (received_msg) {
        /* release the malloc */
        kfree(received_msg);
    }

    unregister_chrdev(Major, DEVICE_NAME);
    shore_remove_all_network_tasks_from_dfa_list();
    shore_remove_all_cpu_tasks_from_dfa_list();

    printk(KERN_INFO "Remove the device file (/dev/%s) and module when done.\n",
           DEVICE_NAME);
    printk(KERN_INFO "shmem module is being unloaded\n");
}

/*
 * Methods
 */

/*
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file) {
    if (Device_Open) return -EBUSY;

    Device_Open++;
    sprintf(msg, "The shore module has successfully loaded!\n");
    msg_Ptr = msg;
    try_module_get(THIS_MODULE);

    return SUCCESS;
}

/*
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file) {
    Device_Open--; /* We're now ready for our next caller */

    /*
     * Decrement the usage count, or else once you opened the file, you'll
     * never get get rid of the module.
     */
    module_put(THIS_MODULE);

    return 0;
}

/*
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */
                           char *buffer,      /* buffer to fill with data */
                           size_t length,     /* length of the buffer     */
                           loff_t *offset) {
    /*
     * Number of bytes actually written to the buffer
     */
    int bytes_read = 0;

    /*
     * If we're at the end of the message,
     * return 0 signifying end of file
     */
    if (*msg_Ptr == 0) return 0;

    /*
     * Actually put the data into the buffer
     */
    while (length && *msg_Ptr) {
        /*
         * The buffer is in the user data segment, not the kernel
         * segment so "*" assignment won't work.  We have to use
         * put_user which copies data from the kernel data segment to
         * the user data segment.
         */
        put_user(*(msg_Ptr++), buffer++);

        length--;
        bytes_read++;
    }

    /*
     * Most read functions return the number of bytes put into the buffer
     */
    return bytes_read;
}

/*
 * Called when a process writes to dev file: echo "hi" > /dev/hello
 */
static ssize_t device_write(struct file *filp, const char *buff, size_t len,
                            loff_t *off) {
    /**
     * This function receives a message from user space and extracts the shore
     * ID and priority level.
     *
     * @param received_msg The buffer to store the received message.
     * @param buff The buffer containing the message from user space.
     * @param len The length of the message.
     * @return The length of the message.
     */
    unsigned long count;  // Variable to store the length of the message
    int shore_id, shore_priority_level,
        operation_mode;  // Variables to store the shore ID, priority level, and
                         // return value
    char *separator1, *separator2;  // Pointer to a character separator

    struct task_struct *target_task;

    memset(received_msg, 0, 1024);  // Clear the received message buffer
    count = copy_from_user(received_msg, buff,
                           len);  // Copy the message from user space to the
                                  // received message buffer

    separator1 =
        strchr(received_msg, ',');  // Find the separator in the message

    if (!separator1) {
        // No comma found
        return -EINVAL;
    }

    separator2 = strchr(separator1 + 1,
                        ',');  // Find the second separator in the message

    if (separator2) {
        // Two commas found
        *separator1 =
            '\0';  // Temporarily end the string at the first comma for parsing
        *separator2 =
            '\0';  // Temporarily end the string at the second comma for parsing

        shore_id = simple_strtoul(received_msg, NULL,
                                  10);  // Extract the shore ID from the message
        shore_priority_level =
            simple_strtoul(separator1 + 1, NULL,
                           10);  // Extract the priority level from the message
        operation_mode =
            simple_strtoul(separator2 + 1, NULL,
                           10);  // Extract the operation mode from the message

        // *separator = ','; // Restore the second comma in case the buffer is
        // needed again

        // simple_strtol returns long, so make sure the values are within int
        // range
        if (shore_id > INT_MAX || shore_id < INT_MIN ||
            shore_priority_level > INT_MAX || shore_priority_level < INT_MIN ||
            operation_mode > INT_MAX || operation_mode < INT_MIN) {
            return -ERANGE;  // Value out of int range
        }

        // TODO remove
        target_task = get_task_by_pid(shore_id);

        if (target_task) {
            printk(KERN_INFO "Found task_struct: %s [PID = %d, State = %ld]\n",
                   target_task->comm, target_task->pid, target_task->state);
            // Directly modify the task priority here
        }

        if (operation_mode == 0) {
            printk(
                "[Shore-Kernel-Interface] Received shore_id: %d, mode/state: "
                "%d\n",
                shore_id, shore_priority_level);
            shore_add_cpu_task_to_dfa_list(shore_id, shore_priority_level);
            // shore_add_network_task_to_dfa_list(shore_id,
            // shore_priority_level);
        } else if (operation_mode == 1) {
            shore_add_network_task_to_dfa_list(shore_id, shore_priority_level);
            printk("Received shore_net_id: %d, priority: %d\n", shore_id,
                   shore_priority_level);
        } else if (operation_mode == 2) {
            shore_set_cpu_task_priority(shore_id, shore_priority_level);
            // shore_set_network_task_priority(shore_id, shore_priority_level);
            printk("Priority set for shore_id: %d, priority: %d\n", shore_id,
                   shore_priority_level);
        } else if (operation_mode == 3) {
            shore_set_network_task_priority(shore_id, shore_priority_level);
            printk("Priority set for shore_net_id: %d, priority: %d\n",
                   shore_id, shore_priority_level);
        } else if (operation_mode == 4) {
            shore_remove_cpu_task_from_dfa_list(shore_id);
            printk("Task removed from DFA list: shore_id: %d\n", shore_id);
        } else if (operation_mode == 5) {
            // shore_remove_cpu_task_from_dfa_list(shore_id);
            shore_remove_network_task_from_dfa_list(shore_id);
            printk("Task removed from DFA net list: shore_id: %d\n", shore_id);
        } else {
            printk("Invalid operation mode: %d\n", operation_mode);
            return -EINVAL;  // Invalid operation mode
        }

        // printk("Received shore_net_id: %d, priority: %d\n", shore_id,
        // shore_priority_level);

        return len;
    } else {
        printk(
            "Invalid usage, please use the following format: "
            "shore_id,priority_level,operation_mode\n");
        return -EINVAL;  // Invalid number of commas
    }

    // *separator1 = '\0'; // Temporarily end the string at the comma for
    // parsing

    // shore_id = simple_strtoul(received_msg, NULL, 10); // Extract the shore
    // ID from the message shore_priority_level = simple_strtoul(separator1 + 1,
    // NULL, 10); // Extract the priority level from the message

    // // *separator = ','; // Restore the comma in case the buffer is needed
    // again

    // // simple_strtol returns long, so make sure the values are within int
    // range if (shore_id > INT_MAX || shore_id < INT_MIN ||
    // shore_priority_level > INT_MAX || shore_priority_level < INT_MIN) {
    //     return -ERANGE; // Value out of int range
    // }
    // shore_add_cpu_task_to_dfa_list(shore_id, shore_priority_level);
    // shore_add_network_task_to_dfa_list(shore_id, shore_priority_level);

    // printk("Received shore_id: %d, priority: %d\n", shore_id,
    // shore_priority_level);

    return len;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tomson Li");
MODULE_DESCRIPTION("Shore Support Kernel Module");