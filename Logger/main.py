# This is a sample Python script.
import os

import serial
import json
import io
from datetime import datetime
import argparse
import csv
import ftplib
import configparser

MESG_UID_SIZE = 2
NODE_UID_SIZE = 1
MESG_TYPE_SIZE = 1
MESG_HOPS_SIZE = 1
MESG_PAYLOAD_LEN_SIZE = 1
MESG_LQI_SIZE = 2

MESG_FIRST_BYTE_OFFSET = 0      # start of the actual message "Packet:" is position 0

HEADER_MESG_UID_OFFSET          = 0
HEADER_TYPE_OFFSET              = (HEADER_MESG_UID_OFFSET + MESG_UID_SIZE)
HEADER_HOPS_OFFSET              = (HEADER_TYPE_OFFSET + MESG_TYPE_SIZE)
HEADER_LQI_OFFSET               = (HEADER_HOPS_OFFSET + MESG_HOPS_SIZE)
HEADER_NEXT_UID_OFFSET          = (HEADER_LQI_OFFSET + MESG_LQI_SIZE)
HEADER_PREVIOUS_UID_OFFSET      = (HEADER_LQI_OFFSET + MESG_LQI_SIZE)
HEADER_PAYLOAD_OFFSET           = (HEADER_PREVIOUS_UID_OFFSET + NODE_UID_SIZE)
HEADER_SIZE                     = HEADER_PAYLOAD_OFFSET

PAYLOAD_NODE_UID_OFFSET         = 0
PAYLOAD_LEN_OFFSET              = (PAYLOAD_NODE_UID_OFFSET + NODE_UID_SIZE)
PAYLOAD_DATA_OFFSET             = (PAYLOAD_LEN_OFFSET + MESG_PAYLOAD_LEN_SIZE)


config = configparser.ConfigParser()
config.read('config.ini')

ignoreNext = False

# parse a line
def parse_line(line_str):
    parts = line_str.rstrip().split(' ')
    # read messageUid
    offset = MESG_FIRST_BYTE_OFFSET + HEADER_MESG_UID_OFFSET
    message_uid = parts[offset] + parts[offset + 1]
    # read messageType
    offset = MESG_FIRST_BYTE_OFFSET + HEADER_TYPE_OFFSET
    message_type = parts[offset]
    # read hops
    offset = MESG_FIRST_BYTE_OFFSET + HEADER_HOPS_OFFSET
    hops = parts[offset]
    # cumulative lqi
    offset = MESG_FIRST_BYTE_OFFSET + HEADER_LQI_OFFSET
    lqi = parts[offset]
    # read nextPreviousUid
    offset = MESG_FIRST_BYTE_OFFSET + HEADER_NEXT_UID_OFFSET
    if NODE_UID_SIZE == 2:
        destination_uid = parts[offset] + parts[offset + 1]  # should always be 0000
    else:
        destination_uid = parts[offset]
    # read payload
    offset = MESG_FIRST_BYTE_OFFSET + HEADER_PAYLOAD_OFFSET

    #print("---START---")
    #print(parts[offset:])
    #print(line_str)
    payload = parse_payload(parts[offset:])

    #print("---END---")
    # general info for this message
    message_info = {
        "message_uid": message_uid,
        "message_type": message_type,
        "hops": hops,
        "lqi": lqi,
        "destination_uid": destination_uid,
        "payload": payload,
        "timestamp": datetime.now().strftime("%Y%m%d-%H%M%S"),
        "raw_message": ' '.join(parts[MESG_FIRST_BYTE_OFFSET:])
    }

    return message_info


# parse the rest of a line
def parse_payload(line_parts):
    #print("Got in parse payload: "+ str(line_parts))
    if not len(line_parts) > 1:
        print("Error parsing appended payload (not enough fields remaining)")
        return {}, []

    offset = 0

    payload = []
    while offset < len(line_parts):
        pl = line_parts[offset:]

        if NODE_UID_SIZE == 2:
            source_uid = pl[PAYLOAD_NODE_UID_OFFSET] + pl[PAYLOAD_NODE_UID_OFFSET+1]
        else:
            source_uid = pl[PAYLOAD_NODE_UID_OFFSET+0]

        try:
            length_field = int(pl[PAYLOAD_LEN_OFFSET], 16)
        except ValueError:
            print("ERROR: length field is not an integer -> ", pl[NODE_UID_SIZE])


        # extract length of forwarded data (5 Msb) and own data (3 Lsb) from the length field
        own_data_length = (length_field & 0x07)
        forwarded_data_length = ((length_field >> 3) & 0x1F)

        own_data = pl[PAYLOAD_DATA_OFFSET:(PAYLOAD_DATA_OFFSET+own_data_length)]
        if forwarded_data_length > 0:
            forwarded_data = pl[(PAYLOAD_DATA_OFFSET+own_data_length):(PAYLOAD_DATA_OFFSET+own_data_length+forwarded_data_length)]
            forwarded_payload = parse_payload(forwarded_data)
        else:
            forwarded_data = []
            forwarded_payload = []


        offset = offset+PAYLOAD_DATA_OFFSET+own_data_length+forwarded_data_length

        payload.append(
            {
                "source_uid": source_uid,
                "payload_length": own_data_length,
                "payload_data": own_data,
                "forwarded_payload": forwarded_payload
            }
        )

    # multiple forwarded payloads on a same distance
    # print("end: rest: "+str(rest_of_data) +", data length forwarded: "+str(forwarded_data_length))
    # if forwarded_data_length == 0 and len(rest_of_data) > 0:
    #     el = parse_payload(rest_of_data)
    #     for e in el:
    #         payload.append(e)

    return payload

def ftp_download(file_name):
    ftp = ftplib.FTP(config["FTP"]["Server"],config["FTP"]["User"],config["FTP"]["Password"])
    ftp.cwd(config["FTP"]["Directory"])

    if file_name is None or file_name == '':
        files = ftp.nlst()
        for file in files:
            if not (file.endswith('.json') or file.endswith('.csv')):
                files.remove(file)

        file_name = files[-1]

    try:
        ftp.retrbinary("RETR " + file_name, open(file_name, 'wb').write)
    except Exception as e:
        print("Error")
        print(e)

    return file_name

def process_csv(file_name):
    global last_rssi
    global last_snr
    global last_action
    global own_uid

    own_uid = "FF"
    last_rssi = 255
    last_snr = 255
    last_action = "error"

    print("Opening csv file")
    message_log = {
        "own_uid": "FF",
        "nr_of_messages": 0,
        "messages": []
    }

    with open(file_name, 'r') as file:
        csvReader = csv.reader((line.replace('\0','') for line in file), delimiter=",")
        header = next(csvReader)
        for row in csvReader:
            #print(row[3].strip())
            process_line(row[3].strip(), message_log)

    resultFilename = file_name.replace(".csv", ".json")
    with io.open(resultFilename, 'w', encoding='utf-8') as json_file:
        json_data = json.dumps(message_log, ensure_ascii=False, indent=4)
        json_file.write(json_data)

    return resultFilename

def process_line(line, message_log):
    global last_rssi
    global last_snr
    global last_action
    global own_uid

    if line.find('RSSI: ') > -1:
        line_parts = line.split(' ')
        try:
            last_rssi = int(line_parts[1])
        except ValueError:
            last_rssi = 255
            print("ERROR: could not convert RSSI to an integer")

    if line.find('SNR: ') > -1:
        line_parts = line.split(' ')
        try:
            last_snr = int(line_parts[1])
        except ValueError:
            last_snr = 255
            print("ERROR: could not convert SNR to an integer")

    if line.find('Setting node UID:') > -1:
        line_parts = line.split(' ')
        try:
            own_uid = line_parts[3]
            print(own_uid)
        except ValueError:
            print("ERROR: could not find UID")

    if line.find('Action:') > -1:
        line_parts = line.split(' ')
        last_action = line_parts[1]

    # check contents of the line and take action when necessary
    if line.find('Packet: ') > -1 and last_action == "none/arrived":
        # parse line containing message to json format
        line = line.replace('Packet: ', '')
        line = line.replace('Packet (not for me): ', '')
        message_info = parse_line(line)
        message_info['action'] = last_action
        message_info['last_sender_uid'] = message_info["payload"][0]["source_uid"]
        message_info['packet_rssi'] = last_rssi
        message_info['packet_snr'] = last_snr


        # Correction factor as described in https://cdn-shop.adafruit.com/product-files/3179/sx1276_77_78_79.pdf

        # default: Packet Strength (dBm) = -157 + PacketRssi
        # if SNR < 0 : Packet Strength (dBm) = -157 + PacketRssi + PacketSnr * 0.25
        # if RSSI > -100dBm: RSSI = -157 + 16/15 * PacketRssi

        # if Packet Strength (dBm) = -157 + PacketRssi + PacketSnr * 0.25 > -100 dBm and SNR >= 0:
            # apply correction factor to packetrssi of 16/15

        packet_strength = -157 + last_rssi + last_snr*0.25
        if packet_strength > -100 and last_snr >= 0:
            # apply correction factor to packetrssi of 16/15
            packet_strength = -157 + (16/15)*last_rssi + last_snr*0.25

        message_info['packet_rss'] = packet_strength


        if message_log["own_uid"] == "FF":
            message_log["own_uid"] = own_uid
        message_log["messages"].append(message_info)

    # Reset last action
    if line.find('Message received') >-1:
        last_action = "none"



    # else:
    #     print(line)

def run_logger(port_name):
    global last_rssi
    global last_snr
    global last_action
    global own_uid

    ser = serial.Serial()
    ser.baudrate = 115200
    ser.port = port_name

    message_log = {
        "own_uid": "FF",
        "nr_of_messages": 0,
        "messages": []
    }

    try:
        ser.open()

        own_uid = "FF"
        last_rssi = 255
        last_snr = 255
        last_action = "none"


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
                process_line(line, message_log)

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


def add_to_payload_list(source_uid, own_data, node_statistics):
    if len(own_data)>0:
        #if source_uid == "20":
        #    print(str(own_data))

        if source_uid not in node_statistics.keys():
            # First payload of this uid
            node_statistics[source_uid] = {"last_payload" : int(""+own_data[0]+own_data[1], 16),
                                           "first_payload": int(""+own_data[0]+own_data[1], 16),
                                           "lost_payloads": 0,
                                           "resets": 0,
                                           "arrived_payloads": 1,
                                           "resent_payloads": 0,
                                           "packet_delivery_ratio":0,
                                           "only_forwarded_data": 0}
        else:
            payload_previous = node_statistics[source_uid]["last_payload"]
            node_statistics[source_uid]["last_payload"] = int(""+own_data[0]+own_data[1], 16)
            node_statistics[source_uid]["arrived_payloads"] += 1

            # Process missed payloads
            if node_statistics[source_uid]["last_payload"] - payload_previous > 1:
                #if source_uid == "20":
                #    print("lost")
                # We lost some in between packets (in counting up)
                node_statistics[source_uid]["lost_payloads"] += node_statistics[source_uid]["last_payload"] - payload_previous - 1
            elif node_statistics[source_uid]["last_payload"] - payload_previous < -max(10,payload_previous/100):
                # Reset must have happened, our counter is lower than previous by at least 10 or 1% of counter
                node_statistics[source_uid]["resets"] += 1
                # When reset, starts counting at 0, so first payload is counter for lost payloads
                if source_uid == "20":
                    print("reset")
                    print(str(payload_previous))
                    print(str(node_statistics[source_uid]["last_payload"]))
                node_statistics[source_uid]["lost_payloads"] += node_statistics[source_uid]["last_payload"]
            elif node_statistics[source_uid]["last_payload"] == payload_previous:
                # When payload is received twice in a row
                node_statistics[source_uid]["resent_payloads"] += 1
            node_statistics[source_uid]["packet_delivery_ratio"] = node_statistics[source_uid]["arrived_payloads"]/(node_statistics[source_uid]["lost_payloads"]+node_statistics[source_uid]["arrived_payloads"])
    else:
        if source_uid not in node_statistics.keys():
            # First payload of this uid
            node_statistics[source_uid] = {"last_payload" : None,
                                           "first_payload": None,
                                           "lost_payloads": 0,
                                           "resets": 0,
                                           "arrived_payloads": 1,
                                           "resent_payloads": 0,
                                           "packet_delivery_ratio": 0,
                                           "only_forwarded_data": 1}
        else:
            node_statistics[source_uid]["only_forwarded_data"] += 1

def analyze_payload(payload, node_statistics):
    print("analyze payload")
    # multiple forwarded payloads on a same distance
    add_to_payload_list(payload["source_uid"], payload["payload_data"], node_statistics)

    if len(payload["forwarded_payload"]) > 0:
        for e in payload["forwarded_payload"]:
            analyze_payload(e, node_statistics)

def file_open(file_name):
    if len(file_name) == 0:
        f = []
        for (_, _, filenames) in os.walk(os.getcwd()):
            f.extend(filenames)
            break

        for f_n in f:
            if not (f_n.endswith('.json') or f_n.endswith('.csv')):
                f.remove(f_n)

        file_name = f[-1]

    print('Analysing ' + file_name)

    if file_name.endswith('.csv'):
        file_name = process_csv(file_name)

    return file_name

def run_analysis(file_name):
    payload_list = {}
    payload_last = {}
    node_statistics = {}

    analysed_data = {
        "nr_of_nodes": 0,
        "node_statistics": []
    }

    node_statistics = {}
    try:
        with open(file_open(file_name)) as json_file:
            data = json.load(json_file)
            print("Total number of messages: " + str(data["nr_of_messages"]))

            # first we look for all the different nodes we can find
            for message in data["messages"]:
                payload = message["payload"]
                for e in payload:
                    analyze_payload(e, node_statistics)

            analysed_data["node_statistics"] = node_statistics
            analysed_data["nr_of_nodes"] = len(node_statistics)

            print(node_statistics)

            resultFilename = file_name.replace(".csv", "").replace(".json", "")
            with io.open(resultFilename + "-analysis.json", 'w', encoding='utf-8') as json_file:
                json_data = json.dumps(node_statistics, ensure_ascii=False, indent=4)
                json_file.write(json_data)

    except ValueError as err:
        print("Wrong json format?")


def run_signal_analysis(file_name):

    try:
        with open(file_open(file_name)) as json_file:
            data = json.load(json_file)
            print("Total number of messages: " + str(data["nr_of_messages"]))

            signal_quality_list = {
                "own_uid": data["own_uid"],
                "connections": {}
            }
            # first we look for all the different nodes we can find
            for message in data["messages"]:
                print(message)
                print(message["last_sender_uid"])
                if message["last_sender_uid"] not in signal_quality_list["connections"].keys():
                    signal_quality_list["connections"][message["last_sender_uid"]] = []
                signal_quality_list["connections"][message["last_sender_uid"]].append({"packet_rssi": message["packet_rssi"], "packet_snr": message["packet_snr"], "packet_rss": message["packet_rss"]})

            resultFilename = file_name.replace(".csv", "").replace(".json", "")
            with io.open(resultFilename+"-signal-analysis.json", 'w', encoding='utf-8') as json_file:
                json_data = json.dumps(signal_quality_list, ensure_ascii=False, indent=4)
                json_file.write(json_data)

    except ValueError as err:
        print("Wrong json format?")


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    valid_option = False


    parser = argparse.ArgumentParser("LoraMultiHop Logger and Parser")
    parser.add_argument("command", help="Should we analyze or log?")
    parser.add_argument("-p", "--port", dest="port", default=config["Serial"]["DefaultPort"], help="Serial port")
    parser.add_argument("-f", "--file", dest="file", default="",  help="Filename")
    parser.add_argument("-o", "--online", dest="online", help="Get file from online FTP source", nargs='?', const='')
    parser.add_argument("-s", "--signal", dest="signal", help="Signal Quality Report", nargs='?', const='')

    args = parser.parse_args()

    if args.command.lower() == "analyze" or args.command.lower() == "a":
        file_name = args.file
        if args.online is not None:
            file_name = ftp_download(args.file)
        print("Running analysis...")
        run_analysis(file_name)
        if args.signal is not None:
            run_signal_analysis(file_name)


    elif args.command.lower() == "log" or args.command.lower() == "l":
        print("Starting logger...")
        print("Logging on (" + config["Serial"]["DefaultPort"] + "), change by specifying -p")
        run_logger(args.port)

    # Test parsing of a packet
    # line = "Packet: F7 A0 03 00 00 10 62 00 50 50 02 00 1B 20 02 00 55 D0 02 00 09 "
    # info = parse_line(line)
    # print(str(info))

    print('Program end')
