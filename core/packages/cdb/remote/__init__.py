#!/usr/bin/env python3

import json
import sys
import subprocess
import websocket
import threading
import time
import re

class CadabraRemoteException(Exception):
    def __init__(self, msg):
        super().__init__(msg)
        
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
        self.received_msg = {} # FIXME: need to keep a stack of messages?
        self.stderr_thread = None
        self.is_open = False
        self.open_error = None
        self.verbose = False # echo the notebook's stderr output

    def start(self, extra_args=[], timeout=30):
        args = [self.name]
        args.extend(extra_args)
        try:
            self.process = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        except Exception as ex:
            raise CadabraRemoteException(f"Failed to start {self.name}: {ex}")
        # The notebook prints 'control channel ws://...' on stderr, but
        # GTK/GLib may print other messages before that, so scan for it.
        preamble = []
        while True:
            info = self.process.stderr.readline()
            if not info:
                code = self.process.wait()
                raise CadabraRemoteException(f"{self.name} exited with code {code} before "
                                             f"announcing control channel; stderr: {''.join(preamble)}")
            m = re.search(r'control channel (ws://\S+)', info)
            if m:
                break
            preamble.append(info)
        for line in preamble:
            print(f"{self.name}: {line}", end="")
        self.url = m.group(1)
        print(f"URL {self.url}")
        # Keep draining stderr, otherwise the pipe fills up and the
        # notebook blocks on its next write.
        self.stderr_thread = threading.Thread(target=self.drain_stderr)
        self.stderr_thread.daemon = True
        self.stderr_thread.start()
        with self.open_condition:
            self.connect()
            if not self.open_condition.wait_for(lambda: self.is_open or self.open_error is not None, timeout):
                raise CadabraRemoteException(f"Timeout connecting to control channel {self.url}")
            if not self.is_open:
                raise CadabraRemoteException(f"Failed to connect to control channel {self.url}: {self.open_error}")
            print("Notebook up and communicating ***")

    def drain_stderr(self):
        for line in self.process.stderr:
            if self.verbose:
                print(f"{self.name}: {line}", end="")

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
                self.received_msg = json.loads(message)
                print(f"Received completion message {self.received_msg}")
                self.condition.notify()
        except Exception as ex:
            print(f"Cannot parse message {message}: {ex}")

    def on_open(self, ws):
        print("Connection open.")
        with self.open_condition:
            self.is_open = True
            self.open_condition.notify()

    def on_close(self, ws, v1, v2):
        print(f"Connection closed: {v1}, {v2}")
        # remaining_stdout = self.process.stdout.read()
        # remaining_stderr = self.process.stderr.read()
        # print(remaining_stdout)
        # print(remaining_stderr)

    def on_error(self, ws, message):
        print(f"Connection error {message}.")
        with self.open_condition:
            if not self.is_open:
                self.open_error = message
                self.open_condition.notify()
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

    def run_cell(self, cell_id, wait=True):
        self.serial += 1
        msg = { "action":   "run_cell",
                "cell_id":  cell_id,
                "serial":   self.serial
               }
        try:
            print(f"Running cell {cell_id}")
            self.ws.send( json.dumps(msg) )
            if wait:
                with self.condition:
                    self.condition.wait()
        except:
            raise CadabraRemoteException("Connection to Cadabra notebook not open.")

    def add_cell(self, content, cell_id=0, wait=True):
        """ Add a cell to the notebook. If `cell_id` is non-zero,
        and a cell with that id already exists, just update the content."""
        self.serial += 1
        msg = { "action":   "add_cell",
                "cell_id":  cell_id,
                "serial":   self.serial,
                "content":  content
               }
        self.ws.send( json.dumps(msg) )
        if wait:
            with self.condition:
                self.condition.wait()
            return self.received_msg["cell_id"]
        return 0

    def insert_text(self, content, cell_id=0, wait=True):
        """ Add a text to the given cell. """
        self.serial += 1
        msg = { "action":   "insert_text",
                "cell_id":  cell_id,
                "serial":   self.serial,
                "content":  content
               }
        self.ws.send( json.dumps(msg) )
        if wait:
            with self.condition:
                self.condition.wait()
            return self.received_msg.get("cell_id", 0)
        return 0

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
            
