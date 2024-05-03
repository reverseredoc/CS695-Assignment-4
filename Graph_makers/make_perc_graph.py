import re
import matplotlib.pyplot as plt
import sys
import os


if len(sys.argv) != 2:
    print("Usage: python script.py <input_file>")
    sys.exit(1)

input_file = sys.argv[1]


# Initialize lists to store data
actual = []
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
        # Check if the line contains "Percentage of pages accessed is"
        if "Percentage of pages accessed is" in line and "Error in Percentage of pages accessed is" not in line:
            
            # Extract the percentage value using regex
            percentage = re.findall(pattern, line)[0]

            # Determine the prefix and append the percentage to the respective list
            if "ACTUAL" in line:
                actual.append(float(percentage))
            elif "CURRENT SAMPLE" in line:
                current_sample.append(float(percentage))
            elif "SLOW MOVING AVG" in line:
                slow_moving_avg.append(float(percentage))
            elif "FAST MOVING AVG" in line:
                fast_moving_avg.append(float(percentage))
            elif "MAX ESTIMATE" in line:
                max_estimate.append(float(percentage))

# Generate x values (runs)
x_values = range(1, len(actual) + 1)
plt.xticks(x_values)



print(len(actual))
print(len(current_sample))
print(len(slow_moving_avg))
print(len(fast_moving_avg))
print(len(max_estimate))


# min_value = min(min(actual), min(current_sample), min(slow_moving_avg), min(fast_moving_avg), min(max_estimate))
max_value = max(max(actual), max(current_sample), max(slow_moving_avg), max(fast_moving_avg), max(max_estimate))
plt.ylim(0, min(max_value + 10, 100))

# Plotting the lines
plt.plot(x_values, current_sample, label="CURRENT SAMPLE")
plt.plot(x_values, slow_moving_avg, label="SLOW MOVING AVG")
plt.plot(x_values, fast_moving_avg, label="FAST MOVING AVG")
plt.plot(x_values, max_estimate, label="MAX ESTIMATE")
plt.plot(x_values, actual, label="ACTUAL")

file_name, _ = os.path.splitext(input_file)


# Add labels and title
plt.xlabel("Run")
plt.ylabel("Percentage of Pages Accessed")
plt.title(f"{file_name}")
# plt.title(f"seq_access")

plt.legend()

# Show the plot
# plt.show()
# plt.savefig("test.png")
# Remove the file extension if it exists
# print(file_name)
# file_name = "ahajaja"
plt.savefig(f"perc_{file_name}.png")

