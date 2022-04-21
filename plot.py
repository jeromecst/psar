import colorama
import json
from pathlib import Path
import os
import pandas
import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns

results_dir = Path('results/')

def plottime(rtime, cputime, data, out_path: Path):
    numa = len(rtime)
    fig, ax = plt.subplots(2, 1, figsize=(10,10))
    # ax[0].boxplot(rtime, positions=range(numa))
    ax[0].violinplot(rtime, positions=range(numa))
    # ax[0] = sns.boxplot(x="read_node", y="times_us", hue="nodes", data=data)
    data = data[::32]
    ax[1] = sns.swarmplot(x="read_node", y="times_us", hue="nodes", data=data)
    for i, method in enumerate(["boxplot", "violinplot"]):
        ax[i].set_title(f"read real time for each numa node ({method})")
        ax[i].set_xlabel("numa node")
        ax[i].set_xticks(range(numa))
        ax[i].set_ylabel("time ($\mu s$)")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(str(out_path) + ".png")

def json_plot(json_dic, name, warmup):
    # 2D array containing time[] for each numa node
    # len(time) should be equal to number of numa node
    df = pandas.DataFrame(json_dic)
    # df["times_us"] = df["times_us"].str.split('[ :]')
    df = df.explode(['times_us', 'nodes'], ignore_index=True)
    r_time = [i["times_us"][warmup:] for i in json_dic]
    plottime(r_time, r_time, df, name)


for path in results_dir.glob("*/*.json"):
    print(colorama.Fore.CYAN + str(path) + colorama.Fore.RESET)
    with path.open("rb") as json_file:
        json_string = json.load(json_file)
        json_plot(json_string["measurements"], Path("plot/") / path.relative_to(results_dir), warmup = 50)
