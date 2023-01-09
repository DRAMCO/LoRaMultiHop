import logFile
import json
import io


def analyzePayload(payload, node_statistics):
    # multiple forwarded payloads on a same distance
    try:
        addToPayloadList(payload["source_uid"], payload["payload_data"], node_statistics)
    except KeyError:
        print("ERROR: Trouble with keys")

    if len(payload["forwarded_payload"]) > 0:
        for e in payload["forwarded_payload"]:
            if len(e) > 0:
                analyzePayload(e, node_statistics)


def addToPayloadList(source_uid, own_data, node_statistics):
    if len(own_data) > 2 and source_uid != "00":
        # if source_uid == "20":
        #    print(str(own_data))

        if source_uid not in node_statistics.keys():
            # First payload of this uid
            node_statistics[source_uid] = {"last_payload": int("" + own_data[0] + own_data[1], 16),
                                           "first_payload": int("" + own_data[0] + own_data[1], 16),
                                           "lost_payloads": 0,
                                           "resets": 0,
                                           "arrived_payloads": 1,
                                           "resent_payloads": 0,
                                           "packet_delivery_ratio": 0,
                                           "only_forwarded_data": 0}
        else:
            payload_previous = node_statistics[source_uid]["last_payload"]
            node_statistics[source_uid]["last_payload"] = int("" + own_data[0] + own_data[1], 16)
            node_statistics[source_uid]["arrived_payloads"] += 1

            if payload_previous is None:
                print(payload_previous)
                print(node_statistics[source_uid]["last_payload"])
                payload_previous = node_statistics[source_uid]["last_payload"]
                print("Trouble")

            # Process missed payloads
            if node_statistics[source_uid]["last_payload"] - payload_previous > 1:
                # We lost some in between packets (in counting up)
                node_statistics[source_uid]["lost_payloads"] += node_statistics[source_uid][
                                                                    "last_payload"] - payload_previous - 1
            elif node_statistics[source_uid]["last_payload"] - payload_previous < -max(10, payload_previous / 100):
                # Reset must have happened, our counter is lower than previous by at least 10 or 1% of counter
                node_statistics[source_uid]["resets"] += 1
                # When reset, starts counting at 0, so first payload is counter for lost payloads
                node_statistics[source_uid]["lost_payloads"] += node_statistics[source_uid]["last_payload"]
            elif node_statistics[source_uid]["last_payload"] == payload_previous:
                # When payload is received twice in a row
                node_statistics[source_uid]["resent_payloads"] += 1
            node_statistics[source_uid]["packet_delivery_ratio"] = node_statistics[source_uid]["arrived_payloads"] / (
                        node_statistics[source_uid]["lost_payloads"] + node_statistics[source_uid]["arrived_payloads"])
    else:
        if source_uid not in node_statistics.keys():
            # First payload of this uid
            node_statistics[source_uid] = {"last_payload": None,
                                           "first_payload": None,
                                           "lost_payloads": 0,
                                           "resets": 0,
                                           "arrived_payloads": 1,
                                           "resent_payloads": 0,
                                           "packet_delivery_ratio": 0,
                                           "only_forwarded_data": 1}
        else:
            node_statistics[source_uid]["only_forwarded_data"] += 1


def runAnalysis(file_name):
    payload_list = {}
    payload_last = {}
    node_statistics = {}

    analysed_data = {
        "nr_of_nodes": 0,
        "node_statistics": []
    }

    node_statistics = {}

    with open(logFile.openFile(file_name)) as json_file:
        data = json.load(json_file)
        print("Total number of messages: " + str(data["nr_of_messages"]))

        # first we look for all the different nodes we can find
        for message in data["messages"]:
            payload = message["payload"]
            for e in payload:
                analyzePayload(e, node_statistics)

        analysed_data["node_statistics"] = node_statistics
        analysed_data["nr_of_nodes"] = len(node_statistics)

        print(node_statistics)

        resultFilename = file_name.replace(".csv", "").replace(".json", "")
        with io.open(resultFilename + "-analysis.json", 'w', encoding='utf-8') as json_file:
            json_data = json.dumps(node_statistics, ensure_ascii=False, indent=4)
            json_file.write(json_data)


def runSignalAnalysis(file_name):
    try:
        with open(logFile.openFile(file_name)) as json_file:
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
                signal_quality_list["connections"][message["last_sender_uid"]].append(
                    {"packet_rssi": message["packet_rssi"], "packet_snr": message["packet_snr"],
                     "packet_rss": message["packet_rss"]})

            resultFilename = file_name.replace(".csv", "").replace(".json", "")
            with io.open(resultFilename + "-signal-analysis.json", 'w', encoding='utf-8') as json_file:
                json_data = json.dumps(signal_quality_list, ensure_ascii=False, indent=4)
                json_file.write(json_data)

    except ValueError as err:
        print("Wrong json format?")
