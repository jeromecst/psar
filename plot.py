import json
import os
import matplotlib.pyplot as plt

directory = 'results/'

def plottime(rtime, cputime, method, name):
    fig, ax = plt.subplots(2, 1, figsize=(15,12))
    if method == plt.boxplot:
        ax[0].boxplot(rtime)
        ax[1].boxplot(cputime)
    else:
        ax[0].violinplot(rtime)
        ax[1].violinplot(cputime)
    for i, time in enumerate(["real time", "cpu time"]):
        ax[i].set_title(f"{time} for each numa")
        ax[i].set_xlabel("numa node")
        ax[i].set_ylabel(f"{time} (ms)")
    fig.savefig(name + "_rtime.png")

def json_plot(json_dic, name):
    # 2D array containing time[] for each numa node
    # len(time) should be equal to number of numa node
    r_time = [i["r_time"] for i in json_string]
    cpu_time = [i["cpu_time"] for i in json_string]
    plottime(r_time, cpu_time, plt.violinplot, name + "_violin")
    plottime(r_time, cpu_time, plt.boxplot, name + "_bar")


for f in ["yeti/test_result.json", "yeti/test2_result.json"]:
    path = directory + f
    if os.path.isfile(path):
        json_file = open(path)
        json_string = json.load(json_file)
        json_plot(json_string, "plot/" + str(f))
        json_file.close()
