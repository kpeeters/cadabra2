from cdb.remote           import *
from cdb.remote.speech    import init, say
from cdb.remote.highlight import mark, subtitle

init("../tutorials/en_GB-alba-medium.onnx")

cdb = CadabraRemote()
cdb.start(["--geometry", "1920x1080"])

subtitle("Tutorial 1: the basics of Cadabra")
time.sleep(1)
subtitle()
say("In this tutorial you will learn the basics of the Cadabra computer algebra system.")
say("After you start it, you are presented with the notebook interface as you see here.")
mark("Open")
say("We will use the open button to load a sample notebook now.")
time.sleep(1)
mark()

# FIXME: open does not wait long enough
cdb.open("../examples/schwarzschild.cnb")
say("This notebook computes some properties of the Schwarzschild spacetime.")
mark("Run")
say("If you press the 'run all' button, all cells will be evaluated.")
mark()
time.sleep(1)
# FIXME: run_all does not wait for completion
cdb.run_all_cells()
time.sleep(3)
say("At the very bottom, you see the Kretschmann scalar.")

print("Press ctrl-c to terminate")
cdb.wait()
