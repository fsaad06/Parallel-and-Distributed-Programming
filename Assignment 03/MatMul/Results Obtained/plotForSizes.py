import matplotlib.pyplot as plt

x_values = [10, 100, 200, 500, 700,1000]
y_set1 = [ 0.00338,0.01893, 0.0668,0.881881, 3.058, 12.123]
y_set2 = [0.005799,0.00467,0.023,0.195,0.629,2.764]
y_set3 = [ 0.0317, 0.03639, 0.0262, 0.2324, 0.85, 4.03075]
plt.figure(figsize=(10, 6))

plt.plot(x_values, y_set1, marker='o', color='blue', label='Serial')
plt.plot(x_values, y_set2, marker='s', color='green', label='OpenMP')
plt.plot(x_values, y_set3, marker='^', color='red', label='Distributed Client Server')

plt.xscale('log')
plt.yscale('log')

plt.title("2D Array Multiplication Results")
plt.xlabel("Input Size (log scale)")
plt.ylabel("Execution Time (seconds)")

plt.grid(True, which="both", linestyle='--', linewidth=0.5)
plt.legend()

plt.tight_layout()
plt.show()
