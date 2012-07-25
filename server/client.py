"""
Client - threaded client objects, handles requests from connected clients
"""

import threading
import simpledb
from util import argsplit
from protocol import DBProtocol
BUFSIZ = 1024

class Client(threading.Thread):
    """ Spawned for each new client """
    def __init__(self, socket, addr):
        threading.Thread.__init__(self, name=' '.join(("Client:", repr(addr))))
        self.sock = socket
        self.addr = addr
        self.init = False
        self.protocol = DBProtocol(self)

    def run(self):
        """ Catches client requests """
        while 1:
            data = self.sock.recv(BUFSIZ)
            if not data:
                break
            args = argsplit(data)
            args = [arg.replace('\r\n', '') for arg in args]

            if len(args) == 0:
                continue
            self.handle_command(args)

        self.sock.close()
        print "Client disconnected: %s" % repr(self.addr)

    def handle_command(self, args):
        cmd = ''.join(['cmd_', args[0]])
        try:
            cmd_func = getattr(self.protocol, cmd)
            if len(args) > 1:
                cmd_func(args[1:])
            else:
                cmd_func(None)
        except AttributeError:
            self.protocol.request_fail("Invalid command: `%s`" %args[0] )

