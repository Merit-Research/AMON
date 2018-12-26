# Bokeh Dashboard Quickstart
### Authors: 
Evan Pesch
Michalis Kallitsis
Stilian Stoev


### 1. Install Bokeh (version 1.0.2) and other necessary libraries in a Linux VM (the instructions below were tested on an Ubuntu 18.04 server)

Issue the following commands on the VM to install the necessary libraries:

    sudo apt install python-pip
    pip install bokeh==1.0.2
    pip install pandas
    pip install pymongo
    pip install flask
    pip install configparser

See instructions at https://bokeh.pydata.org/en/latest/docs/installation.html


### 2. Set up SSH tunnel:

Issue the following command on the local machine to establish an SSH tunnel to the remote host at port 5008:

    ssh -NfL localhost:5008:localhost:5008 username@hostname

Replace "username" and "hostname" with your username and the hostname of your VM.

Visit http://bokeh.pydata.org/en/latest/docs/user_guide/server.html#ssh-tunnels for help connecting the Bokeh remote host to your local host.


### 3. SSH into your VM

Issue the following command on the local machine to ssh into your VM:

    ssh username@hostname
    
Replace "username" and "hostname" with your username and the hostname of your VM.


### 4. Copy the Application to your vM

Copy the bokeh-dashboard directory to your VM to run the Flask App

    username@hostname:~$  git clone <placeholder>


### 5. Edit dashboard.ini configuration file to match you MongoDB

In bokeh-dashboard/dashboard.ini, replace the MONGO_HOST, DATABASE_NAME, and COLLECTION NAME values with the names of the your VM (where MongoDB is stored), MongoDB database (that contains the collection where your databricks are stored), and MongoDB collection (where your databricks are stored):

    [LIVE_IP_PAIR]
    MONGO_HOST = vm_hostname
    DATABASE_NAME = db_name
    COLLECTION_NAME = collection_name
    
example:

    [LIVE_IP_PAIR]
    MONGO_HOST = hostname.merit.edu
    DATABASE_NAME = amon
    COLLECTION_NAME = databricks


### 6. Run the Flask application

Issue the following command on your VM to run the Flask application:

    python bokeh-dashboard/views.py
