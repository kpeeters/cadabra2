#!/bin/sh

# amend python path
export PYTHONPATH=${PYTHONPATH}:/usr/local/lib/python3.11/dist-packages
jupyter notebook --ip=0.0.0.0 --allow-root $@
