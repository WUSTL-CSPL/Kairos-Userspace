#!/bin/bash

# Get the number of CPUs available
NUM_CPUS=$(nproc)

# Function to calculate workers based on desired CPU load percentage
calculate_workers() {
    local percentage=$1
    echo $(( (percentage * NUM_CPUS + 99) / 100 )) # Calculate the number of workers, rounded up
}

# Ask for user input on desired CPU load percentage
read -p "Enter desired CPU load percentage (1-100): " CPU_LOAD_PERCENTAGE

# Validate input
if [[ "$CPU_LOAD_PERCENTAGE" -lt 1 || "$CPU_LOAD_PERCENTAGE" -gt 100 ]]; then
    echo "Error: Please enter a valid percentage between 1 and 100."
    exit 1
fi

# Calculate the number of workers to use
WORKERS=$(calculate_workers $CPU_LOAD_PERCENTAGE)

# Execute stress-ng with the calculated number of workers
echo "Starting stress-ng with $WORKERS workers to achieve approx $CPU_LOAD_PERCENTAGE% CPU load..."
stress-ng --cpu $WORKERS --timeout 600s --metrics-brief

echo "Stress test complete."

