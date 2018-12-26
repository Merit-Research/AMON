# views.py
# Author: Evan Pesch
# Last Updated: December 11, 2018

# General Imports
from flask import Flask, render_template, jsonify, request
import collections
import numpy as np
import pandas as pd
import datetime
from datetime import timedelta
from pymongo import MongoClient
import time
import re
import socket

# Bokeh Imports
from bokeh.embed import components
from bokeh.io import curdoc
from bokeh.layouts import row, widgetbox, column, layout
from bokeh.models.widgets import Slider, TextInput, DataTable, TableColumn, Button
from bokeh.plotting import ColumnDataSource
from bokeh.models.sources import AjaxDataSource
from bokeh.models import (
        ColumnDataSource,
        HoverTool,
        CategoricalColorMapper,
        BasicTicker,
        PrintfTickFormatter)
from bokeh.plotting import figure
from bokeh.layouts import widgetbox
from bokeh.models.widgets import Div
from bokeh.models import OpenURL



app = Flask(__name__)

from live_ip_pair import create_source_live_ip, make_ajax_plot

@app.route('/', methods=['GET', 'POST'])
def live_ip_pair():

    global traffic_scale
    traffic_scale = request.form.get("traffic_scale", False)

    if traffic_scale == False:
        traffic_scale = 1000000
    else:
        traffic_scale = int(traffic_scale) 

    global databrick
    databrick = request.form.get("databrick", False)
    if databrick == False:
        databrick = "equinix"
    else:
        databrick = str(databrick)

    plots = []
    plots.append(make_ajax_plot(traffic_scale, databrick))

    return render_template('live_ip_pair.html', plots=plots, traffic_scale=traffic_scale, databrick=databrick)

@app.route('/data/', methods=['POST'])
def data_live_ip():

    new_source = create_source_live_ip(traffic_scale, databrick)
    traffic = list(new_source['traffic'])
    traffic = [int(v) for v in traffic]
    x = list(new_source['x'])
    x = [int(v) for v in x]
    y = list(new_source['y'])
    y = [int(v) for v in y]
    hitters=list(new_source['hitters'])
    category = list(new_source['category'])
    category = [int(v) for v in category]
    return jsonify(traffic=traffic, x=x, y=y, hitters=hitters, category=category)


# With debug=True, Flask server will auto-reload
# when there are code changes
if __name__ == '__main__':
    app.run(port=5008, debug=True)
