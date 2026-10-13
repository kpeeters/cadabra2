
# Prerequisites

* Make sure a text-to-speech server with OpenAI API is available, e.g.

        https://github.com/remsky/Kokoro-FastAPI

  This starts with
  
        docker run -p 8880:8880 ghcr.io/remsky/kokoro-fastapi-cpu:latest
	 
* Install OBS python websocket module,

        pip install obs-websocket-py --break-system-packages
	 
* Install python-xlib, used to locate the notebook window on screen
  for highlights and subtitles (X11 only),

        sudo apt install python3-xlib
	 
* Start OBS.

	
# TOC

* Overview

  * Cadabra is a symbolic computer algebra system. This means that you
    can use it to manipulate mathematical expressions symbolically,
    just like you would do with pen-and-paper. Cadabra has a number of
    features which make it different from other computer algebra
    systems, but we will get there later. Let us just jump right in
    with some simple examples to give you a flavour first.
	
  * Here is an example of a simple quadratic polynomial:
  
       ex := (x + a) (x + b);
	  
    Everything to the right of the dot-colon symbol is a maths 
	expression. You will see later that this side of the expression
	is always in LaTeX form, which makes it easier to type things.
	The bit to the left of the dot-colon is the expression label.
	
  * You can now do things with this expression. For instance, we
    can distribute the product,
	
	   distribute(ex);
	   
    When you want to do something to an expression, you can use 
	all sorts of functions to manipulate it. The distribute function
	is one of those: it writes out the product of sums.
  
  * Cadabra's strength lies in mathematical expressions which you
    encounter in various areas of physics, such as electromagnetism,
    relativity or quantum mechanics.

* Manipulation

* Non-commuting

* Numerics and graphics

  * 
