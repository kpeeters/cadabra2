from cadabra import *
e=Ex("hi")
try:
    e.kasper()
except ParseException, e:
    print "exception"

