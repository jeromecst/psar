import json
import os
import matplotlib.pyplot as plt

directory = 'results/'

def plottime(rtime, cputime, name):
    fig, ax = plt.subplots(2, 1, figsize=(10,10))
    ax[0].boxplot(rtime)
    ax[1].violinplot(rtime)
    for i, method in enumerate(["boxplot", "violinplot"]):
        ax[i].set_title(f"read real time for each numa node ({method})")
        ax[i].set_xlabel("numa node")
        ax[i].set_ylabel("time ($\mu s$)")
    fig.savefig(name + ".png")

def json_plot(json_dic, name, warmup):
    # 2D array containing time[] for each numa node
    # len(time) should be equal to number of numa node
    r_time = [i["times_us"][warmup:] for i in json_dic]
    plottime(r_time, r_time, name)


# for f in ["yeti/test_result.json", "yeti/test2_result.json"]:
for f in ["yeti/test1.json", "yeti/test2.json"]:
    path = directory + f
    if os.path.isfile(path):
        json_file = open(path)
        json_string = json.load(json_file)
        json_plot(json_string["measurements"], "plot/" + str(f), warmup = 50)
        json_file.close()
