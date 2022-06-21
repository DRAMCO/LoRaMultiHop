# This is a sample Python script.
import os

import serial
import json
import io
from datetime import datetime

# general settings
port = 'COM31'

MESG_FIRST_BYTE_OFFSET = 1
MESG_UID_OFFSET = 0
TYPE_OFFSET = 2
HOPS_OFFSET = 3
NEXT_PREVIOUS_UID_OFFSET = 4
NODE_UID_OFFSET = 6
PAYLOAD_SIZE_OFFSET = 8
PAYLOAD_OFFSET = 9


# parse a line
def parse_line(line_str):
    parts = line_str.rstrip().split(' ')
    # read messageUid
    offset = MESG_FIRST_BYTE_OFFSET + MESG_UID_OFFSET
    message_uid = parts[offset] + parts[offset + 1]
    # read messageType
    offset = MESG_FIRST_BYTE_OFFSET + TYPE_OFFSET
    message_type = parts[offset]
    # read hops
    offset = MESG_FIRST_BYTE_OFFSET + HOPS_OFFSET
    hops = parts[offset]
    # read nextPreviousUid
    offset = MESG_FIRST_BYTE_OFFSET + NEXT_PREVIOUS_UID_OFFSET
    destination_uid = parts[offset] + parts[offset + 1]  # should always be 0000
    # read sourceUid
    offset = MESG_FIRST_BYTE_OFFSET + NODE_UID_OFFSET
    source_uid = parts[offset] + parts[offset + 1]
    # read payloadLength
    offset = MESG_FIRST_BYTE_OFFSET + PAYLOAD_SIZE_OFFSET
    payload_length = parts[offset]
    try:
        payload_length_int = int(payload_length, 16)
        offset = MESG_FIRST_BYTE_OFFSET + PAYLOAD_OFFSET
        payload_data = parts[offset:(offset + payload_length_int)]
        rest_of_line = parts[(offset + payload_length_int):]
    except ValueError:
        print("Wrong payload length: " + payload_length)
        payload_data = []
        rest_of_line = []
        payload_length_int = -1

    # dictionary to store all payloads from this message
    # we initialize using the first payload
    payload = {
        "nr_of_payloads": 1,
        "payloads": []
    }
    first_payload = {
        "source_uid": source_uid,
        "payload_length": payload_length_int,
        "payload_data": payload_data
    }

    payload["payloads"].append(first_payload)

    # general info for this message
    message_info = {
        "message_uid": message_uid,
        "message_type": message_type,
        "hops": hops,
        "destination_uid": destination_uid,
        "payload": payload,
        "timestamp": datetime.now().strftime("%Y%m%d-%H%M%S")
    }

    # check if more payloads were appended
    rest_parsed = True
    if not len(rest_of_line) == 0:
        rest_parsed = False
    while not rest_parsed:
        (new_payload, rest_of_line) = parse_rest(rest_of_line)
        # add payload to payloads
        payload = message_info["payload"]
        payload["nr_of_payloads"] = payload["nr_of_payloads"] + 1
        payload["payloads"].append(new_payload)
        if len(rest_of_line) == 0:
            rest_parsed = True

    return message_info


# parse the rest of a line
def parse_rest(line_parts):
    if not len(line_parts) > 3:
        print("Error parsing appended payload (not enough fields remaining)")
        return {}, []

    source_uid = line_parts[0] + line_parts[1]
    payload_length = line_parts[2]
    try:
        payload_length_int = int(payload_length, 16)
        payload_data = line_parts[3:(3 + payload_length_int)]
        rest_of_line = line_parts[(3 + payload_length_int):]
    except ValueError:
        print("Wrong payload length (in rest): " + payload_length)
        payload_data = []
        rest_of_line = []
        payload_length_int = -1

    payload = {
        "source_uid": source_uid,
        "payload_length": payload_length_int,
        "payload_data": payload_data
    }

    return payload, rest_of_line


def run_logger(port_name):
    ser = serial.Serial()
    ser.baudrate = 115200
    ser.port = port_name

    message_log = {
        "nr_of_messages": 0,
        "messages": []
    }

    try:
        ser.open()

        try:
            while True:  # continuously read and parse lines
                line = ""
                line_complete = False
                while not line_complete:  # read characters until '\n' (and throw away any shit)
                    char = ser.read(1)
                    if not (char < b' '):
                        line += char.decode()
                    if char == b'\n':
                        line_complete = True

                # check contents of the line and take action when necessary
                if line.find('Packet: ') > -1:
                    print("Parsing line:")
                    print(line)

                    # parse line containing message to json format
                    message_info = parse_line(line)

                    # add message_info to list // TODO: do we have enough RAM?
                    message_log["nr_of_messages"] = message_log["nr_of_messages"] + 1
                    message_log["messages"].append(message_info)

                else:
                    print(line)

        except KeyboardInterrupt:
            if ser.isOpen():
                ser.close()

    except serial.SerialException as err:
        print(f"Unexpected {err=}, {type(err)=}")

    print("Writing log")
    filename = datetime.now().strftime("%Y%m%d-%H%M%S.json")
    with io.open(filename, 'w', encoding='utf-8') as json_file:
        json_data = json.dumps(message_log, ensure_ascii=False, indent=4)
        json_file.write(json_data)


def run_analysis(file_name):
    if len(file_name) == 0:
        f = []
        for (_, _, filenames) in os.walk(os.getcwd()):
            f.extend(filenames)
            break

        for f_n in f:
            if not f_n.endswith('.json'):
                f.remove(f_n)

        file_name = f[-1]

    print('Analysing ' + file_name)

    analysed_data = {
        "nr_of_nodes": 0,
        "node_statistics": []
    }

    node_statistics_unsorted = []
    try:
        with open(file_name) as json_file:
            data = json.load(json_file)
            print("Total number of messages: " + str(data["nr_of_messages"]))

            # first we look for all the different nodes we can find
            for message in data["messages"]:
                payload = message["payload"]
                if payload["nr_of_payloads"] > 0:
                    for single_payload in payload["payloads"]:
                        node_uid = single_payload["source_uid"]
                        node_uid_in_list = False
                        for node_info in node_statistics_unsorted:
                            if len(analysed_data) > 0:
                                if node_info["node_uid"] == node_uid:
                                    node_uid_in_list = True
                                    break
                        if not node_uid_in_list:
                            node_statistics_unsorted.append({"node_uid": node_uid})

            node_statistics_sorted = sorted(node_statistics_unsorted, key=lambda k: k["node_uid"])
            analysed_data["node_statistics"] = node_statistics_sorted
            analysed_data["nr_of_nodes"] = len(node_statistics_sorted)
            print(analysed_data)

    except ValueError as err:
        print("Wrong json format?")


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    valid_option = False

    #run_analysis('')

    while not valid_option:
        ans = input("Analyze (A) or log (L): ")

        if ans.upper() == 'A':
            valid_option = True
            print("Running analysis...")

        if ans.upper() == 'L':
            valid_option = True
            print("Starting logger...")

            prompt = "Specify port or enter for (" + port + "): "
            ans = input(prompt)
            if len(ans.rstrip()) == 0:
                ans = port.upper()

            run_logger(ans)

    # Test parsing of a packet
    # line = "Packet: 45 7E 03 00 00 00 30 00 02 00 6B 20 00 01 55 12 34 03 10 20 FF\n"
    # info = parse_line(line)
    # print(str(info))

    print('program end')
