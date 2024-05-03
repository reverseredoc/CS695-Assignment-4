import re
import matplotlib.pyplot as plt
import numpy as np
import sys
import os


if len(sys.argv) != 2:
    print("Usage: python script.py <input_file>")
    sys.exit(1)

input_file = sys.argv[1]

# Initialize lists to store data
current_sample = []
slow_moving_avg = []
fast_moving_avg = []
max_estimate = []

# Regular expression pattern to match percentage values in quotes
pattern = r'"([-+]?\d*\.\d+|\d+)"'

# Open the input file
with open(input_file, "r") as file:
    # Read lines from the file
    lines = file.readlines()

    # Iterate through each line
    for line in lines:
        # Check if the line contains "Error in Percentage of pages accessed is"
        if "Error in Percentage of pages accessed is" in line:
            # Extract the percentage value using regex
            percentage = float(re.findall(pattern, line)[0])

            # Determine the prefix and append the percentage to the respective list
            if "CURRENT SAMPLE" in line:
                current_sample.append(abs(percentage))
            elif "SLOW MOVING AVG" in line:
                slow_moving_avg.append(abs(percentage))
            elif "FAST MOVING AVG" in line:
                fast_moving_avg.append(abs(percentage))
            elif "MAX ESTIMATE" in line:
                max_estimate.append(abs(percentage))

# Generate x values (runs)
x_values = range(1, len(current_sample) + 1)

# Width of each bar
bar_width = 0.2

# Calculate x positions for each group of bars
x_current_sample = np.array(x_values) - bar_width
x_slow_moving_avg = np.array(x_values)
x_fast_moving_avg = np.array(x_values) + bar_width
x_max_estimate = np.array(x_values) + 2 * bar_width


# Plotting the grouped bar graph
plt.bar(x_current_sample, current_sample, width=bar_width, label="CURRENT SAMPLE")
plt.bar(x_slow_moving_avg, slow_moving_avg, width=bar_width, label="SLOW MOVING AVG")
plt.bar(x_fast_moving_avg, fast_moving_avg, width=bar_width, label="FAST MOVING AVG")
plt.bar(x_max_estimate, max_estimate, width=bar_width, label="MAX ESTIMATE")

file_name, _ = os.path.splitext(input_file)


# Add labels and title
plt.xlabel("Run")
plt.ylabel("Absolute Error in Percentage of Pages Accessed")
# plt.title("Absolute Error in Percentage of Pages Accessed for Each Run")

plt.title(f"{file_name}")
# plt.title(f"seq_access")

plt.xticks(x_values)
plt.legend()

plt.ylim(0, 100)

# Show the plot
# plt.savefig("test_bar.png")
plt.savefig(f"error_{file_name}.png")

