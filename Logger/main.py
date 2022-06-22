# This is a sample Python script.
import os

import serial
import json
import io
from datetime import datetime

# general settings
port = 'COM31'

MESG_UID_SIZE = 2
NODE_UID_SIZE = 1

MESG_FIRST_BYTE_OFFSET = 1      # start of the actual message "Packet:" is position 0
MESG_UID_OFFSET = 0
TYPE_OFFSET = MESG_UID_OFFSET + MESG_UID_SIZE
HOPS_OFFSET = TYPE_OFFSET + 1
NEXT_PREVIOUS_UID_OFFSET = HOPS_OFFSET + 1
PAYLOAD_OFFSET = NEXT_PREVIOUS_UID_OFFSET + NODE_UID_SIZE


# parse a line
def parse_line(line_str):
    parts = line_str.rstrip().split(' ')
    # read messageUid
    offset = MESG_FIRST_BYTE_OFFSET + MESG_UID_OFFSET
    if NODE_UID_SIZE == 2:
        message_uid = parts[offset] + parts[offset + 1]
    else:
        message_uid = parts[offset]
    # read messageType
    offset = MESG_FIRST_BYTE_OFFSET + TYPE_OFFSET
    message_type = parts[offset]
    # read hops
    offset = MESG_FIRST_BYTE_OFFSET + HOPS_OFFSET
    hops = parts[offset]
    # read nextPreviousUid
    offset = MESG_FIRST_BYTE_OFFSET + NEXT_PREVIOUS_UID_OFFSET
    if NODE_UID_SIZE == 2:
        destination_uid = parts[offset] + parts[offset + 1]  # should always be 0000
    else:
        destination_uid = parts[offset]
    # read payload
    offset = MESG_FIRST_BYTE_OFFSET + PAYLOAD_OFFSET
    payload = parse_payload(parts[offset:])

    # general info for this message
    message_info = {
        "message_uid": message_uid,
        "message_type": message_type,
        "hops": hops,
        "destination_uid": destination_uid,
        "payload": payload,
        "timestamp": datetime.now().strftime("%Y%m%d-%H%M%S"),
        "raw_message": ' '.join(parts[MESG_FIRST_BYTE_OFFSET:])
    }

    return message_info


# parse the rest of a line
def parse_payload(line_parts):
    if not len(line_parts) > 1:
        print("Error parsing appended payload (not enough fields remaining)")
        return {}, []

    offset = 0
    if NODE_UID_SIZE == 2:
        source_uid = line_parts[0] + line_parts[1]
    else:
        source_uid = line_parts[0]

    offset += NODE_UID_SIZE
    try:
        length_field = int(line_parts[offset], 16)
    except ValueError:
        print("ERROR: length field is not an integer -> ", line_parts[NODE_UID_SIZE])

    # extract length of forwarded data (5 Msb) and own data (3 Lsb) from the length field
    own_data_length = (length_field & 0x07)
    forwarded_data_length = ((length_field >> 3) & 0x1F)
    offset += 1
    own_data = line_parts[offset:(offset+own_data_length)]
    rest_of_data = line_parts[(offset+own_data_length):]
    if forwarded_data_length == 0:
        forwarded_data = []
        forwarded_payload = []
    else:
        forwarded_data = line_parts[(offset+own_data_length):]
        forwarded_payload = parse_payload(forwarded_data)
    # extra check
    if not (len(forwarded_data) == forwarded_data_length):
        print("ERROR: forwarded data length does not match length field")

    payload = [
        {
            "source_uid": source_uid,
            "payload_length": own_data_length,
            "payload_data": own_data,
            "forwarded_payload": forwarded_payload
        }
    ]

    # multiple forwarded payloads on a same distance
    if forwarded_data_length == 0 and len(rest_of_data) > 0:
        el = parse_payload(rest_of_data)
        for e in el:
            payload.append(e)

    return payload


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

        last_rssi = 0
        last_snr = 0
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

                if line.find('RSSI: ') > -1:
                    line_parts = line.split(' ')
                    try:
                        last_rssi = int(line_parts[1])
                    except ValueError:
                        last_rssi = 0
                        print("ERROR: could not convert RSSI to an integer")

                if line.find('SNR: ') > -1:
                    line_parts = line.split(' ')
                    try:
                        last_snr = int(line_parts[1])
                    except ValueError:
                        last_snr = 0
                        print("ERROR: could not convert SNR to an integer")

                # check contents of the line and take action when necessary
                if line.find('Packet: ') > -1:
                    print("Parsing line:")
                    print(line)

                    # parse line containing message to json format
                    message_info = parse_line(line)
                    message_info['rssi'] = last_rssi
                    message_info['snr'] = last_snr
                    print(message_info)

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


def get_uids_from_payload(payload_data):
    uid = []
    for single_payload in payload_data:
        uid.append(single_payload["source_uid"])
        fwp = single_payload["forwarded_payload"]
        if len(fwp) > 0:
            uids = get_uids_from_payload(fwp)
            for u in uids:
                uid.append(u)

    return uid


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

                # find all different occurring node uid's
                node_uids = get_uids_from_payload(payload)
                # for each node_uid found, check if it is allready in the statistics, otherwise add it to the list
                for node_uid in node_uids:
                    node_uid_in_list = False
                    for node_info in node_statistics_unsorted:
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

    while not valid_option:
        ans = input("Analyze (A) or log (L): ")

        if ans.upper() == 'A':
            valid_option = True
            run_analysis('')
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
    # line = "Packet: F7 A0 03 00 00 10 62 00 50 50 02 00 1B 20 02 00 55 D0 02 00 09 "
    # info = parse_line(line)
    # print(str(info))

    print('program end')
