#!/usr/bin/env fish

# Define your project build and flash commands
set build_command "west build -b nrf5340_senseiv1_cpuapp"
set flash_command "west flash"  # Replace with your actual flash command
set serial_monitor_command "pyserial-miniterm - 115200" 

# Build the project
echo "Building the project..."
$build_command

# Check if the build was successful
if test $status -eq 0
    echo "Build successful."

    # Flash the project
    echo "Flashing the project..."
    $flash_command

    # Check if the flash command was successful
    if test $status -eq 0
        echo "Flash successful. Opening serial monitor..."
        # Open the serial monitor
        $serial_monitor_command
    else
        echo "Flash failed."
    end
else
    echo "Build failed."
end