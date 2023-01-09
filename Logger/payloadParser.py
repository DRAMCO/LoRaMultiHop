import config
from datetime import datetime

# parse a line
def parseLine(line_str):
    parts = line_str.rstrip().split(' ')
    # read messageUid
    message_uid = parts[config.MESG_FIRST_BYTE_OFFSET + config.HEADER_MESG_UID_OFFSET] + parts[config.MESG_FIRST_BYTE_OFFSET + config.HEADER_MESG_UID_OFFSET + 1]
    # read messageType
    message_type = parts[config.MESG_FIRST_BYTE_OFFSET + config.HEADER_TYPE_OFFSET]
    # read hops
    hops = parts[config.MESG_FIRST_BYTE_OFFSET + config.HEADER_HOPS_OFFSET]
    # cumulative lqi
    lqi = parts[config.MESG_FIRST_BYTE_OFFSET + config.HEADER_LQI_OFFSET]
    # read nextPreviousUid
    if config.NODE_UID_SIZE == 2:
        destination_uid = parts[config.MESG_FIRST_BYTE_OFFSET + config.HEADER_NEXT_UID_OFFSET] + parts[config.MESG_FIRST_BYTE_OFFSET + config.HEADER_NEXT_UID_OFFSET + 1]  # should always be 0000
    else:
        destination_uid = parts[config.MESG_FIRST_BYTE_OFFSET + config.HEADER_NEXT_UID_OFFSET]
    # read payload
    print(parts)
    payload = parsePayload(parts[(config.MESG_FIRST_BYTE_OFFSET + config.HEADER_PAYLOAD_OFFSET):], 0)

    # general info for this message
    message_info = {
        "message_uid": message_uid,
        "message_type": message_type,
        "hops": hops,
        "lqi": lqi,
        "destination_uid": destination_uid,
        "payload": payload,
        "timestamp": datetime.now().strftime("%Y%m%d-%H%M%S"),
        "raw_message": ' '.join(parts[config.MESG_FIRST_BYTE_OFFSET:])
    }

    return message_info


# parse the rest of a line
def parsePayload(line_parts, level):
    if not len(line_parts) > 1:
        print("Error parsing appended payload (not enough fields remaining)")
        return {}, []

    offset = 0

    payload = []
    while offset < len(line_parts):
        pl = line_parts[offset:]

        if config.NODE_UID_SIZE == 2:
            source_uid = pl[config.PAYLOAD_NODE_UID_OFFSET] + pl[config.PAYLOAD_NODE_UID_OFFSET+1]
        else:
            source_uid = pl[config.PAYLOAD_NODE_UID_OFFSET+0]

        try:
            length_field = int(pl[config.PAYLOAD_LEN_OFFSET], 16)
        except ValueError:
            print("ERROR: length field is not an integer -> ", pl[config.NODE_UID_SIZE])
        except IndexError:
            print("ERROR: index error")

        # extract length of forwarded data (5 Msb) and own data (3 Lsb) from the length field
        own_data_length = (length_field & 0x07)

        # # TODO: dirty fix for 12 byte own data bug
        # if own_data_length == 4:
        #     own_data_length = 12
        #
        forwarded_data_length = ((length_field >> 3) & 0x1F)
        # # TODO: dirty fix for overflow bug
        if forwarded_data_length > 0 and forwarded_data_length < 8:
            forwarded_data_length += 32

        # To further fix overflow bug, we can look at remaining message bytes for last forward
        if level == 0:
            forwarded_data_length = len(pl) - own_data_length - config.PAYLOAD_DATA_OFFSET
        if config.IGNORE_ROUTE:
            forwarded_data_length = len(pl) - own_data_length - config.PAYLOAD_DATA_OFFSET

        own_data = pl[config.PAYLOAD_DATA_OFFSET:(config.PAYLOAD_DATA_OFFSET+own_data_length)]
        if forwarded_data_length > 0:
            print(pl)
            forwarded_data = pl[(config.PAYLOAD_DATA_OFFSET+own_data_length):(config.PAYLOAD_DATA_OFFSET+own_data_length+forwarded_data_length)]
            if len(forwarded_data) == 0:
                print("trouble")
            forwarded_payload = parsePayload(forwarded_data, level+1)
        else:
            forwarded_data = []
            forwarded_payload = []


        offset = offset + config.PAYLOAD_DATA_OFFSET + own_data_length + forwarded_data_length

        payload.append(
            {
                "source_uid": source_uid,
                "payload_length": own_data_length,
                "payload_data": own_data,
                "forwarded_payload": forwarded_payload
            }
        )

    return payload

def getUidsFromPayload(payload_data):
    uid = []
    for single_payload in payload_data:
        uid.append(single_payload["source_uid"])
        fwp = single_payload["forwarded_payload"]
        if len(fwp) > 0:
            uids = getUidsFromPayload(fwp)
            for u in uids:
                uid.append(u)

    return uid