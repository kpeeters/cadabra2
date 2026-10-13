#!/usr/bin/env python3

from cdb.remote           import *
from cdb.remote.speech    import init, say, warp
from cdb.remote.highlight import init_highlight, mark, subtitle, highlighter
from cdb.remote.record    import *
import sys

if len(sys.argv)<2:
    print("Usage: 04_basics.py [OBS password]", file=sys.stderr)
    sys.exit(-1)

password = sys.argv[1]    

try:
   init()
   
   cdb = CadabraRemote()
   cdb.start(["--geometry", "1920x1080", "--title", "Cadabra tutorial 4"])
   time.sleep(1)
   
   setup_region_capture("Cadabra tutorial 4", password)
   start_record()
   
   init_highlight("Cadabra tutorial 4")
   
   #warp(True)
   subtitle("Tutorial 4: Plotting", large=True)
   time.sleep(4)
   subtitle()
   
   say("In this tutorial, you will learn how to use Cadabra to make plots of functions, solutions to differential equations, and other things you might want to visualise.")
   say("Let us start by explaining how to make a simple plot of a function of one variable.")
   say("All the plotting facilities are contained in a Cadabra package, so let's import this.", block=True)
   cell_id = cdb.add_cell(r"from cdb.graphics.plot import *")
   time.sleep(2)
   cdb.run_cell(cell_id)
   time.sleep(1)
   say("Here is the function we would like to plot.", block=True)
   cell_id1 = cdb.add_cell(r"f:= x**2 \sin(x);")
   time.sleep(2)
   cdb.run_cell(cell_id1)
   say("Now we can plot it as follows.", block=True)
   time.sleep(1)
   cell_id = cdb.add_cell(r"plot(f, {$x$: (0, 10)} );")
   time.sleep(2)
   cdb.run_cell(cell_id)
   say("Pay special attention to how we have set the range of the x-variable.", block=True)
   time.sleep(1)
   say("In Cadabra, just feeding the function into the plot command is not the best approach.", block=True)
   say("Better is to give the plot command an equation, which defines the name of the function.", block=True)
   time.sleep(1)
   subtitle()
   cell_id = cdb.add_cell(r"ex:= f(x) = x**2 \sin(x);", cell_id=cell_id1)
   time.sleep(2)
   cdb.run_cell(cell_id);
   time.sleep(1)
   cell_id = cdb.add_cell(r"plot(ex, {$x$: (0,10)} );")
   cdb.run_cell(cell_id);
   time.sleep(1)
   say("You see that the plot now automatically contains a legend.", block=True)

   time.sleep(5)
   
   subtitle()
   mark()
   
   stop_record()
   cdb.wait()

except Exception as ex:
   print(f"Error: {ex}")
   stop_record()
   cdb.process.terminate()

