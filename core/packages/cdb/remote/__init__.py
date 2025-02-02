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
        self.serial = 99
        self.condition = threading.Condition()
        self.open_condition = threading.Condition()
        self.close_condition = threading.Condition()
        self.name = "cadabra2-gtk"

    def start(self, extra_args=[]):
        args = [self.name]
        args.extend(extra_args)
        self.process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            info = self.process.stderr.readline()
            while not info:
                time.sleep(0.1)
                info = self.process.stderr.readline()
            print(f"Socket at {info}")
            self.url = info[16:-1]
            print(f"URL {self.url}")
            self.connect()
            with self.open_condition:
                self.open_condition.wait()
                print("Notebook up and communicating ***")
        except Exception as ex:
            raise CadabraRemoteException(f"Failed to start cadabra2-gtk: {ex}")

    def connect(self):
        print("Starting thread...")
        self.ws_thread = threading.Thread(target=self.ws_run)
        self.ws_thread.daemon = True
        self.ws_thread.start()

    def ws_run(self):
        print("Thread started")
        print(f"Connecting to control socket {self.url}...")
        # websocket.enableTrace(True)
        self.ws = websocket.WebSocketApp(self.url,
                                         on_open = self.on_open,
                                         on_message = self.on_message,
                                         on_error = self.on_error,
                                         on_close = self.on_close)
        # self.ws.on_open = self.on_open
        print("Websocket handler going into main loop")
        self.ws.run_forever()
        print("Websocket handler exited")
        with self.close_condition:
            self.close_condition.notify()
        
    def on_message(self, ws, message):
        try:
            with self.condition:
                msg = json.loads(message)
                print(f"Received completion message {msg}")
                self.condition.notify()
        except Exception as ex:
            print(f"Cannot parse message {message}: {ex}")

    def on_open(self, ws):
        print("Connection open.")
        with self.open_condition:
            self.open_condition.notify()

    def on_close(self, ws, v1, v2):
        print(f"Connection closed: {v1}, {v2}")
        # remaining_stdout = self.process.stdout.read()
        # remaining_stderr = self.process.stderr.read()
        # print(remaining_stdout)
        # print(remaining_stderr)

    def on_error(self, ws, message):
        print(f"Connection error {message}.")
        # remaining_stdout = self.process.stdout.read()
        # remaining_stderr = self.process.stderr.read()
        # print(remaining_stdout)
        # print(remaining_stderr)
            
    def open(self, notebook, wait=True):
        self.serial += 1
        msg = { "action":   "open",
                "notebook": notebook,
                "serial":   self.serial
               }
        print("Loading notebook")
        self.ws.send( json.dumps(msg) )
        if wait:
            with self.condition:
                self.condition.wait()

    def run_all_cells(self, wait=True):
        self.serial += 1
        msg = { "action":   "run_all_cells",
                "serial":   self.serial
               }
        try:
            print("Running cells")
            self.ws.send( json.dumps(msg) )
            if wait:
                with self.condition:
                    self.condition.wait()
        except:
            raise CadabraRemoteException("Connection to Cadabra notebook not open.")

    def wait(self):
        with self.close_condition:
            try:
                self.close_condition.wait()
                if self.process:
                    self.process.terminate()
            except KeyboardInterrupt:
                if self.process:
                    self.process.terminate()
                

if __name__=="__main__":
    # This is for testing only; see for more extensive
    # usage of this module the scripts in the "tutorials"
    # folder of the Cadabra repository.
    cdb = CadabraRemote()
    cdb.start()
    cdb.open("../examples/schwarzschild.cnb")
    time.sleep(1)
    print("Notebook loaded")
    cdb.run_all_cells()
    print("Cells ran")
    print("Press ctrl-c to terminate")
    cdb.wait()
            
