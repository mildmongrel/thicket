#
# Thicket transmit/receive helper routines
#

import socket
import struct
import zlib
import messages_pb2
from google.protobuf import message as protobuf_message

# Takes a ClientToServerMsg
def send_msg(sock, msg):
    payload = msg.SerializeToString()
    # Prefix each message with a 2-byte length (network byte order)
    data = struct.pack('!H', len(payload)) + payload
    sock.sendall(data)

# Returns a ServerToClientMsg
def recv_msg( sock ):
    # Read message length and unpack it into an integer
    raw_header = recvall( sock, 2 )
    header = struct.unpack( '!H', raw_header )[0]
    payload_compressed_flag = (header & 0x8000) != 0
    payload_len = header & 0x7FFF
    print( "len={}, compressed={}".format( payload_len, payload_compressed_flag ) )

    # Read the message data, decompressing if necessary
    if payload_compressed_flag:
        recvall( sock, 4 )  # skip the length bytes added by qCompress
        compressed_payload = recvall( sock, payload_len - 4 )
        payload = zlib.decompress( compressed_payload )
    else:
        payload = recvall( sock, payload_len )

    # Decode the message
    msg = messages_pb2.ServerToClientMsg()
    try:
        msg.ParseFromString( payload )
    except protobuf_message.DecodeError:
        print( "error decoding message!" )
        return None

    return msg

def recvall(sock, n):
    # Helper function to recv n bytes or return None if EOF is hit
    data = ''
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if packet:
            data += packet
    return data


