import sys
import os
sys.path.append(os.getcwd())
import cadabra2
import imp; 
f=open(imp.find_module('cadabra2_defaults')[1]); 
code=compile(f.read(), 'cadabra2_defaults.py', 'exec'); 
exec(code); 
f.close()