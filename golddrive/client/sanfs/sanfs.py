# run process en background and detached from parent

import os
import shlex
from subprocess import Popen, PIPE

CREATE_NEW_PROCESS_GROUP = 0x00000200
DETACHED_PROCESS = 0x00000008

DIR = os.path.dirname(os.path.realpath(__file__)).replace('\\','/')
cmd = shlex.split(fr'"{DIR}/sanfs.exe" 192.168.100.201 w: -u sag -k c:/users/sant/.ssh/id_rsa-sag-golddrive')
print (cmd)
p = Popen(cmd, stdin=PIPE, stdout=PIPE, stderr=PIPE,
          creationflags=DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP)
print(p.pid)