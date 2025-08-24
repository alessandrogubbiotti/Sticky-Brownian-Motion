import matplotlib.pyplot as plt
import numpy as np

data = np.loadtxt("trajectory.dat")
time = data[:, 0]
position = data[:, 1]

plt.plot(time, position, lw=1)
plt.xlabel("Rescaled Time")
plt.ylabel("Rescaled Position")
plt.title("Markov Chain Trajectory")
plt.grid(True)

plt.tight_layout()
plt.savefig("trajectory.png")  # or "trajectory.png"
