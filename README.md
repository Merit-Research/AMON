All-packet MONitor
------------------

Description
-------------
AMON is a software tool for processing multi-10Gbps streams of network data.
It is based on PF-RING (zero-copy).
This version of AMON reads packets directly from the network interface (NIC),
using the PF\_RING API. It then generates and streams (to a centralized MongoDB
database) the following data output (for more details, see [1]):
1. A 128x128 matrix (aka "databrick") that conveys information about the network's traffic *intensity* and *structure* 
2. A 128x128 matrix that conveys information about the network heavy-hitters. These hitters are identified with the help of the MJRTY Boyer-Moore algorithm. 

Installation
-------------
AMON is currenly supported on Ubuntu and CentoOS systems. So, the first step, is to install one of these on the server that will be receiving the traffic :)

Overall, AMON can be easily built from source, once these two prerequisites are installed:
1. PF\_RING: we recommend installing PF\_RING as a binary. Great instructions can be found here:  
http://packages.ntop.org (NOTE: if you want to build PF\_RING from source, and then compile AMON, please drop as a line (see "Contact Us" section below).)
2. MongoDB C drivers: see README.mongo

Usage
------
1. Populate accordingly the fields in *amon.config*
2. Type './amon -i eth0' where 'eth0' is the interface you are receiving network traffic from
3. Populate the "strata.txt" file (optional). This is for reserving specific "bins" (e.g, the first k=5 leftmost bins) for subnets of interest (e.g., Google, Apple, etc.)

To verify that things work, check that you get updated traffic statistics (provided by PF\_RING) every 1 second, and that you get a list of the top-hitters every 
ALARM\_SLEEP seconds (parameter set in amon.config -- we recommend setting this to values less than or equal to 10 seconds)

Contact Us
----------

Please email research@merit.edu for support or contact mgkallit AT umich

References
-------------

[1]  M. Kallitsis, S. Stoev, S. Bhattacharya, G. Michailidis, AMON: An Open Source Architecture for Online Monitoring, Statistical Analysis and Forensics of Multi-gigabit Streams, IEEE JSAC Special Issue on Measuring and Troubleshooting the Internet, July 2016. [Online] http://ieeexplore.ieee.org/document/7460178/
