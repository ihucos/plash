

# fork away

# for client in socketbla:
#     fork
#     user = get unix user
#     client sends argv and session id over socket, then closes it
#     exec that plash with dtach, create a socket by the session id
#     by doing that pass plash something so he knows a user initiated this root stuff, so building is not done

import socket
import time
import os


BASE_DIR = os.environ['PLASH_DATA']


def serve_client(connection):
    data = conn.recv()
    print('got', data)
    session, plash_argv = json.dumps(data)

newpid = os.fork()
if not newpid:
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.bind(os.path.join(BASE_DIR, 'daemonsocket'))
    connection, client_address = sock.accept()
    print('got connection from', client_address)
    if not os.fork():
        serve_client(connection)
else:
    print('daemon started in background')
