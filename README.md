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


References
-------------

[1]  M. Kallitsis, S. Stoev, S. Bhattacharya, G. Michailidis, AMON: An Open Source Architecture for Online Monitoring, Statistical Analysis and Forensics of Multi-gigabit Streams, IEEE JSAC Special Issue on Measuring and Troubleshooting the Internet, July 2016. [Online] http://ieeexplore.ieee.org/document/7460178/
