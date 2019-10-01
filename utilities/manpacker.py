#! /usr/bin/python3
import struct
import sys
import os

SOURCE=sys.argv[1]
MAXLEN=int(sys.argv[2])
DEST='manufacturers.db'
if len(sys.argv) > 3:
    DEST=sys.argv[3]

def stripunicode(str_in):
    return ''.join([i if ord(i) < 128 else '.' for i in str_in])

# Header format 
#   MAXLEN          1 Byte  Max size of string
#   MANUFACTURERS   2 Bytes Total number of manufacturers
#
# If there is not Manufacturer for a given OUI, the entry is blank.
maxid=0
manufacturers=[]
with open(SOURCE) as f:
    for line in f:
        data = line.rstrip().split(None, 2)
        cid=int(data[0])
        if cid > len(manufacturers):
            manufacturers.extend([None]*(cid-len(manufacturers)+1))
        manufacturers[cid]=stripunicode(data[2])

HEADER=struct.pack("BH", MAXLEN, len(manufacturers))
ofile=open(DEST,'wb')
ofile.write(HEADER)
i=0
while i < len(manufacturers):
    #for manufacturer in manufacturers:
    if manufacturers[i]:
        ofile.seek(MAXLEN*(i+1))
        byteString = manufacturers[i][:MAXLEN].encode('ascii')
        ofile.write(byteString)
    i+=1
print("Packed %d records" % int(i))
