# databrick_live_stream.py
###################################################################################################

''' 
Use the ``bokeh serve`` command to run the application by executing:

    bokeh serve databrick_live_stream.py 

at your command prompt. Then navigate to the URL

    http://localhost:5006/databrick_live_stream

in your browser.
'''
import numpy as np

from bokeh.io import curdoc
from bokeh.layouts import row, widgetbox
from bokeh.models.widgets import Slider, TextInput
from bokeh.plotting import ColumnDataSource

import pandas as pd
import datetime
from pymongo import MongoClient
import time 
from bokeh.models import HoverTool

from bokeh.models import (
    ColumnDataSource,
    HoverTool,
    CategoricalColorMapper,
    BasicTicker,
    PrintfTickFormatter,
)
from bokeh.plotting import figure

###################################################################################################

# Connect to MongoDB server
mongo_host = 'localhost' # This is the hostname that runs the MongoDB database -- WE ASSUME IT'S ALREADY CONFIGURED!
client = MongoClient(mongo_host, 27017)
db = client.amon # This is the MongoDB database -- WE ASSUME IT'S ALREADY CONFIGURED!
collection = db.databricks # This is the Databricks collection -- WE ASSUME IT'S ALREADY CONFIGURED!

###################################################################################################

'''
RETURN A DICTIONARY OF A DATAFRAME FOR MOST RECENT DATABRICK DATA. THIS FUNCTION GETS CALLED WITHIN THE updata_data() FUNCTION, WHICH GETS CALLED EVERY 3 SECONDS.
'''

def create_source():

    # Set d1 to most 5 seconds ago to allow brick to collect the most recent databrick data
    d1 = datetime.datetime.now() - datetime.timedelta(seconds=5)
    d2 = datetime.datetime.now()
    
    # find the first record from the database between five seconds ago and now
    brick = collection.find_one({"$and": [{"timestamp": {"$gt": d1}}, {"timestamp": {"$lte": d2}}]})

    # reshape the 1 dimensional data and hitters arrays to create a matrix of 128 by 128. This allows the databrick to be visualised on a 128 x 128 heatmap.
    matrix_data = [brick['data'][x:x+128] for x in range(0, len(brick['data']), 128)]
    matrix_hitters = [brick['hitters'][x:x+128] for x in range(0, len(brick['hitters']), 128)]
    full_matrix = [matrix_data, matrix_hitters]

    data_list= []
    
    # assign x and y values to the data in the matrix, allowing for simple plotting
    for x in full_matrix[0]:
        x_value = full_matrix[0].index(x)
        start = 0
        for y in x:
            y_value = x.index(y, start)
            data_list.append((full_matrix[0][x_value][y_value], x_value, y_value, full_matrix[1][x_value][y_value]))
            start = y_value + 1

    # create dataframe from the databrick data and x and y values. Rename columns for ease of use. 
    df = pd.DataFrame(data_list)
    df.rename(columns={0: 'traffic'}, inplace=True)
    df.rename(columns={1: 'x'}, inplace=True)
    df.rename(columns={2: 'y'}, inplace=True)
    df.rename(columns={3: 'hitters'}, inplace=True)

    # add a category column to the dataframe and set the restrictions for which category each data value belongs to. This allows for coloring of the visualization based on traffic intensity.
    df.loc[df['traffic'] >= 400000, 'category'] = 10
    df.loc[df['traffic'] < 400000 , 'category'] = 5
    df.loc[df['traffic'] < 150000, 'category'] = 1
    df.loc[df['hitters'] == '', 'hitters'] = 'No Top Hitters'

    return dict(df)

###################################################################################################

# set an initial source value by calling create_source(). This will populate the visualization upon loading.
source=ColumnDataSource(data=create_source())

###################################################################################################

'''
CREATE THE VISUALIZATION
'''

# Map specific colors to each category value in the dataframe and set the tools to be suppplied with the visualization.
mapper = CategoricalColorMapper(factors=[1, 5, 10], palette=['#3a9244', '#f9cc10', '#b2282f'])
TOOLS = "hover,save,pan,box_zoom,reset,wheel_zoom, crosshair"

# create a 128 x 128 Bokeh figure entitled "Databrick." 
p = figure(title="Databrick",
           tools=TOOLS, x_range=(0, 127), y_range=(0,127), x_axis_label="source", y_axis_label="destination")

# get rid of any grid, axis, or tick_line color.
p.grid.grid_line_color = None
p.axis.axis_line_color = None
p.axis.major_tick_line_color = None

# Create 1 x 1 heatmap elements (rectangles) that visualize each piece of data in from source.data
p.rect(x="x", y="y", width=1, height=1,
       source=source,
       fill_color={'field': 'category', 'transform': mapper},
       line_color=None)

# Set the values to be displayed by the hover tool.
p.select_one(HoverTool).tooltips = [
     ('Traffic', '@traffic'),
     ('source bin', '@x'), ('dest bin', '@y'), 
     ('hitters', '@hitters')
]

###################################################################################################

'''
CREATE A SIMPLE FUNCTION TO BE CALLED EVERY THREE SECONDS THAT UPDATES THE source.data VARIABLE
'''
def update_data():

    source.data = create_source()
    
    return source.data


###################################################################################################

'''
ADD AN HTML NAVIGATION MENU TO THE APPLICATION IN ORDER TO NAVIGATE TO OTHER APPLICATIONS
'''

from bokeh.layouts import widgetbox
from bokeh.models.widgets import Div

div = Div(text="""<div>
                        <style>
                            ul{
                                padding-left: 30px;
                                padding-top: 9px;
                            }
                            li {
                                background-color: #eaeaea;
                                text-decoration: none;
                                list-style-type: none;
                                padding-top: 20px;
                                padding-bottom: 20px;
                                width: 250px;
                            }
                            a {
                                color: #008ee5;
                                padding-left: 20px;
                                text-decoration: none;
                            }
                            li:hover {

                                background-color: #d8d8d8;
                                transition: 0.1s ease;
                                width: 245px;
                                border-left: 5px solid #008ee5;
                            }
                            #current {
                                color: orange !important;
                            }
                            p {
                                padding-left: 30px;
                            }
                            h4 {
                                padding-left: 30px;
                                padding-top: 10px;
                            }
                        </style>

                        <ul>
                            <li><a href="http://localhost:5006/live_stream_traffic" target="_blank">Live Total Traffic</a></li>
                            <li><a href="http://localhost:5006/live_top_hitter" target="_blank">Live Top Hitter</a></li>
                            <li id="current"><a href="http://localhost:5006/databrick_live_stream" target="_blank">Live Databrick</a></li>
                            <li><a href="http://localhost:5006/date_test_hitters" target="_blank">Historic Databricks</a></li>
			    <li><a href="http://localhost:5006/timebrick_live_stream" target="_blank">Live Timebrick</a></li>
			    <li><a href="http://localhost:5006/timebrick_pre_load" target="_blank">Pre-loaded Timebrick</a></li>
			    <li><a href="http://localhost:5006/subset_network" target="_blank">Subset Network (umich)</a></li>
                        </ul>
                        
                        <h4>Live Databrick</h4>    
                        <p>This feature replicates the live updating of the databrick visualization that is currently the IP Pair feature of the AMON Dashboard.</p>
                    </div>""")

#################################################################################################

'''
SET UP THE SETTINGS (WHAT TO DISPLAY) FOR THE DOCUMENT AND SET THE CALLBACK TIME FOR update_data()
'''

div1 = widgetbox(div)

doc = curdoc()
doc.add_periodic_callback(update_data, 3000)
doc.add_root(row(p, div1))
doc.title = "Live Databrick"
