#!/usr/bin/env python3

from cdb.remote           import *
from cdb.remote.speech    import init, say, warp
from cdb.remote.highlight import init_highlight, mark, subtitle
from cdb.remote.record    import *
import sys

if len(sys.argv)<2:
    print("Usage: 01_basics.py [OBS password]", file=sys.stderr)
    sys.exit(-1)

password = sys.argv[1]    

try:
   init()
   
   cdb = CadabraRemote()
   cdb.start(["--geometry", "1920x1080", "--title", "Cadabra tutorial 1"])
   time.sleep(1)
   
   setup_region_capture("Cadabra tutorial 1", password)
   start_record()
   
   init_highlight("Cadabra tutorial 1")
   
   #warp(True)
   subtitle("Tutorial 1: the basics of Cadabra", large=True)
   time.sleep(4)
   subtitle()

   cdb.insert_text("ex:= \cos(x)**2 + \sin(x)**2;")
   
   say("In this tutorial, you will learn the basics of the Cadabra computer-algebra system.")
   say("You will get a flavour of how you can setup computations with it, and how you can manipulate maths expressions.")
   say("After you start it, you are presented with the notebook interface as you see here.", block=True)
   time.sleep(1)
   say("The top of the screen has the menu bar and some control buttons.", block=True)
   time.sleep(0.5)
   mark("Open")
   time.sleep(0.5)
   mark()
   mark("Save")
   time.sleep(0.5)
   mark()
   mark("Save as")
   time.sleep(0.5)
   mark()
   mark("Run")
   time.sleep(0.5)
   mark()
   mark("Stop")
   time.sleep(0.5)
   mark()
   mark("Restart kernel")
   time.sleep(0.5)
   mark()
   
   subtitle()
   time.sleep(1)
   say("The bottom of the screen shows status information.", block=True)
   time.sleep(1)
   subtitle()
   time.sleep(1)
   say("Finally, the blank canvas in the middle is where our computations will go. Right now, it has a single empty cell.", block=True)
   time.sleep(0.5)
   subtitle()
   time.sleep(1)
   
   say("Let's add a maths expression in this first cell.", block=True)
   time.sleep(1)
   subtitle()
   time.sleep(1)
   cell_id = cdb.add_cell(r"ex := x \cos(x);")
   time.sleep(2)
   
   say("Mathematical expressions are entered using the colon-equals notation.",
       subtext="Mathematical expressions are entered using the ':=' notation.")
   say("The left-hand side is the 'name' of the expression.")
   say("The right-hand side is 'maths', and written using lay tech-notation.",
       subtext="The right-hand side is maths, and written using LaTeX notation.", block=True)
   say("You do not need to 'declare' variables in maths expressions; just write them.", block=True)
   say("If you end the expression with a semi-colon, like here, it will be displayed.", block=True)
   
   mark("Run")
   say("You can run the whole notebook using the 'run' button, or run a single cell by pressing shift-enter.", block=True)
   time.sleep(1)
   cdb.run_all_cells()
   mark()
   
   say("This will show the expression properly typeset.")
   say("You can manipulate it by using a wide variety of 'algorithms'.", block=True)
   
   cell_id = cdb.add_cell(r"substitute( ex, $x -> 2 y + \pi$ );")
   say("Here we are replacing the 'x' variable with '2 y plus pi'.")
   #warp(False)
   say("Note how we used an 'inline' maths expression, using laytech dollar notation.",
       subtext="Note how we used an 'inline' maths expression, using LaTeX dollar notation.", block=True)
   cdb.run_cell(cell_id)
   say("Again, the semi-colon at the end has resulted in Cadabra displaying the result.", block=True);
   #warp()
   subtitle()

   time.sleep(1)
   say("You will notice that Cadabra has only done the substitution.", block=True)
   say("In general, Cadabra behaves as a scratch pad: it does not try to be clever about rewriting expressions, unless you tell it to.", block=True)
   say("Of course, we can for instance tell it to distribute the product.", block=True)
   cell_id = cdb.add_cell("distribute( ex );")
   
#    time.sleep(2)
#    say("You can plot expressions like these, by importing the plotting functionality from a package.", block=True)
#    time.sleep(2)
#    cdb.add_cell("from cdb.graphics.plot import *\nplot(ex, ($y$,0,10));");
#    time.sleep(2)
#    subtitle()
#    cdb.run_all_cells()
#    time.sleep(2)
#    say("Documentation for all packages is available from the web site.", block=True)
#    time.sleep(2)

   

   subtitle()
   mark()
   
   stop_record()
   cdb.wait()

except Exception as ex:
   print(f"Error: {ex}")
   cdb.process.terminate()

