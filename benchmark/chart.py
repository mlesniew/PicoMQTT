#!/usr/bin/env python3
import csv
import sys

import pygal

data = list(csv.DictReader(sys.stdin, dialect="excel-tab"))
X = "message size"
SERIES = [e for e in data[0].keys() if e and e != X]

chart = pygal.XY(
    legend_at_bottom=True,
    logarithmic=True,
    x_title="payload size [B]",
    y_title="messages delivery rate [1/s]",
    pretty_print=True,
    stroke_style={"width": 50},
    x_label_rotation=-90,
)

for name in SERIES:
    chart.add(name, [(float(e[X]), float(e[name])) for e in data])

sys.stdout.write(chart.render().decode("utf-8"))
