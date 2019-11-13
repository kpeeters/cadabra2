import docker
import ws4py

client=docker.from_env()

def start():
    container = client.containers.run("kpeeters/cadabra2:2.2.7", detach=True, auto_remove=True)
    container.reload()
    print(container.attrs["NetworkSettings"])
    print(container.exec_run("ps auwx"))
#    container.stop(timeout=0)
    

print(client.containers.list())    
print(client.images.list())
start()
