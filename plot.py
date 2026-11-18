"""plot.py -- draws the trajectory written by sticky.c.

Reads trajectory.dat (rescaled time and rescaled position, one line per jump)
and saves trajectory.png. Between two jumps the walk does not move, so the
trajectory is drawn as a step function.

    python3 plot.py
"""
import matplotlib
matplotlib.use("Agg")          # no window is needed: the picture is saved to a file
import matplotlib.pyplot as plt
import numpy as np

data = np.loadtxt("trajectory.dat", ndmin=2)
time = data[:, 0]
position = data[:, 1]

# The first line of the file carries the parameters of the run.
with open("trajectory.dat") as f:
    header = f.readline().strip()
title = "Markov Chain Trajectory"
if header.startswith("#"):
    title += "  (" + header.lstrip("# ") + ")"

plt.step(time, position, where="post", lw=1)
plt.xlabel("Rescaled Time")
plt.ylabel("Rescaled Position")
plt.title(title)
plt.grid(True)

plt.tight_layout()
plt.savefig("trajectory.png")
