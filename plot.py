import random

import colorama
import json
from pathlib import Path
import os
import pandas
import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
fontsize = 18
plt.rcParams['font.size'] = fontsize

results_dir = Path('results/')
points_to_plot = 300

def plottime(data, out_path: Path):
    fig, ax = plt.subplots(1, 1, figsize=(15,8))
    numa = max(data["read_node"]) + 1
    xaxis = range(numa)
    fig.suptitle(f"{out_path.stem} @ {out_path.parent.stem}", fontsize=fontsize+2)
    ax = sns.swarmplot(x="read_node", y="times_ms", hue="nodes", data=data, size=3)
    # ax = sns.violinplot(x="read_node", y=data["times_ms"].astype(int), data=data, ax=ax)
    method = "swarmplot"
    ax.set_title(f"read real time for each numa node ({method})")
    ax.set_xlabel("numa node")
    ax.set_xticks(xaxis)
    ax.set_yticks(np.arange(0, max(data["times_ms"]), 2))
    ax.set_ylabel("time ($ms$)")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(str(out_path) + ".png")

def json_plot(json_dic, name, warmup):
    for numa_dic in json_dic:
        # these data points are not relevant (warmup)
        numa_dic["nodes"] = numa_dic["nodes"][warmup:]
        numa_dic["times_us"] = numa_dic["times_us"][warmup:]
        # we have decided to only keep this amount of data points
        nodes_sub, times_us_sub = \
            zip(*random.sample(list(zip(numa_dic["nodes"], numa_dic["times_us"])), points_to_plot))
        numa_dic["nodes"] = list(nodes_sub)
        numa_dic["times_us"] = list(times_us_sub)
    df = pandas.DataFrame(json_dic)
    df = df.explode(['times_us', 'nodes'], ignore_index=True)
    df["times_ms"] = df["times_us"] / 1E3
    plottime(df, name)


for path in results_dir.glob("*/*.json"):
    print(colorama.Fore.CYAN + str(path) + colorama.Fore.RESET)
    with path.open("rb") as json_file:
        json_string = json.load(json_file)
        json_plot(json_string["measurements"], Path("plot/") / path.relative_to(results_dir), warmup = 50)
