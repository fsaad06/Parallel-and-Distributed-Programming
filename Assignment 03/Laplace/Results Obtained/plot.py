import matplotlib.pyplot as plt

x_values = [1, 2, 4, 8, 16]

y_set1 = [ 0.510,0.4326,0.427,0.431,0.4539]
y_set2 = [0.4760,0.2590,0.1754,2.277,3.16944]
y_set3 = [ 0.112,0.244,21.22,0.870,0.8525]

plt.figure(figsize=(10, 6))

plt.plot(x_values, y_set1, marker='o', color='blue', label='Serial')
plt.plot(x_values, y_set2, marker='s', color='green', label='OpenMP')
plt.plot(x_values, y_set3, marker='^', color='red', label='Distributed Client Server')

plt.yscale('log')

plt.title("Laplace Calculations")
plt.xlabel("Input Size (log scale)")
plt.ylabel("Execution Time (seconds)")

plt.grid(True, which="both", linestyle='--', linewidth=0.5)
plt.legend()

plt.tight_layout()
plt.show()
