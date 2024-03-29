IGNORE_ROUTE = True

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

