import socket
import struct
import collections
import binascii
import zlib
import time
import sys
import argparse

# Session message types.
SM_SessionRequest = 0x01
SM_SessionResponse = 0x02
SM_Combined = 0x03
SM_SessionDisconnect = 0x05
SM_ApplicationPacket = 0x09
SM_Ack = 0x15
SM_Types = {value: name for name, value in globals().items() if name.startswith("SM_")}

# Login message types.
LM_ChatMessageRequest = 0x01
LM_LoginRequest = 0x02
LM_UnknownRequest = 0x03
LM_ServerListRequest = 0x04
LM_PlayRequest = 0x0d
LM_ChatMessageResponse = 0x16
LM_LoginResponse = 0x17
LM_ServerListResponse = 0x18
LM_PlayResponse = 0x21
LM_Types = {value: name for name, value in globals().items() if name.startswith("LM_")}

# World message types (Titanium).
WM_SendLoginInfo = 0x4dd0
WM_Types = {value: name for name, value in globals().items() if name.startswith("WM_")}

ALL_Types = {"SM": SM_Types, "LM": LM_Types, "WM": WM_Types}

# Possible status values in PlayResponse messages.
PLAY_Allowed = 101
PLAY_Denied = 326
PLAY_Suspended = 337
PLAY_Banned = 338
PLAY_WordFull = 303

class Parameter(object):
    def __init__(self, name, code, value):
        self.name = name
        self.code = code
        self.value = value

class Message(object):
    def __init__(self, ns, type):
        self.ns = ns
        self.type = type
        self.params = collections.OrderedDict()
        self.body = None
    
    def add_param(self, name, code, value=None):
        self.params[name] = Parameter(name, code, value)
    
    def serialize(self):
        patterns = []
        if self.ns == "SM":
            patterns.append("!")
        else:
            patterns.append("<")
        patterns.append("H")
        args = [self.type]
        for param in self.params.values():
            patterns.append(param.code)
            args.append(param.value)
        data = struct.pack("".join(patterns), *args)
        if self.body:
            data += self.body
        return data
    
    def deserialize(self, data):
        if self.ns == "SM":
            byte_order = "!"
        else:
            byte_order = "<"
        patterns = [param.code for param in self.params.values()]
        pattern = byte_order + "".join(patterns)
        total_size = struct.calcsize(pattern)
        #print("deserialize() -> pattern: %s total_size: %d data: %s" % (pattern, total_size, repr(data)))
        values = struct.unpack(pattern, data[0:total_size])

        # Assign the deserialized values to the message parameters.
        param_names = list(self.params)
        value_pos = 0
        for param_index in range(0, len(patterns)):
            name = param_names[param_index]
            pattern = patterns[param_index]
            num_values = len(pattern)
            if num_values == 1:
                param_value = values[value_pos]
            else:
                param_value = values[value_pos:value_pos + num_values]
            self.params[name].value = param_value
            value_pos += num_values

        # Any remaining data belongs to the message body.
        if len(data) > total_size:
            self.body = data[total_size:]
    
    def __getitem__(self, name):
        param = self.params[name]
        return param.value
    
    def has_param(self, name):
        return name in self.params

    def __str__(self):
        chunks = []
        msg_types = ALL_Types[self.ns]
        try:
            msg_name = msg_types[self.type]
        except KeyError:
            msg_name = "%s_%02x" % (self.__class__.__name__, self.type)
            
        param_chunks = []
        for param in self.params.values():
            param_chunks.append("%s=%s" % (param.name, param.value))
        chunks.append("<")
        chunks.append(msg_name)
        chunks.append("(%s)" % ", ".join(param_chunks))
        chunks.append(">")
        return "".join(chunks)

class SessionMessage(Message):
    def __init__(self, type):
        super(SessionMessage, self).__init__("SM", type)

class LoginMessage(Message):
    def __init__(self, type):
        super(LoginMessage, self).__init__("LM", type)

class WorldMessage(Message):
    def __init__(self, type):
        super(LoginMessage, self).__init__("WM", type)

class SessionClient(object):
    def __init__(self, addr):
        self.addr = addr
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.compressed = False
        self.crc_key = 0
        self.next_ack_in = 0
        self.next_seq_in = 0
        self.next_seq_out = 0
        self.pending_packets = []
    
    def close(self):
        self.socket.close()

    def receive(self):
        """ Receive a session message from the server. """
        while True:
            packet, crc_removed = self._receive_packet()
            if not packet:
                # XXX Is it really possible to receive zero from recvfrom?
                return None
            session_msg = self._parse_packet(packet, crc_removed)
            if session_msg.type == SM_Ack:
                seq_num = session_msg["SeqNum"]
                if seq_num >= self.next_seq_out:
                    raise Exception("Received ack for message that was not sent: %d (seq_out = %d)"
                        % (seq_num, self.next_seq_out - 1))
                self.next_ack_in = seq_num + 1
            elif session_msg.type == SM_Combined:
                pos = 0
                data = session_msg.body
                while pos < len(data):
                    sub_msg_len = ord(data[pos])
                    pos += 1
                    if (pos + sub_msg_len) > len(data):
                        raise Exception("Sub-message length out of range.")
                    sub_msg_data = data[pos:pos + sub_msg_len]
                    pos += sub_msg_len
                    self.pending_packets.append((sub_msg_data, True))
            elif self._has_seq_num(session_msg.type):
                # Only accept messages in order.
                seq_num = session_msg["SeqNum"]
                if self.next_seq_in == seq_num:
                    self._send_ack(seq_num)
                    return session_msg
            else:
                return session_msg
    
    def send(self, msg):
        """ Send a session message to the server. """
        if msg.ns != "SM":
            raise ValueError("Not a session message.")
        elif self._has_seq_num(msg.type):
            msg.add_param("SeqNum", "H", self.next_seq_out)
            self.next_seq_out += 1
        packet = msg.serialize()
        
        # Compress the packet if asked to and if it makes sense.
        if self.compressed:
            header, payload = packet[0:2], packet[2:]
            compression_flag = chr(0xa5) # Uncompressed by default.
            if len(payload) > 10:
                payload, compression_flag = zlib.compress(payload), 'Z'
            packet = header + compression_flag + payload
        
        # Add checksum at the end of the packet.
        # XXX Figure out why we need to byte-swap it before sending.
        if msg.type not in (SM_SessionRequest, SM_SessionResponse):
            crc = socket.htons(self._crc32(packet) & 0xffff)
            packet += struct.pack("H", crc)
        self._send_packet(packet)
    
    def _format_addr(self, addr):
        return "%s:%d" % addr
    
    def _send_ack(self, seq_num):
        if seq_num != self.next_seq_in:
            raise ValueError("Sending ack for message that was not received: %d (seq_in = %d)"
                % (seq_num, self.next_seq_in - 1))
        ack_msg = SessionMessage(SM_Ack)
        ack_msg.add_param("SeqNum", "H", seq_num)
        self.send(ack_msg)
        self.next_seq_in += 1
    
    def _send_packet(self, packet):
        """ Send a session packet to the server. """
        print("%s >>> %s" % (self._format_addr(self.addr),
                             binascii.b2a_hex(packet).decode('utf8')))
        self.socket.sendto(bytes(packet), self.addr)
    
    def _receive_packet(self, max_size=1024):
        """ Return a session packet from the server. """
        if self.pending_packets:
            packet, crc_removed = self.pending_packets.pop(0)
            remote = self.addr
        else:
            packet, remote = self.socket.recvfrom(1024)
            crc_removed = False
            print("%s <<< %s" % (self._format_addr(remote),
                                 binascii.b2a_hex(packet).decode('utf8')))
        return packet, crc_removed
    
    def _has_seq_num(self, sm_type):
        """ Determine whether the message has a sequence number parameter and
        whether it should be incremented before sending it. """
        return sm_type in (SM_ApplicationPacket, )

    def _crc32_round(self, a, b):
        return (a >> 8) ^ CRC32Lookup[(b ^ a) & 0xff]
    
    def _crc32(self, data):
        crc = binascii.crc32(struct.pack("<I", self.crc_key))
        crc = binascii.crc32(data, crc)
        return crc & 0xffffffff
    
    def _parse_packet(self, packet, no_crc=False):
        """ Parse a session packet. """
        # Extract the message type and CRC and validate them.
        msg_type = struct.unpack("!H", packet[0:2])[0]
        if msg_type > 0xff:
            raise ValueError("Message type out of range: 0x%04x" % msg_type)
        has_crc = (msg_type not in (SM_SessionRequest, SM_SessionResponse) and not no_crc)
        if has_crc:
            crc, packet = struct.unpack("!H", packet[-2:])[0], packet[0:-2]
            computed_crc = self._crc32(packet) & 0xffff
            if (crc != 0) and (crc != computed_crc):
                raise ValueError("Invalid CRC: computed 0x%04x, but found 0x%04x." % (computed_crc, crc))
        packet = packet[2:]
        
        # Decompresse the packet's payload if needed.
        if self.compressed:
            # Check the flag byte to see if the packet really is compressed.
            compression_flag = packet[0]
            packet = packet[1:]
            if compression_flag == 'Z':
                # Deflate-compressed.
                packet = zlib.decompress(packet)
            elif compression_flag == chr(0xa5):
                # Not compressed.
                pass
            else:
                raise ValueError("Invalid compression flag: 0x%x." % ord(compression_flag))
        
        # Specify the header fields for the message, if we know about any.
        msg = SessionMessage(msg_type)
        if msg_type == SM_SessionResponse:
            msg.add_param("Session", "I")
            msg.add_param("Key", "I")
            msg.add_param("UnknownA", "B")
            msg.add_param("Format", "B")
            msg.add_param("UnknownB", "B")
            msg.add_param("MaxLength", "I")
            msg.add_param("UnknownC", "I")
        elif msg_type == SM_ApplicationPacket:
            msg.add_param("SeqNum", "H")
        elif msg_type == SM_Ack:
            msg.add_param("SeqNum", "H")

        # Deserialize the session data, copying any unknown data to the body.
        msg.deserialize(packet)
        return msg

class ApplicationClient(object):
    """ High-level interface to send and receive messages over a session. """
    def __init__(self, msg_types, compressed):
        self.msg_types = msg_types
        self.session_id = 0x26ec5075 # XXX Should it be random?
        self.session = None
        self.compressed = compressed
    
    def __enter__(self):
        pass
    
    def __exit__(self, type, value, tb):
        self.disconnect()
    
    def connect(self, remote_addr):
        """ Initiate a session with the remote server. """
        if self.session:
            return
        session = SessionClient(remote_addr)
        connected = False
        try:
            request = SessionMessage(SM_SessionRequest)
            request.add_param("UnknownA", "I", 0x00000002)
            request.add_param("Session", "I", self.session_id)
            request.add_param("MaxLength", "I", 0x00000200)
            session.send(request)
            response = session.receive()
            if response.type != SM_SessionResponse:
                raise Exception("Server did not respond with SessionResponse")
            response_id = response["Session"]
            if response_id != self.session_id:
                raise Exception("Server responded with different session ID: 0x%x, ours: 0x%x"
                    % (response_id, self.session_id))
            session.crc_key = response["Key"]
            session.compressed = self.compressed
            self.session = session
            connected = True
        finally:
            if not connected:
                session.close()
    
    def disconnect(self):
        if not self.session:
            return
        try:
            request = SessionMessage(SM_SessionDisconnect)
            request.add_param("Session", "I", self.session_id)
            request.add_param("UnknownA", "H", 6)
            self.session.send(request)
        finally:
            self.session.close()
            self.session = None
    
    def receive(self):
        """ Wait until an application message has been received from the server. """
        session_msg = self.session.receive()
        if (not session_msg) or (session_msg.type == SM_SessionDisconnect):
            self.active = False
            return None
        elif session_msg.type == SM_ApplicationPacket:
            return self._parse_packet(session_msg.body)
        else:
            print("Unexpected session message: %s" % session_msg)
            return None

    def send(self, app_msg):
        """ Send an application message to the server. """
        if app_msg.ns not in ("LM", "WM"):
            raise ValueError("Not an application message.")
        session_msg = SessionMessage(SM_ApplicationPacket)
        session_msg.body = app_msg.serialize()
        self.session.send(session_msg)

    def _parse_packet(self, packet):
        """ Parse an application message from a received packet. """
        # Extract the message type.
        msg_type, packet = struct.unpack("<H", packet[0:2])[0], packet[2:]
        msg = LoginMessage(msg_type)
        
        # Call the function that can parse the message, if it exists.
        try:
            msg_name = self.msg_types[msg_type]
        except KeyError:
            pass
        else:
            fn_name = "_parse_%s" % msg_name
            if hasattr(self, fn_name):
                fn = getattr(self, fn_name)
                fn(msg, packet)
        return msg
    
    def _read_c_string(self, data, pos):
        nul_pos = data.index("\0", pos)
        return data[pos:nul_pos], nul_pos + 1
    
    def _read_field(self, data, pos, pattern):
        size = struct.calcsize(pattern)
        return struct.unpack(pattern, data[pos:pos+size])[0], pos + size

class LoginClient(ApplicationClient):
    """ High-level interface to talk to a login server. """
    def __init__(self):
        super(LoginClient, self).__init__(LM_Types, False)
    
    def begin_get_chat_message(self):
        request = LoginMessage(LM_ChatMessageRequest)
        request.add_param("UnknownA", "I", 2)
        request.add_param("UnknownB", "I", 0)
        request.add_param("UnknownC", "H", 8)
        request.add_param("UnknownD", "H", 0)
        self.send(request)
    
    def begin_login(self, username, password):
        request = LoginMessage(LM_LoginRequest)
        request.add_param("UnknownA", "I", 3)
        request.add_param("UnknownB", "I", 2)
        request.add_param("UnknownC", "H", 0)
        # Allowed packet lengths: 20, 28, 36, 44, etc.
        packet_size = (len(password) + len(username) + 14)
        allowed_size = 20
        while allowed_size < packet_size:
            allowed_size += 8
        padding = (allowed_size - packet_size + 1)
        credentials_chunks = [password, "\x00", username, "\x00" * padding]
        request.body = "".join(credentials_chunks)
        self.send(request)
    
    def begin_list_servers(self):
        request = LoginMessage(LM_ServerListRequest)
        request.add_param("UnknownA", "I", 4)
        request.add_param("UnknownB", "I", 0)
        request.add_param("UnknownC", "H", 0)
        self.send(request)
    
    def begin_play(self, server_id, sequence):
        request = LoginMessage(LM_PlayRequest)
        request.add_param("Sequence", "H", sequence)
        request.add_param("UnknownA", "I", 0)
        request.add_param("UnknownB", "I", 0)
        request.add_param("ServerID", "I", server_id)
        self.send(request)
    
    def end_get_chat_message(self, response):
        """ Extract the chat message from the response. """
        return response.body
    
    def end_login(self, response):
        """ Extract the login result from the response.
        This is a tuple with the values: success, user ID and session key. """
        result_fields = struct.unpack("IIi16sI", response.body[0:32])
        status, unknown_a, user_id, key, failed_attempts = result_fields
        if (status != 1) or (user_id == -1):
            return False, user_id, None
        return True, user_id, key

    def end_list_servers(self, response):
        msg = response
        servers = []
        pos = 0
        while msg.body and (pos < len(msg.body)):
            server = ServerInfo()
            server.host, pos = self._read_c_string(msg.body, pos)
            server.type, pos = self._read_field(msg.body, pos, "I")
            server.runtime_id, pos = self._read_field(msg.body, pos, "I")
            server.name, pos = self._read_c_string(msg.body, pos)
            locale_1, pos = self._read_c_string(msg.body, pos)
            locale_2, pos = self._read_c_string(msg.body, pos)
            server.locale = (locale_1, locale_2)
            server.status, pos = self._read_field(msg.body, pos, "I")
            server.players, pos = self._read_field(msg.body, pos, "I")
            servers.append(server)
        return servers
    
    def end_play(self, response):
        """ Extract the play result from the response.
        This is a tuple with the values: success, status, server ID and sequence. """
        success = (response["Allowed"] == 1)
        status = response["Status"]
        server_id = response["ServerID"]
        sequence = response["Sequence"]
        return success, status, server_id, sequence

    def _parse_LM_ChatMessageResponse(self, msg, packet):
        msg.add_param("UnknownA", "I")
        msg.add_param("UnknownB", "I")
        msg.add_param("UnknownC", "BBB")
        msg.add_param("UnknownD", "I")
        msg.deserialize(packet)
    
    def _parse_LM_LoginResponse(self, msg, packet):
        msg.add_param("UnknownA", "I")
        msg.add_param("UnknownB", "I")
        msg.add_param("UnknownC", "H")
        msg.deserialize(packet)
    
    def _parse_LM_ServerListResponse(self, msg, packet):
        msg.add_param("UnknownA", "I")
        msg.add_param("UnknownB", "I")
        msg.add_param("UnknownC", "I")
        msg.add_param("UnknownD", "I")
        msg.add_param("ServerCount", "I")
        msg.deserialize(packet)
    
    def _parse_LM_PlayResponse(self, msg, packet):
        msg.add_param("Sequence", "I")
        msg.add_param("UnknownA", "B" * 6)
        msg.add_param("Allowed", "B")
        msg.add_param("Status", "H")
        msg.add_param("UnknownB", "B" * 3)
        msg.add_param("ServerID", "I")
        msg.deserialize(packet)

class WorldClient(ApplicationClient):
    """ High-level interface to talk to a world server. """
    def __init__(self):
        super(WorldClient, self).__init__(WM_Types, True)
    
    def begin_login(self, server_id, session_key):
        zoning = "\x00" # "\x01" when zoning.
        request = LoginMessage(WM_SendLoginInfo)
        body_chunks = []
        body_chunks.append(str(server_id))
        body_chunks.append("\x00")
        body_chunks.append(session_key)
        body_chunks.append("\x00")
        padding = 124 + 64 - sum(len(chunk) for chunk in body_chunks)
        body_chunks.append("\x00" * padding)
        body_chunks.append(zoning)
        padding2 = 464 - sum(len(chunk) for chunk in body_chunks)
        body_chunks.append("\x00" * padding2)
        request.body = "".join(body_chunks)
        self.send(request)

class ServerInfo(object):
    def __init__(self):
        self.type = 0
        self.runtime_id = 0
        self.status = 0
        self.players = 0
        self.host = None
        self.name = None
        self.locale = None

def client_login(server_addr, username, password):
    client = LoginClient()
    client.connect(server_addr)
    with client:
        stage = 0
        server_by_id = {}
        session_key = None
        client.begin_get_chat_message()
        response = client.receive()
        while response:
            handled = False
            if (stage == 0) and (response.type == LM_ChatMessageResponse):
                # Waiting for a chat message response.
                print("Chat message: %s" % client.end_get_chat_message(response))
                stage = 1
                handled = True
                client.begin_login(username, password)
            elif (stage == 1) and (response.type == LM_LoginResponse):
                # Waiting for a login response.
                success, user_id, session_key = client.end_login(response)
                if success:
                    print("Successfully logged in.")
                else:
                    print("Failed to logd in.")
                    break
                stage = 2
                handled = True
                client.begin_list_servers()
            elif (stage == 2) and (response.type == LM_ServerListResponse):
                # Waiting for a server list response.
                servers = client.end_list_servers(response)
                print("%d servers online." % len(servers))
                for i, server in enumerate(servers):
                    server_by_id[server.runtime_id] = server
                    print("%d: %s (%d players, ID=%d)" % (i, server.name,
                        server.players, server.runtime_id))
                # Login in the first server in the list.
                handled = True
                if servers:
                    stage = 3
                    sequence = 5
                    client.begin_play(servers[0].runtime_id, sequence)
                else:
                    print("No server to connect to, exiting.")
                    break
            elif (stage == 3) and (response.type == LM_PlayResponse):
                # Waiting for a play response.
                handled = True
                success, status, server_id, sequence = client.end_play(response)
                server = server_by_id[server_id]
                if success:
                    print("Server '%s' (ID=%d) allowed play." % (server.name, server_id))
                    # HACK: give the login server a chance to tell the world
                    # server that we can log in.
                    time.sleep(0.1)
                    client_play(server, sequence, session_key)
                else:
                    print("Server '%s' (ID=%d) disallowed play, reason = %d." %
                        (server.name, server_id, status))
                    break
            if not handled:
                print(response)
            response = client.receive()

def client_play(server, sequence, session_key):
    client = WorldClient()
    server_addr = (server.host, 9000)
    client.connect(server_addr)
    with client:
        stage = 0
        client.begin_login(sequence, session_key)
        response = client.receive()
        while response:
            handled = False
            #if (stage == 0) and (response.type == LM_ChatMessageResponse):
            #    # Waiting for a login response.
            #    print("Chat message: %s" % client.end_get_chat_message(response))
            #    stage = 1
            #    handled = True
            #    client.begin_login(username, password)
            if not handled:
                print(response)
            response = client.receive()

def main():
    parser = argparse.ArgumentParser(description='Connect to an EQEmu login server.')
    parser.add_argument("-H", "--host", default="localhost")
    parser.add_argument("-P", "--port", type=int, default=5998)
    parser.add_argument("-u", "--user", default='user')
    parser.add_argument("-p", "--password", default='password')
    args = parser.parse_args()
    try:
        client_login((args.host, args.port), args.user, args.password)
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    main()
