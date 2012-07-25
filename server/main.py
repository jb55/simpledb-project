"""
DBServ: A simple database server agent which handles requests and returns results
"""

import socket
from client import Client
import simpledb
#test

HOST = '0.0.0.0'
PORT = 8080
ADDR = (HOST, PORT)

def main():
    """ The main thread, handles initial connections """
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ADDR = (HOST, PORT)
    try:
        sock.bind(ADDR)
    except socket.error:
        ADDR = (HOST, PORT+1)
        sock.bind(ADDR)
    sock.listen(2)
    simpledb.init("simpledb")
    print "Server listening on %s:%d" % ADDR
    while 1:
        client_sock, client_addr = sock.accept()
        print "Client connected:", client_addr
        client = Client(client_sock, client_addr)
        client.start()
        

if __name__ == '__main__':
    main()
