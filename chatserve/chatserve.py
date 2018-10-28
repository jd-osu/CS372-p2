# !/usr/bin/python
# mypython
# Author: 
# CS344
# February 24, 2018
# This program ...

import sys

if (len(sys.argv) < 2) : 
    print "USAGE: ", str(sys.argv[0]), " port"
    exit(1)

port = int(sys.argv[1])

print "port: ", str(port)

print "test"