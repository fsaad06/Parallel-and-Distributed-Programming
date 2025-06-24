import matplotlib.pyplot as plt

# Data
x_values = [10000, 20000, 50000, 70000, 100000, 200000, 1000000, 2000000, 10000000, 20000000]
y_set1 = [0.0000477, 0.0001075, 0.000249, 0.0003670, 0.00073, 0.001143, 0.0029, 0.00540, 0.0255, 0.050416]
y_set2 = [0.000324, 0.0000280, 0.00240, 0.0000722, 0.000112, 0.0001357, 0.001510, 0.00270, 0.009269, 0.0158096]
y_set3 = [0.0365791, 0.018785, 0.02789, 0.0503, 0.05321, 0.058477, 0.0102, 0.0100, 0.041674, 0.08104]

# Create plot
plt.figure(figsize=(10, 6))

plt.plot(x_values, y_set1, marker='o', color='blue', linewidth=2, label='Serial')
plt.plot(x_values, y_set2, marker='s', color='green', linewidth=2, label='OpenMP')
plt.plot(x_values, y_set3, marker='^', color='red', linewidth=2, label='Distributed Client Server')

# Set log scale on both axes
plt.xscale('log')
plt.yscale('log')

# Labels and title
plt.title("Execution Time vs Input Size (Log-Log Scale)", fontsize=14)
plt.xlabel("Input Size (log scale)", fontsize=12)
plt.ylabel("Execution Time (seconds, log scale)", fontsize=12)

# Grid and legend
plt.grid(True, which='both', linestyle='--', linewidth=0.5)
plt.legend()

plt.tight_layout()
plt.show()
