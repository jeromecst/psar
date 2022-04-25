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

def what_layout(layout_times, value):
    idx = (np.abs(layout_times - value)).argmin()
    return layout[idx]

def format_dic(dic, layout_times, warmup):
    for exp in dic:
        list_layout = []
        exp["times_us"] = exp["times_us"][warmup:]
        for i in exp["times_us"]:
            list_layout += [what_layout(layout_times, i)]
        for l in layout:
            exp[l] = np.count_nonzero([x == l for x in list_layout]) / len(exp["times_us"])
        # delete useless fields
        for field in ["times_us","nodes","buffer_core","init_core","read_core"]:
            exp.pop(field, None)

def plot_bar(df, ax):
    x = range(len(df))
    colors = ['#33cc33', '#3366ff', '#006699', '#DB4444']
    bottom_sum = np.zeros(len(df))
    for i, tag in enumerate(layout):
        ax.bar(x, df[tag], bottom=bottom_sum, color=colors[i])
        bottom_sum += df[tag]


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
        plot_bar(df[node_read::4], ax_read)
        ax_read.set_title(f"read on node {node_read}")
        ax_read.set_xticks(xticks, xlabels)
        ax_read.set_xlabel("(pagecache node, buff node)")
        ax_read.legend(layout)

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
