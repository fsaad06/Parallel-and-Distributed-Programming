import numpy as np
import matplotlib.pyplot as plt


matrix = np.loadtxt("matrixMul_Serial.csv", delimiter=",")

# Setting Grids
x = np.arange(matrix.shape[1])  
y = np.arange(matrix.shape[0])  
X, Y = np.meshgrid(x, y)

plt.figure(figsize=(8, 6))
contour = plt.contourf(X, Y, matrix, cmap='viridis')  
plt.colorbar(contour)  # color scale


plt.title("Contour Plot of Matrix C (from CSV)")
plt.xlabel("Column Index")
plt.ylabel("Row Index")
plt.tight_layout()
plt.show()
