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

def format_dic(dic, layout_times):
    for exp in dic:
        list_layout = []
        for i in exp["times_us"]:
            list_layout += [what_layout(layout_times, i)]
        for l in layout:
            exp[l] = np.count_nonzero([x == l for x in list_layout])
        # delete useless fields
        for field in ["times_us","nodes","buffer_core","init_core","read_core"]:
            exp.pop(field, None)

def plot_bar(df, _from, _to, ax):
    x = range(_to - _from)
    colors = ['#1D2F6F', '#8390FA', '#6EAF46', '#FAC748']
    bottom_sum = np.zeros(_to - _from)
    for i, tag in enumerate(layout):
        ax.bar(x, df[tag][_from:_to], bottom=bottom_sum)
        bottom_sum += df[tag][_from:_to]



def json_plot_gettime_all(json_dic, name, warmup):
    layout_times = get_layout_times()
    format_dic(json_dic, layout_times)

    print(layout_times)
    df = pandas.DataFrame(json_dic)
    pandas.set_option("display.max_rows", 65, "display.max_columns", None)
    print(df)
    
    fig, ax = plt.subplots(4, 1, figsize=(12, 20))

    for i in range(4):
        plot_bar(df, i*16, (i+1)*16, ax[i])

    fig.savefig(name + ".png")

warmup = 20
points_to_plot = 250
layout = ["ll", "ld", "dl", "dd"]

json_file = open("results/yeti/test_get_time_all_scenarios.json")
json_string = json.load(json_file)["measurements"]
json_plot_gettime_all(json_string, "test_get_time_all_scenarios", warmup)
