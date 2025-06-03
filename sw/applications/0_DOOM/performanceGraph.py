import matplotlib.pyplot as plt
# Here is a graph that can be updated by other future contributors
# Labels for each version
versions = [
    "320x200\nNo Cache",
    "320x200\nWith Cache",
    "160x160\nNo Cache",
    "160x160\nWith Cache",
    "Final Version",
    "No Floors/Ceilings"
]

# Average times per frame in seconds
average_times = [18.53, 8.23, 9.04, 4.88, 4.52, 4.23]

# Plotting
plt.figure(figsize=(10, 6))
plt.bar(versions, average_times, color='red')
plt.ylabel("Average Time per Frame (s)")
plt.title("Average Time per Frame Comparison at 15 MHz for the Demo")
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.tight_layout()

plt.show()
