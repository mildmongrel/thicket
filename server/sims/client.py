import socket
import time
import random
import string
import messages_pb2
from threading import Thread
from google.protobuf import message as protobuf_message

import txrx

def send_keep_alive_ind( sock ):
    msg = messages_pb2.ClientToServerMsg()
    msg.keep_alive_ind.SetInParent()  # this is how to set an empty message
    txrx.send_msg( sock, msg )

def send_server_chat_message_ind( sock, text ):
    msg = messages_pb2.ClientToServerMsg()
    msg.chat_message_ind.scope = messages_pb2.CHAT_SCOPE_ALL
    msg.chat_message_ind.text = text
    txrx.send_msg( sock, msg )

def send_login_req( sock, name ):
    msg = messages_pb2.ClientToServerMsg()
    msg.login_req.name = name
    txrx.send_msg( sock, msg )

def send_join_room_req( sock, room_id ):
    msg = messages_pb2.ClientToServerMsg()
    msg.join_room_req.room_id = room_id
    txrx.send_msg( sock, msg )

def send_player_ready_ind( sock, ready ):
    msg = messages_pb2.ClientToServerMsg()
    msg.player_ready_ind.ready = ready
    txrx.send_msg( sock, msg )

def send_player_card_selection_req( sock, pack_id, card ):
    msg = messages_pb2.ClientToServerMsg()
    msg.player_card_selection_req.pack_id = pack_id
    msg.player_card_selection_req.card.name = card.name
    msg.player_card_selection_req.card.set_code = card.set_code
    txrx.send_msg( sock, msg )

def random_lowercase_str( size ):
    return ''.join(random.choice(string.ascii_lowercase) for _ in range(size));


# client state variables
greeting_received = False
logged_in = False
rooms_updated = False
join_rsp_received = False
create_rsp_received = False

class Room:
    def __init__(self, id):
        self.id = id
        self.chair_count = 0
        self.player_count = 0
    def __str__(self):
        return "id={}: players={}/{}".format( self.id, self.player_count, self.chair_count )

rooms = {}


def action_loop():
    global s, greeting_received, logged_in, rooms_updated, join_rsp_received, create_rsp_received
    global rooms
    tick = 0
    login_sent = False
    join_sent = False
    create_sent = False
    in_room = False
    while True:
        time.sleep(1)
        tick += 1

        # Send login req if greeting received and login not already sent
        if greeting_received and not login_sent:
            print "sending login_req"
            send_login_req( s, "sim_" + "0000" )
            login_sent = True

        if logged_in:

            # Send keepalives every 25 seconds if logged in
            if (tick % 25) == 0:
                print "sending keep_alive"
                send_keep_alive_ind( s )

            # 5% chance of sending server chat message once logged in
            if random.random() < 0.05:
                print "sending random chat msg"
                n = int( random.random() * 100 ) + 1;
                send_server_chat_message_ind( s, random_lowercase_str(n) )

            # If rooms have been updated and we haven't tried to get into
            # room yet, join or create a room.
            if rooms_updated and not (join_sent or create_sent):
                for room in rooms:
                    if rooms[room].player_count < rooms[room].chair_count:
                        send_join_room_req( s, rooms[room].id )
                        join_sent = True
                if not join_sent:
                        print "TODO CREATE ROOM!"
                        create_sent = True

            # if join or create rsp succeeded, mark self ready
            if (join_rsp_received or create_rsp_received) and not in_room:
                in_room = True
                send_player_ready_ind( s, True )


def receive_loop():
    global s, greeting_received, logged_in, rooms_updated, join_rsp_received, create_rsp_received
    global rooms
    while True:
        msg = txrx.recv_msg( s )
        print msg

        if msg.HasField( "greeting_ind" ):
            greeting_received = True

        if msg.HasField( "login_rsp" ):
            if msg.login_rsp.result == messages_pb2.LoginRsp.RESULT_SUCCESS:
                logged_in = True
            else:
                print( "ERROR: login_rsp: {}".format( msg.login_rsp.result ) )


        if msg.HasField( "rooms_info_ind" ):
            rooms_updated = True
            for room in msg.rooms_info_ind.added_rooms:
                new_room = Room( room.room_id )
                new_room.chair_count = room.room_config.chair_count
                rooms[room.room_id] = new_room
            for room_id in msg.rooms_info_ind.removed_rooms:
                del rooms[room_id]
            for player_count in msg.rooms_info_ind.player_counts:
                rooms[player_count.room_id].player_count = player_count.player_count

            print( "Rooms: {}".format( len(rooms) ) )
            for i in rooms:
                print rooms[i]

        if msg.HasField( "create_room_success_rsp" ):
            create_rsp_received = True

        if msg.HasField( "join_room_success_rspind" ):
            join_rsp_received = True

        if msg.HasField( "player_current_pack_ind" ):
            select_pack_id = msg.player_current_pack_ind.pack_id
            select_card = msg.player_current_pack_ind.cards[0]
            send_player_card_selection_req( s, select_pack_id, select_card )


s = socket.socket()             # Create a socket object
host = socket.gethostname()     # Get local machine name
port = 53333                    # Reserve a port for your service.

s.connect((host, port))

thread_receive_loop = Thread( target=receive_loop )
thread_receive_loop.daemon = True
thread_receive_loop.start()

thread_action_loop = Thread( target=action_loop )
thread_action_loop.daemon = True
thread_action_loop.start()

while True:
    # Do nothing, but need a main loop to accept KeyboardInterrupt
    time.sleep(60)

thread_action_loop.join()
thread_receive_loop.join()
