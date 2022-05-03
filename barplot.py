import json
import os
import pandas
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.patches as patch
pandas.set_option("display.max_rows", 10, "display.max_columns", None)

colors = ['#15E476', '#E48A15', '#E6E626', '#EB4545', 'white']

def set_layout(dic):
    dic[0]["layout"] = "LL"
    dic[1]["layout"] = "LD"
    dic[2]["layout"] = "DL"
    dic[3]["layout"] = "DD"

def get_layout_times():
    f = open("results/yeti/test_get_time.json")
    dic = json.load(f)["measurements"]
    set_layout(dic)
    layout_times = np.array([np.average(i["times_us"]) for i in dic])
    return layout_times

def what_layout_time(layout_times, value):
    idx = (np.abs(layout_times - value)).argmin()
    return layout[idx]

def what_layout_node(node_read, node_pcache, node_buffer):
    # (pagecache, buffer) = (local|distant, local|distant)
    _layout = ""
    node_read = 1 << node_read
    for node in [1 << node_pcache, node_buffer]:
        if (node_read == node):
            _layout += "L"
        else:
            _layout += "D"
    return _layout

def format_dic(dic, layout_times, warmup):
    for exp in dic:
        list_layout = []
        exp["times_us"] = exp["times_us"][warmup:]
        exp["buffer_nodes"] = exp["buffer_nodes"][warmup:]
        for i in range(len(exp["times_us"])):
            list_layout += [what_layout_node(exp["nodes"][i], exp["pagecache_node"], exp["buffer_nodes"][i])]
        exp["list_layout"] = list_layout
        for l in layout:
            exp[l] = np.count_nonzero([x == l for x in list_layout]) / len(exp["times_us"])
        # delete useless fields
        for field in ["times_us", "nodes","buffer_core","init_core","read_core"]:
            exp.pop(field, None)

def barplot_save(df, ax, separator = 4):
    # df.reset_index(drop=True, inplace=True)
    dummy_row = df.iloc[:1].copy()
    for i in layout:
        dummy_row[i] = 0.0
    for i in range(separator, 1, -1):
        j = (i - 1)*4
        df = pandas.concat([df.iloc[:j], dummy_row, df.iloc[j:]], ignore_index = True)

    length = len(df)
    x = range(length)
    bottom_sum = np.zeros(length)
    for i, tag in enumerate(layout):
        ax.bar(x, df[tag], bottom=bottom_sum, color=colors[i], width=.95)
        bottom_sum += df[tag]
    ax.legend(layout)

def barplot(df, ax, separator = 4):
    # df.reset_index(drop=True, inplace=True)
    dummy_row = df.iloc[:1].copy()
    for i in layout:
        dummy_row[i] = 0.0
    for i in range(separator, 1, -1):
        j = (i - 1)*4
        df = pandas.concat([df.iloc[:j], dummy_row, df.iloc[j:]], ignore_index = True)

    x = range(len(df))
    for row in x:
        bottom_sum = 0
        if row > 0 and (row + 1) % 5 == 0:
            continue 
        for _layout in df.iloc[row]["list_layout"]:
            color_id = np.argwhere(np.array(layout) == _layout)[0][0]
            bottom_sum += 1
            ax.bar(row, 1, bottom=bottom_sum, color=colors[color_id], width=.95)

    ax.legend([patch.Patch(color=colors[0]), patch.Patch(color=colors[1]), patch.Patch(color=colors[2]), patch.Patch(color=colors[3])], layout, loc='upper right')

def json_plot_gettime_all(json_dic, name, warmup):
    layout_times = get_layout_times()
    format_dic(json_dic, layout_times, warmup)

    print(layout_times)
    df = pandas.DataFrame(json_dic)

    fig, ax = plt.subplots(4, 1, figsize=(14, 20))

    xlabels = [ f"({i}, {j})" if j < 4 else "" for i in range(4) for j in range(5)]
    xlabels.pop()
    xticks = np.arange(len(xlabels)) 
    for node_read, ax_read in enumerate(np.array(ax).flat):
        barplot(df[node_read::4].copy(deep=True), ax_read)
        ax_read.set_title(f"read on node {node_read}")
        ax_read.set_xticks(xticks, xlabels)
        ax_read.set_xlabel("(pagecache node, buff node)")
        ax_read.set_ylabel("number of iterations")
    fig.suptitle(name, fontsize=16)

    fig.savefig("plot/yeti/" + name + ".png")

warmup = 20
points_to_plot = 250
layout = ["LL", "LD", "DL", "DD"]

path = "results/yeti/"

test_name = "test_get_time_all_scenarios"
for suffix in ["", "_forced", "_bound", "_bound_forced"]:
    file = f"{path}{test_name}{suffix}.json"
    if os.path.exists(file):
        json_file = open(file)
        json_string = json.load(json_file)["measurements"]
        json_plot_gettime_all(json_string,f"{test_name}{suffix}", warmup)
