#!/bin/bash
# Check if the script is run as root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root (use sudo)." >&2
    exit 1
fi

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <enable|disable>"
    exit 1
fi

if [ "$1" = "enable" ]; then
    make
    # Insert the kernel module
    dmesg --clear
    insmod shore_kernel_support_module.ko
    # Wait a bit for the system to register the module and log the messages
    sleep 1
    # Extract the relevant dmesg line, remove everything before and include the first quote,
    # and then remove the last quote plus a period at the end of the line
    command_to_execute=$(dmesg | grep mknod | tail -1 | sed "s/.*'\(.*\)'.\$/\1/")
    # Execute the extracted command
    eval $command_to_execute

    chmod a+rw /dev/shore_kernel_module

    echo "Shore kernel support enabled"

elif [ "$1" = "disable" ]; then
    # Remove the kernel module
    rmmod shore_kernel_support_module.ko
    make clean
    rm /dev/shore_kernel_module
    echo "Shore kernel support disabled"
else
    echo "Invalid argument: $1"
    echo "Usage: $0 <enable|disable>"
    exit 1
fi