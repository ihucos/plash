
BASE_DIR = os.environ['PLASH_DATA']

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect(os.path.join(BASE_DIR, 'daemonsocket'))
mysessionid = 'myrandomseasonidjaldskf'
sock.send(json.dumps([mysessionid, sys.argv[1:]]))

def usr1handler():
    exec dtach -blah thecreateddtachsocket
