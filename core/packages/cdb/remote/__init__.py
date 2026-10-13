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
        self.received_msg = {} # last reply returned by _request
        self.replies = {}      # serial -> reply, for requests being waited on
        self.waiting = set()   # serials of requests someone is waiting on
        self.closed = False
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
        with self.condition:
            self.closed = True
            self.condition.notify_all()
        with self.close_condition:
            self.close_condition.notify()
        
    def on_message(self, ws, message):
        try:
            msg = json.loads(message)
        except Exception as ex:
            print(f"Cannot parse message {message}: {ex}")
            return
        print(f"Received completion message {msg}")
        with self.condition:
            # Store the reply rather than just notifying: a notify is lost
            # if the requester has not yet started waiting.
            serial = msg.get("serial")
            if serial in self.waiting:
                self.replies[serial] = msg
            self.condition.notify_all()

    def on_open(self, ws):
        print("Connection open.")
        with self.open_condition:
            self.is_open = True
            self.open_condition.notify()

    def on_close(self, ws, v1, v2):
        print(f"Connection closed: {v1}, {v2}")
        with self.condition:
            self.closed = True
            self.condition.notify_all()
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
            
    def _request(self, msg, wait):
        """ Send a request to the notebook. If `wait` is true, block until
        the reply with the matching serial arrives and return it."""
        with self.condition:
            self.serial += 1
            serial = self.serial
            msg["serial"] = serial
            # Register before sending, so that even an immediate reply
            # gets stored by on_message.
            if wait:
                self.waiting.add(serial)
        try:
            self.ws.send( json.dumps(msg) )
        except Exception as ex:
            with self.condition:
                self.waiting.discard(serial)
            raise CadabraRemoteException(f"Cannot send to Cadabra notebook: {ex}")
        if not wait:
            return None
        with self.condition:
            # wait_for checks the predicate before sleeping, so a reply
            # which arrived before we got here is not missed.
            self.condition.wait_for(lambda: serial in self.replies or self.closed)
            self.waiting.discard(serial)
            if serial not in self.replies:
                raise CadabraRemoteException("Connection to Cadabra notebook closed while waiting for reply.")
            self.received_msg = self.replies.pop(serial)
            return self.received_msg

    def open(self, notebook, wait=True):
        msg = { "action":   "open",
                "notebook": notebook
               }
        print("Loading notebook")
        self._request(msg, wait)

    def run_all_cells(self, wait=True):
        msg = { "action":   "run_all_cells"
               }
        print("Running cells")
        self._request(msg, wait)

    def run_cell(self, cell_id, wait=True):
        msg = { "action":   "run_cell",
                "cell_id":  cell_id
               }
        print(f"Running cell {cell_id}")
        self._request(msg, wait)

    def add_cell(self, content, cell_id=0, wait=True):
        """ Add a cell to the notebook. If `cell_id` is non-zero,
        and a cell with that id already exists, just update the content."""
        msg = { "action":   "add_cell",
                "cell_id":  cell_id,
                "content":  content
               }
        reply = self._request(msg, wait)
        if reply:
            return reply.get("cell_id", 0)
        return 0

    def insert_text(self, content, cell_id=0, wait=True):
        """ Add a text to the given cell. """
        msg = { "action":   "insert_text",
                "cell_id":  cell_id,
                "content":  content
               }
        reply = self._request(msg, wait)
        if reply:
            return reply.get("cell_id", 0)
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
            
