import csv
import payloadParser
import serial
import json
import io

def ftpDownload(file_name):
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

def processCsv(file_name):
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
            processLine(row[3].strip(), message_log)

    resultFilename = file_name.replace(".csv", ".json")
    with io.open(resultFilename, 'w', encoding='utf-8') as json_file:
        json_data = json.dumps(message_log, ensure_ascii=False, indent=4)
        json_file.write(json_data)

    return resultFilename

def processLine(line, message_log):
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
        message_info = payloadParser.parseLine(line)
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

def runLogger(port_name):
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
                processLine(line, message_log)

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

def openFile(file_name):
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
        file_name = processCsv(file_name)

    return file_name