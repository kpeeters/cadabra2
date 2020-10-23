from ipykernel.kernelapp import IPKernelApp
from cadabra2_jupyter.kernel import CadabraJupyterKernel

if __name__ == "__main__":
    IPKernelApp.launch_instance(kernel_class=CadabraJupyterKernel)
