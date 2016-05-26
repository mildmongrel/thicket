import socket
import time
import random
import string
import messages_pb2
from threading import Thread
import Queue
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

def send_room_chat_message_ind( sock, text ):
    msg = messages_pb2.ClientToServerMsg()
    msg.chat_message_ind.scope = messages_pb2.CHAT_SCOPE_ROOM
    msg.chat_message_ind.text = text
    txrx.send_msg( sock, msg )

def send_login_req( sock, name ):
    msg = messages_pb2.ClientToServerMsg()
    msg.login_req.name = name
    txrx.send_msg( sock, msg )

def send_create_room_req( sock, name ):
    msg = messages_pb2.ClientToServerMsg()
    msg.create_room_req.room_config.name = name
    msg.create_room_req.room_config.password_protected = False
    msg.create_room_req.room_config.chair_count = 8
    msg.create_room_req.room_config.bot_count = 4
    # add rounds
    for i in range(0,3):
        round = msg.create_room_req.room_config.rounds.add()
        round.booster_round_config.clockwise = True
        round.booster_round_config.time = 30
        bundle = round.booster_round_config.card_bundles.add()
        bundle.set_code = "10E"
        bundle.method = messages_pb2.RoomConfiguration.CardBundle.METHOD_BOOSTER
        bundle.set_replacement = True
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


class Room:
    def __init__(self, id):
        self.id = id
        self.chair_count = 0
        self.player_count = 0
    def __str__(self):
        return "id={}: players={}/{}".format( self.id, self.player_count, self.chair_count )

class Client:

    instance = 0

    def __init__(self, name):
        self.name = name
        self.instance = Client.instance
        Client.instance += 1
        self.sock = socket.socket()
        self.rx_msg_q = Queue.Queue()
        self.receive_thread = Thread( target=self.receive_loop )
        self.receive_thread.daemon = True
        self.action_thread = Thread( target=self.action_loop )
        self.action_thread.daemon = True
        self.print_msgs = True
        self.complete = False
        random.jumpahead( self.instance+5000 )

    def start(self):
        host = socket.gethostname()     # Get local machine name
        port = 53333                    # Reserve a port for your service.
        self.sock.connect((host, port))

        self.receive_thread.start()
        self.action_thread.start()

    def join(self):
        self.receive_thread.join()
        self.action_thread.join()

    def action_loop(self):

        logged_in = False
        rooms = {}
        join_sent = False
        create_sent = False
        in_room = False
        in_room_time = None
        draft_started = False
        next_keep_alive_time = None
        next_chat_check_time = None

        while not self.complete:

            try:
                msg = self.rx_msg_q.get( True, 1 )
            except Queue.Empty:
                msg = None

            # PROCESS TIME-BASED EVENTS

            if logged_in:

                # Send keepalives every 25 seconds if logged in
                if next_keep_alive_time == None:
                    next_keep_alive_time = time.time()

                if time.time() >= next_keep_alive_time:
                    print "sending keep_alive"
                    send_keep_alive_ind( self.sock )
                    next_keep_alive_time += 25

                if next_chat_check_time == None:
                    next_chat_check_time = time.time()

                if time.time() >= next_chat_check_time:
                    # 1% chance of sending server chat message per interval once logged in
                    if random.random() < 0.01:
                        print "sending random chat msg"
                        n = int( random.random() * 100 ) + 1;
                        send_server_chat_message_ind( self.sock, random_lowercase_str(n) )

                    # 1% chance of sending room chat message per interval once in room
                    if in_room and (random.random() < 0.01):
                        print "sending random chat msg"
                        n = int( random.random() * 100 ) + 1;
                        send_room_chat_message_ind( self.sock, random_lowercase_str(n) )

                    next_chat_check_time += 1;

                if in_room:
                    # If in room for 60 seconds and draft not started, bail
                    if (time.time() - in_room_time >= 60) and not draft_started:
                        print "draft not started after 60s, closing"
                        self.sock.close()
                        self.complete = True
                        continue

            # PROCESS MESSAGE-BASED EVENTS

            if not msg:
                continue

            if msg.HasField( "greeting_ind" ):
                if not logged_in:
                    print "sending login_req"
                    send_login_req( self.sock, self.name )

            if msg.HasField( "login_rsp" ):
                if msg.login_rsp.result == messages_pb2.LoginRsp.RESULT_SUCCESS:
                    print "LOGGED IN"
                    logged_in = True
                else:
                    print( "ERROR: login_rsp: {}".format( msg.login_rsp.result ) )

            if msg.HasField( "rooms_info_ind" ):
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

                if not (join_sent or create_sent):
                    for room in rooms:
                        if rooms[room].player_count < rooms[room].chair_count:
                            print "JOIN ROOM!"
                            send_join_room_req( self.sock, rooms[room].id )
                            join_sent = True
                            break
                    if not join_sent:
                            print "CREATE ROOM!"
                            send_create_room_req( self.sock, "{}_{}".format( self.name, self.instance ) )
                            create_sent = True;

            if msg.HasField( "create_room_success_rsp" ):
                # if create succeeded, send join req
                created_room_id = msg.create_room_success_rsp.room_id
                send_join_room_req( self.sock, created_room_id )

            if msg.HasField( "create_room_failure_rsp" ):
                # if create failed, reset state to wait for room info update again
                print "CREATE ROOM FAILURE: ".format( msg.create_room_failure_rsp.result )
                create_sent = False;

            if msg.HasField( "join_room_success_rspind" ):
                in_room = True
                in_room_time = time.time()

                # Ready to go 
                send_player_ready_ind( self.sock, True )

            if msg.HasField( "join_room_failure_rsp" ):
                # if join failed, reset state to wait for room info update again
                print "JOIN ROOM FAILURE: {}".format( msg.join_room_failure_rsp.result )
                join_sent = False;

            if msg.HasField( "player_current_pack_ind" ):
                select_pack_id = msg.player_current_pack_ind.pack_id
                select_card = msg.player_current_pack_ind.cards[0]
                send_player_card_selection_req( self.sock, select_pack_id, select_card )

            if msg.HasField( "room_stage_ind" ):
                self.draft_started = True
                if msg.room_stage_ind.complete:
                    print "COMPLETE!"
                    self.sock.close()
                    self.complete = True

        print "action loop exiting"


    def receive_loop(self):
        while not self.complete:
            try:
                msg = txrx.recv_msg( self.sock )
            except socket.error:
                msg = None

            if not msg:
                print "receive loop aborting"
                break
            if self.print_msgs:
                print msg
            self.rx_msg_q.put( msg )

        print "receive loop exiting"



for i in range( 0, 100 ):
    c = Client( "Client{}".format(i) )
    c.print_msgs = False
    c.start()

    # Delay a small (<1s) random amount to start next client, otherwise
    # all clients create a room at the same instant.
    time.sleep(random.random())


while True:
    # Do nothing, but need a main loop to accept KeyboardInterrupt
    time.sleep(60)

