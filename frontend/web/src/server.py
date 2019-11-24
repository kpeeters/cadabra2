import docker
import tornado.websocket
import tornado.ioloop
import tornado.web
import json


class CadabraHub(tornado.web.RequestHandler):
    def get(self):
        self.render("index.html", title="Cadabra2")


class CadabraProxy(tornado.websocket.WebSocketHandler):
    def open(self, *args, **kwargs):
        print("WebSocket opened")
        res = self.start_cadabra_server()
        print(res)
        self.cdb_data=res
        msg={"header": {"msg_type": "hello", "token": res["token"]}}
        self.write_message(json.dumps(msg))
        url = "ws://localhost:"+str(res["port"])
        print(url)
        self.cdb_data["conn_fut"] = tornado.websocket.websocket_connect(url, on_message_callback=self.on_cdb_message)
        self.cdb_data["conn_fut"].add_done_callback(self.on_cdb_connected)

    # Handler for messages which come in from the browser app and need to be
    # forwarded to the kernel.
    
    def on_message(self, message):
        print("message from "+str(self.cdb_data))
        print(message)
#        self.write_message(u"You said: " + message)
        if "conn" in self.cdb_data:
            self.cdb_data["conn"].write_message(message)
        else:
            print("Received before kernel connected")

    def on_close(self):
        print("WebSocket for "+str(self.cdb_data)+" closed")
        self.cdb_data["container"].stop()

    def on_cdb_connected(self, fut):
        print("Websocket to kernel connected")
        conn = fut.result()
        self.cdb_data["conn"]=conn
#         req = {}
#         header = {}
#         content = {}
#         header["uuid"]="none"
#         header["cell_id"]=1
#         header["cell_origin"]="client"
#         header["msg_type"]="execute_request"
#         content["code"]="ex:= A_{m n};"
#         req["auth_token"]=self.cdb_data["token"]
#         req["header"]=header
#         req["content"]=content
#         conn.write_message(json.dumps(req))
        
    def on_cdb_message(self, msg):
        #print("Received message from cdb kernel")
        #print(str(msg))
        if msg!=None:
            # Forward to the browser.
            # First change any header/cell_id to a string to prevent
            # Javascript from rounding it...
            jmsg=json.loads(msg)
            jmsg["header"]["cell_id"]=str(jmsg["header"]["cell_id"])
            self.write_message(json.dumps(jmsg))
        
    def start_cadabra_server(self):
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
        return { "port": port, "token": tok, "container": container };


#print(client.containers.list())    
#print(client.images.list())
#start()


if __name__ == "__main__":
    app =  tornado.web.Application([
        (r"/",   CadabraHub),
        (r"/ws", CadabraProxy),
        (r"/js/(.*)",
         tornado.web.StaticFileHandler,
         {'path': '../js/'}),
        (r"/css/(.*)",
         tornado.web.StaticFileHandler,
         {'path': '../css/'})
    ])
    app.settings["template_path"]="../html"
    app.listen(8888)
    tornado.ioloop.IOLoop.current().start()
