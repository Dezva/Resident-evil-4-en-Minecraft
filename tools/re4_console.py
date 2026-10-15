"""Bounded console output: selecting text must never block the control socket."""
import queue
import threading

class ConsoleReporter:
    def __init__(self,output=None):
        self.output=output or (lambda message:print(message,flush=True))
        self.pending=queue.Queue(maxsize=32)
        self.closed=threading.Event()
        self.dropped=0
        self.worker=threading.Thread(target=self.run,name='re4bridge-console',daemon=True)
        self.worker.start()
    def print(self,*parts,**kwargs):
        self.emit(' '.join(str(part) for part in parts))
    def emit(self,message):
        if self.closed.is_set():return
        try:self.pending.put_nowait(message)
        except queue.Full:
            try:self.pending.get_nowait();self.dropped+=1
            except queue.Empty:pass
            try:self.pending.put_nowait(message)
            except queue.Full:self.dropped+=1
    def run(self):
        while not self.closed.is_set() or not self.pending.empty():
            try:message=self.pending.get(timeout=.05)
            except queue.Empty:continue
            try:self.output(message)
            except (OSError,ValueError):pass # Console closed; game control remains independent.
    def close(self):
        self.closed.set()
        self.worker.join(.1) # A blocked Windows console cannot delay native disarming/shutdown.
