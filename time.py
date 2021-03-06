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
colors = ['#15E476', '#E48A15', '#E6E626', '#EB4545', 'white']

# ax = sns.violinplot(x="read_node", y=data["times_ms"].astype(int), data=data, ax=ax)

results_dir = Path('results/')

def makefig(data, out_path: Path, xstr, xticks=None, hue=None):
    fig, ax = plt.subplots(1, 1, figsize=(15,8))
    fig.suptitle(f"{out_path.stem} @ {out_path.parent.stem}", fontsize=fontsize+2)
    ax = sns.swarmplot(x=xstr, y="times_per_page", hue=hue, data=data, size=3, palette=colors)
    method = "swarmplot"
    title = "real time to read 1 page for each"
    ax.set_title(f"{title} {xstr} ({method})")
    ax.set_xlabel(xstr)
    if xticks != None:
        ax.set_xticks(xticks)
    ax.set_yticks(np.linspace(0, max(data["times_per_page"]), 20))
    ax.set_ylabel("time (μs)")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(str(out_path) + ".svg")

def sub_sample(dic, warmup):
    size = len(dic[0]["nodes"])
    iteration = int(size / (points_to_plot + warmup))
    for numa_dic in dic:
        # these data points are not relevant (warmup)
        numa_dic["nodes"] = numa_dic["nodes"][warmup::iteration]
        numa_dic["times_us"] = numa_dic["times_us"][warmup::iteration]

def set_layout(dic):
    dic[0]["layout"] = "LL"
    dic[1]["layout"] = "LD"
    dic[2]["layout"] = "DL"
    dic[3]["layout"] = "DD"

def init_dataframe(dic):
    df = pandas.DataFrame(dic)
    df = df.explode(['times_us', 'nodes'], ignore_index=True)
    df["times_ms"] = df["times_us"] / 1E3
    df["times_per_page"] = df["times_us"] * (0x1000 / 51200000) # 1 page
    return df

def json_plot(json_dic, name, warmup):
    sub_sample(json_dic, warmup)
    df = init_dataframe(json_dic)
    xticks = range(max(df["read_node"]) + 1)
    xaxisname = "init node sched"
    df[xaxisname] = df["read_node"]
    makefig(df, name, xaxisname, xticks=xticks, hue="nodes")

def json_plot_gettime(json_dic, name, warmup):
    set_layout(json_dic)
    sub_sample(json_dic, warmup)
    df = init_dataframe(json_dic)
    makefig(df, name, "layout")

warmup = 20
points_to_plot = 250

for path in results_dir.glob("*/*.json"):
    print(colorama.Fore.CYAN + str(path) + colorama.Fore.RESET)
    with path.open("rb") as json_file:
        json_string = json.load(json_file)["measurements"]
        name = Path("plot/") / path.relative_to(results_dir)
        if "get_time.json" in str(path):
            json_plot_gettime(json_string, name, warmup)
