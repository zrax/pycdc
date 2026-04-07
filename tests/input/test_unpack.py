import struct

def wtob(w):
    return struct.pack('<'+'I'*len(w), *w)

wtob([12,3])
