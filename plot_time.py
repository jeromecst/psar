import argparse
import json
import matplotlib.pyplot as plt
import pandas as pd

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--measurement-index", default=0, type=int)
parser.add_argument("json_path")
args = parser.parse_args()

with open(args.json_path) as f:
    measurements = json.load(f)["measurements"]

times = measurements[args.measurement_index]["times_us"]

plt.plot(range(len(times)), times, label="read time (µs)")
plt.title("read times over time")
plt.ylim(0, max(times) + 1000)
plt.xlabel("iteration")
plt.ylabel("read time (µs)")
plt.xticks(rotation=45)
plt.legend()
plt.savefig("plot/plot_time.svg")
