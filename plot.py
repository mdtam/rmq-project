#!/usr/bin/env python3

import os
import matplotlib.pyplot as plt
import pandas as pd

os.makedirs("plots", exist_ok=True)

df = pd.read_csv("data.csv")

# Verify all methods produce the same checksum for each n
checksums = df.pivot(index="n", columns="name", values="sum")
assert (checksums.eq(checksums.iloc[:, 0], axis=0).all(axis=None)), \
    f"Checksum mismatch:\n{checksums}"

plt.rcParams["axes.grid"] = True

names = sorted(df["name"].unique())
colors = {name: f"C{i}" for i, name in enumerate(names)}

def pivot(field):
    return df.pivot(index="n", columns="name", values=field)[names]

# Plot 1: query time per n
fig, ax = plt.subplots()
pivot("time").plot(ax=ax, marker="o", color=[colors[n] for n in names])
ax.set_xlabel("n")
ax.set_ylabel("Query time (ns/query)")
ax.set_xscale("log")
ax.set_yscale("log", base=2)
ax.set_title("Query time per n")
fig.tight_layout()
fig.savefig("plots/query_time.png", dpi=150)
plt.close(fig)

# Plot 2: space usage per n
fig, ax = plt.subplots()
pivot("space").plot(ax=ax, marker="o", color=[colors[n] for n in names])
ax.set_xlabel("n")
ax.set_ylabel("Space (bytes)")
ax.set_xscale("log")
ax.set_yscale("log")
ax.set_title("Space usage per n")
fig.tight_layout()
fig.savefig("plots/space.png", dpi=150)
plt.close(fig)

# Plot 3: space-time tradeoff at max n
max_n = df["n"].max()
tradeoff = df[df["n"] == max_n]
fig, ax = plt.subplots()
for _, r in tradeoff.iterrows():
    ax.scatter(r["space"], r["time"], color=colors[r["name"]], label=r["name"], zorder=3)
    ax.annotate(r["name"], (r["space"], r["time"]),
                textcoords="offset points", xytext=(6, 4))
ax.set_xlabel("Space (bytes)")
ax.set_ylabel("Query time (ns/query)")
ax.set_xscale("log")
ax.set_yscale("log", base=2)
ax.set_title(f"Space–time tradeoff (n={max_n})")
fig.tight_layout()
fig.savefig("plots/space_time_tradeoff.png", dpi=150)
plt.close(fig)

print("Saved plots/query_time.png, plots/space.png, plots/space_time_tradeoff.png")
