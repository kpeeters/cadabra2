from setuptools import setup, find_packages
from setuptools.command.install import install

from cadabra2_jupyter import __version__

class KernelInstall(install):
    """ custom installation class """
    def run(self):
        """ run override """
        install.run(self)
        # install directly into jupyter environment
        from jupyter_client.kernelspec import install_kernel_spec
        install_kernel_spec("kernelspec", kernel_name="Cadabra2")

setup(
    name="cadabra2_jupyter",
    description="Jupyter Kernel for Cadabra2",
    author="Fergus Baker",
    version=__version__,
    packages=["cadabra2_jupyter"],
    cmdclass={"install": KernelInstall},
    zip_safe=False,
)
