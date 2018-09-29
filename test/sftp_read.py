#!/usr/bin/python

"""Example script for SFTP read"""

import socket
import os
import sys
from datetime import datetime

from ssh2.session import Session
from ssh2.sftp import LIBSSH2_FXF_READ, LIBSSH2_SFTP_S_IRUSR
import ssh2

def main():
    user = 'sag'
    host = '192.168.100.201'
    # publickey = os.path.expanduser('~\\.ssh\\id_rsa-sag-golddrive.pub')
    privatekey = os.path.expanduser('~\\.ssh\\id_rsa-sag-golddrive')
    if not os.path.isfile(privatekey):
        print("No such private key %s" % (privatekey,))
    if not os.path.isfile(privatekey):
        print("No such public key %s" % (publickey,))
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, 22))
    s = Session()
    s.handshake(sock)
    s.userauth_publickey_fromfile(user, privatekey)   
    sftp = s.sftp_init()
    now = datetime.now()
    print("Starting read for remote file...")
    with sftp.open('/tmp/file.bin', 
        LIBSSH2_FXF_READ, LIBSSH2_SFTP_S_IRUSR) as r, \
        open('file.bin','wb') as w:
        for size, data in r:
            w.write(data)
    print("Finished file read in %s" % (datetime.now() - now,))


if __name__ == "__main__":
    main()