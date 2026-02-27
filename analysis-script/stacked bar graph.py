import matplotlib.pyplot as plt
import numpy as np

# data from https://allisonhorst.github.io/palmerpenguins/

machines = (
    "ELITE",
    "THINK",
)
tempo = np.array([42.65, 44.38])
tpkg = np.array([54.53, 48.28])
tpkgcore = np.array([57.34, 49.06])
all = np.array([64.06, 49.84])
weight_counts = {
    "Time": tempo,
    "Time+pkg": tpkg - tempo,
    "Time+pkg+cores": tpkgcore - tpkg,
    "All data available": all - tpkgcore,
}
width = 0.5

fig, ax = plt.subplots()
bottom = np.zeros(2)

for boolean, weight_count in weight_counts.items():
    p = ax.bar(machines, weight_count, width, label=boolean, bottom=bottom)
    bottom += weight_count

ax.set_title("Classification Accuracy Increasing Number of Attributes (flag -O2)")
ax.legend(loc="upper right")
plt.ylabel('Percentage Correctly Classified')

plt.show()