
CadabraWeb
==========


CadabraWeb consists of two parts: a server part written in Python, and
a client part which is mostly C++ compiled for the web using
Emscripten, with some thin Javascript glue to bind it together.

Why not JupyterLab?
-------------------

I do not agree with the philosophy behind JupyterLab. It duplicates
loads of things for which I already have perfect solutions: text
editor, terminal window, file manager, window manager. For people who
do not want to install anything except a browser on their own machine,
it is no doubt a useful tool. But for people who are not afraid to
use a reasonably standard Linux or macOS machine, JupyterLab simply
implements a lot of functionality which is already available, often in
a much better form.

So CadabraWeb takes the approach that the notebook tool should only do
that: let you interact with notebooks. Everything else, like editing
text, running commands in a terminal, managing files or windows,
belongs elsewhere.



Building the server
-------------------

# Create the docker instance

Do `make docker-prepare` to create the source tarball, `make docker`
to create the docker image.


# Create the server

The server is a simple python program which requires no separate
building, just run `src/server.py`.


Building the client
-------------------

For the client you need an installation of emscripten. Activate it by
doing::

    source emsdk_env.sh
    
in the top-level emscripten directory. After that, build the client
with::

    mkdir build
    cd build
    cmake .. \
       -DCMAKE_TOOLCHAIN_FILE=${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake
    make
    

    
    
