#!/usr/bin/env python3

import json
import sys
import subprocess
import websocket
import threading
import time

class CadabraRemoteException(Exception):
    def __init__(self, msg):
        pass
        
class CadabraRemote:
    def __init__(self):
        self.url = ""
        self.process = None
        self.ws = None
        self.ws_thread = None

    def start(self):
        self.process = subprocess.Popen(["cadabra2-gtk"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            info = self.process.stderr.readline()
            while not info:
                info = self.process.stderr.readline()
            print(f"Socket at {info}")
            self.url = info[16:-1]
            print(f"URL {self.url}")
            self.connect()
        except Exception as ex:
            raise CadabraRemoteException(f"Failed to start cadabra2-gtk: {ex}")

    def connect(self):
        print(f"Connecting to control socket {self.url}...")
        self.ws = websocket.WebSocketApp(self.url,
                                         on_message = self.on_message,
                                         on_error = self.on_error,
                                         on_close = self.on_close)
        self.ws.on_open = self.on_open
        self.ws_thread = threading.Thread(target=self.ws_run)
        self.ws_thread.start()

    def ws_run(self):
        print("Thread started")
        self.ws.run_forever()
        
    def on_message(self, ws, message):
        print(f"Received {message}")

    def on_open(self, ws):
        print("Connection open.")

    def on_close(self, ws, v1, v2):
        print(f"Connection closed: {v1}, {v2}")

    def on_error(self, ws, message):
        print(f"Connection error {message}.")
        remaining_stdout = self.process.stdout.read()
        remaining_stderr = self.process.stderr.read()
        print(remaining_stdout)
        print(remaining_stderr)
            
    def open(self, notebook):
        msg = { "action": "open",
                "notebook": notebook
               }
        self.ws.send( json.dumps(msg) )

    def run_all_cells(self):
        msg = { "action": "run_all_cells" }
        try:
            self.ws.send( json.dumps(msg) )
        except:
            raise CadabraRemoteException("Connection to Cadabra notebook not open.")
    

if __name__=="__main__":
    cdb = CadabraRemote()
    cdb.start()
    time.sleep(2)
    # cdb.open("../examples/schwarzschild.cnb")
    cdb.run_all_cells()
    
    print("Exiting...")
    
