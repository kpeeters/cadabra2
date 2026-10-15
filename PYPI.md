# Cadabra

*A field-theory motivated approach to computer algebra.*

Cadabra is a symbolic computer algebra system, designed specifically
for the solution of problems encountered in quantum and classical
field theory. It has extensive functionality for tensor computer
algebra, tensor polynomial simplification including multi-term
symmetries, fermions and anti-commuting variables, Clifford algebras
and Fierz transformations, implicit coordinate dependence, multiple
index types and many more. The input format is a subset of TeX.

This package contains the Cadabra Python module, the Cadabra Jupyter
kernel and an IPython extension. The Cadabra notebook interface
(`cadabra2-gtk`) and the command-line client are *not* included; get
those from the native packages at <https://cadabra.science/download.html>.


## Installation

```
pip install cadabra2
```

Wheels are available for Linux (x86_64 and arm64) and Python 3.10 or later.


## Using Cadabra in Jupyter

There are two ways to use Cadabra in Jupyter:

* Choose the **Cadabra2** kernel when creating a new notebook. The
  kernel is registered automatically, in whichever Python environment
  the package was installed into.

* Use an ordinary **Python** notebook and load the Cadabra extension in
  the first cell:

  ```
  %load_ext cadabra2_jupyter
  ```

  after which cells accept Cadabra notation, e.g.

  ```
  ex:=(A+B) (C+D);
  distribute(ex);
  ```

  This works in environments where you cannot install extra kernels,
  such as Google Colab, Kaggle or VS Code notebooks. In Google Colab,
  run `!pip install cadabra2` in a cell first.


## Using Cadabra from Python

The module can also be used from plain Python:

```python
import cadabra2

ex = cadabra2.Ex(r"A_{m n} + B_{m n}")
print(ex)
```

After `import cadabra2`, Cadabra notebooks (`.cnb`) and Jupyter
notebooks (`.ipynb`) written in Cadabra notation can be imported as
ordinary Python modules.


## More information

- Documentation, tutorials and reference manual: <https://cadabra.science>
- Questions and answers forum: <https://cadabra.science/qa/>
- Source code: <https://github.com/kpeeters/cadabra2>
- Bug reports: <https://github.com/kpeeters/cadabra2/issues>

If you use Cadabra in your research, please cite
K. Peeters, *Cadabra2: computer algebra for field theory revisited*,
J. Open Source Softw. 3 (2018) 1118,
[doi:10.21105/joss.01118](https://doi.org/10.21105/joss.01118).
