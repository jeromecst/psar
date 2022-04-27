import json
import numpy as np
import seaborn as sns
import pandas
import matplotlib.pyplot as plt

def set_layout(dic):
    dic[0]["layout"] = "ll"
    dic[1]["layout"] = "ld"
    dic[2]["layout"] = "dl"
    dic[3]["layout"] = "dd"

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
            _layout += "l"
        else:
            _layout += "d"
    return _layout

def format_dic(dic, layout_times, warmup):
    for exp in dic:
        list_layout = []
        exp["times_us"] = exp["times_us"][warmup:]
        exp["buffer_nodes"] = exp["buffer_nodes"][warmup:]
        for i in range(len(exp["times_us"])):
            list_layout += [what_layout_node(exp["nodes"][i], exp["pagecache_node"], exp["buffer_nodes"][i])]
        for l in layout:
            exp[l] = np.count_nonzero([x == l for x in list_layout]) / len(exp["times_us"])
        # delete useless fields
        for field in ["nodes","buffer_core","init_core","read_core"]:
            exp.pop(field, None)

def barplot(df, ax):
    x = range(len(df))
    colors = ['#33cc33', '#3366ff', '#006699', '#DB4444']
    bottom_sum = np.zeros(len(df))
    for i, tag in enumerate(layout):
        ax.bar(x, df[tag], bottom=bottom_sum, color=colors[i])
        bottom_sum += df[tag]
    ax.legend(layout)

def violinplot(df, ax):
    x = range(len(df))
    colors = ['#33cc33', '#3366ff', '#006699', '#DB4444']
    bottom_sum = np.zeros(len(df))
    for index, row in df.iterrows():
        df["times_us"][index] = np.array(row["times_us"])[np.array(row["times_us"]) < 25000]
    ax.violinplot(df["times_us"], np.array(x))

def json_plot_gettime_all(json_dic, name, warmup):
    layout_times = get_layout_times()
    format_dic(json_dic, layout_times, warmup)

    print(layout_times)
    df = pandas.DataFrame(json_dic)

    # pandas.set_option("display.max_rows", 65, "display.max_columns", None)
    # print(df)

    fig, ax = plt.subplots(4, 1, figsize=(14, 20))

    xlabels = [ f"({i}, {j})" for i in range(4) for j in range(4)]
    xticks = np.arange(16) 
    for node_read, ax_read in enumerate(np.array(ax).flat):
        barplot(df[node_read::4], ax_read)
        # violinplot(df[node_read::4], ax_read)
        ax_read.set_title(f"read on node {node_read}")
        ax_read.set_xticks(xticks, xlabels)
        ax_read.set_xlabel("(pagecache node, buff node)")

    fig.savefig("plot/yeti/" + name + ".png")

warmup = 20
points_to_plot = 250
layout = ["ll", "ld", "dl", "dd"]

json_file = open("results/yeti/test_get_time_all_scenarios.json")
json_string = json.load(json_file)["measurements"]
json_plot_gettime_all(json_string, "test_get_time_all_scenarios", warmup)

json_file = open("results/yeti/test_get_time_all_scenarios_forced.json")
json_string = json.load(json_file)["measurements"]
json_plot_gettime_all(json_string, "test_get_time_all_scenarios_forced", warmup)
