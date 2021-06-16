#!/usr/bin/python
# shameless copy and paste from various python sources.
# this script is to be placed /usr/lib/nagios/plugins of your icinga/nagios server

import socket
import sys
import argparse
import signal

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    parser.add_argument("-H", "--hostname", help="Hostname.")
    parser.add_argument("-w", "--warning", help="Warning threshold.")
    parser.add_argument("-c", "--critical", help="Critical threshold.")

    args = parser.parse_args()


if not args.hostname:
	print >>sys.stderr, 'missing hostname'
	sys.exit(3)

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# Connect the socket to the port where the server is listening
server_address = (args.hostname, 80)
sock.connect(server_address)


# Send data
message = '{getTemp:}'
sock.sendall(message)

# Look for the response
amount_received = 0
amount_expected = len(message)
    
data = sock.recv(16)
sock.close()

if args.critical:
     if float(data) > float(args.critical):
        print('CRITICAL - temp:'"%s" % data)
        sys.exit(2) # Critical

if args.warning:
     if float(data) > float(args.warning):
	print('WARNING - temp:'"%s" % data)
        sys.exit(1) # Warning

if float(data) < 10.0:
     print('WARNING - temp too low:'"%s" % data)
     sys.exit(1)

print('OK - temp:'"%s" % data)
sys.exit(0) # OK
