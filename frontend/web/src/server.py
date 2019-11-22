import docker

import tornado.websocket
import tornado.ioloop
import tornado.web

class CadabraHub(tornado.web.RequestHandler):
    def get(self):
        self.render("index.html", title="Cadabra2")

class CadabraProxy(tornado.websocket.WebSocketHandler):
    def open(self):
        print("WebSocket opened")

    def on_message(self, message):
        self.write_message(u"You said: " + message)

    def on_close(self):
        print("WebSocket closed")


def start_cadabra_server():
    """ Start a Cadabra server in a docker container.
        Returns a port and authentication token.
    """
    client=docker.from_env()
    container = client.containers.run("kpeeters/cadabra2:master",
                                      ports={'32768/tcp': None},
                                      detach=True,
                                      auto_remove=True)
    container.reload()
    nws = container.attrs["NetworkSettings"]
    pfw = nws["Ports"]["32768/tcp"]
    port = pfw[0]["HostPort"]
    print("Cadabra port at", port)
    out = container.logs(stream=True)
    i=0
    tok=""
    for line in out:
        tok=line
        if i==1:
            break
        i+=1
    tok=tok.decode("utf-8") 
    tok=tok.rstrip("\n")
    print("Authentication token", tok)
    return { "port": port, "token": tok };
#    print(container.exec_run("cadabra-server 32768 0"))
#    container.stop(timeout=0)
    

#print(client.containers.list())    
#print(client.images.list())
#start()


if __name__ == "__main__":
    app =  tornado.web.Application([
        (r"/",   CadabraHub),
        (r"/ws", CadabraProxy),
        (r"/js/(.*)",
         tornado.web.StaticFileHandler,
         {'path': '../js/'})
    ])
    app.settings["template_path"]="../html"
    app.listen(8888)
    tornado.ioloop.IOLoop.current().start()
