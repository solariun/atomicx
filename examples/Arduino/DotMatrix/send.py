# Send UDP broadcast packets

MYPORT = 2221 

from datetime import datetime
import sys, time
from socket import *

s = socket(AF_INET, SOCK_DGRAM)
s.bind(('', 0))
s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)

while 1:
    data = datetime.now().strftime("%d/%m/%y")
    s.sendto(data, ('192.168.1.255', MYPORT))
    time.sleep(5)
    data = datetime.now().strftime("%H:%M")
    s.sendto(data, ('192.168.1.255', MYPORT))
    time.sleep(5)
    
    
    
