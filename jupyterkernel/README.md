# Py Cadabra2 Jupyter Kernel

Pure-Python Jupyter Kernel for Cadabra2.

## Installation
The installation should be OS agnostic (not yet tested).

- *Requires Python 3.x*

Inclusion is triggered by the CMake flag `-DENABLE_PY_JUPYTER=true`. To only install this kernel, simply

```bash
mkdir build && cd build

cmake -DENABLE_PY_JUPYTER=true ..
cd jupyterkernel
make install
```

#### Installation details

The installation is controlled by `setup.py`, and installs directly into the Python environment. All necessary information relating to the Python environment is gathered by CMake, and used to configure

- `kernelspec/kernel.json`
- `cadabra2_jupyter/__init__.py`

The `setup.py` script is then invoked by the CMake install directive. This implicitly installs `jupyter` and `jupyter-client`, so that the kernel specification can be directly made available to Jupyter.


## Use
To use, simply start a Jupyter notebook server
```bash
jupyter notebook
```
and the kernel will appear listed.
