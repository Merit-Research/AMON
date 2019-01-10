from flask import Flask, render_template, jsonify, request
from bokeh.plotting import figure
from bokeh.embed import components
from bokeh.models.sources import AjaxDataSource

import numpy as np
import pandas as pd
import datetime
# from datetime import timezone # datetime.timezone was not added until Python 3.2
from pymongo import MongoClient
import time

from bokeh.io import curdoc
from bokeh.layouts import row, widgetbox
from bokeh.models.widgets import Slider, TextInput
from bokeh.plotting import ColumnDataSource
from bokeh.models import (
        ColumnDataSource,
        HoverTool,
        CategoricalColorMapper,
        BasicTicker,
        PrintfTickFormatter,
)
from bokeh.plotting import figure
from bokeh.models.widgets import Div
from configparser import ConfigParser

config = ConfigParser()
config.read('AMON/visualizations/bokeh-dashboard/dashboard.ini')

def create_source_live_ip(traffic_scale, databrick):

    # Define proper database and collection
    mongo_host = str(config['LIVE_IP_PAIR']['MONGO_HOST'])
    client = MongoClient(mongo_host, 27017)
    db_name = config['LIVE_IP_PAIR']['DATABASE_NAME']
    db = client[db_name]
    collection_name = config['LIVE_IP_PAIR']['COLLECTION_NAME']
    collection = db[collection_name]
    
    # Set brick to the most recent record in the collection
    brick = collection.find_one(sort=[('$natural', -1)])

    # reshape the 1 dimensional data and hitters arrays to create a matrix of 128 by 128. This allows the databrick to be visualised on a 128 x 128 heatmap.
    matrix_data = [brick['data'][x:x+128] for x in range(0, len(brick['data']), 128)]
    matrix_hitters = [brick['hitters'][x:x+128] for x in range(0, len(brick['hitters']), 128)]
    full_matrix = [matrix_data, matrix_hitters]

    data_list= []

    # assign x and y values to the data in the matrix, allowing for simple plotting
    startx = 0
    for x in full_matrix[0]:
        x_value = full_matrix[0].index(x, startx)
        starty = 0
        for y in x:
            y_value = x.index(y, starty)
            data_list.append((full_matrix[0][x_value][y_value], x_value, y_value, full_matrix[1][x_value][y_value]))
            starty = y_value + 1
        startx = x_value + 1

    # create dataframe from the databrick data and x and y values. Rename columns for ease of use. 
    df = pd.DataFrame(data_list)
    df.rename(columns={0: 'traffic'}, inplace=True)
    df.rename(columns={1: 'x'}, inplace=True)
    df.rename(columns={2: 'y'}, inplace=True)
    df.rename(columns={3: 'hitters'}, inplace=True)

    # add a category column to the dataframe and set the restrictions for which category each data value belongs to. This allows for coloring of the visualization based on traffic intensity.
    df.loc[df['traffic'] >= traffic_scale, 'category'] = '10'
    df.loc[df['traffic'] < traffic_scale, 'category'] = '5'
    df.loc[df['traffic'] < traffic_scale/2, 'category'] = '1'
    df.loc[df['hitters'] == '', 'hitters'] = 'No Top Hitters'

    return dict(df)


def make_ajax_plot(traffic_scale, databrick):
    traffic_scale=traffic_scale
    databrick = databrick

    source = AjaxDataSource(data_url=request.url_root + 'data/', polling_interval=3000, mode='replace')

    #source.data = dict(traffic=[], x=[], y=[], hitters=[], category=[])

    source.data = create_source_live_ip(traffic_scale, databrick)

    # Map specific colors to each category value in the dataframe and set the tools to be suppplied with the visualization.
    mapper = CategoricalColorMapper(factors=('1', '5', '10'), palette=['#3a9244', '#f9cc10', '#b2282f'])
    TOOLS = "hover,save,pan,box_zoom,reset,wheel_zoom, crosshair"

    # create a 128 x 128 Bokeh figure entitled "Databrick." 
    plot = figure(title="Databrick",
        tools=TOOLS, x_range=(0, 127), y_range=(0,127), x_axis_label="source", y_axis_label="destination")

    # get rid of any grid, axis, or tick_line color.
    plot.grid.grid_line_color = None
    plot.axis.axis_line_color = None
    plot.axis.major_tick_line_color = None

    # Create 1 x 1 heatmap elements (rectangles) that visualize each piece of data in from source.data
    plot.rect(x="x", y="y", width=1, height=1,
        source=source,
        fill_color={'field': 'category', 'transform': mapper},
        line_color=None)

    # Set the values to be displayed by the hover tool.
    plot.select_one(HoverTool).tooltips = [
        ('Traffic', '@traffic'),
        ('source bin', '@x'), ('dest bin', '@y'),
        ('hitters', '@hitters')
    ]

    script, div = components(plot)
    return script, div
