# LoRaMultiHop
 

## Network overview
<img src="data:image/svg+xml;charset=UTF-8,%3Csvg overflow='hidden' viewBox='0 0 2108 1423' xml:space='preserve' xmlns='http://www.w3.org/2000/svg'%3E%3Cdefs%3E%3CclipPath id='d'%3E%3Crect x='-14' y='706' width='2108' height='1423'/%3E%3C/clipPath%3E%3CclipPath id='c'%3E%3Crect x='964' y='1046' width='194' height='192'/%3E%3C/clipPath%3E%3CclipPath id='b'%3E%3Crect x='964' y='1046' width='194' height='192'/%3E%3C/clipPath%3E%3CclipPath id='a'%3E%3Crect x='964' y='1046' width='194' height='192'/%3E%3C/clipPath%3E%3C/defs%3E%3Cg transform='translate(14 -706)' clip-path='url(%23d)'%3E%3Crect x='811' y='1238' width='500' height='165' fill='%234472C4'/%3E%3Ctext transform='translate(934.37 1345)' fill='%23FFFFFF' font-family='Open Sans,Open Sans_MSFontService,sans-serif' font-size='73' font-weight='400'%3EROOT%3C/text%3E%3Cg clip-path='url(%23c)'%3E%3Cg clip-path='url(%23b)'%3E%3Cg clip-path='url(%23a)'%3E%3Cpath transform='translate(965 -782.53)' d='m80.64 1872.7c-23.971 0-40.739 21.52-38.58 42.78-14.118 2.77-24.78 15.26-24.78 30.18 0 16.97 13.754 30.72 30.72 30.72h94.08c19.524 0 32.64-12.81 32.64-28.8 0-13.36-9.419-26.52-23.04-28.8 0-23.04-21.12-32.64-36.48-24.96-5.76-11.52-18.149-21.12-34.56-21.12z' fill='%234472C4'/%3E%3C/g%3E%3C/g%3E%3C/g%3E%3Cpath d='m8.9999 784c0-43.078 34.922-78 78-78 43.078 0 78 34.922 78 78s-34.922 78-78 78c-43.078 0-78-34.922-78-78z' fill='%23ED7D31' fill-rule='evenodd'/%3E%3Ctext transform='translate(63.594 813)' fill='%23FFFFFF' font-family='Open Sans,Open Sans_MSFontService,sans-serif' font-size='83' font-weight='400'%3E1%3C/text%3E%3Cpath d='m-14 1398c0-43.08 34.922-78 78-78 43.078 0 78 34.92 78 78s-34.922 78-78 78c-43.078 0-78-34.92-78-78z' fill='%23ED7D31' fill-rule='evenodd'/%3E%3Ctext transform='translate(40.677 1426)' fill='%23FFFFFF' font-family='Open Sans,Open Sans_MSFontService,sans-serif' font-size='83' font-weight='400'%3E3%3C/text%3E%3Cpath d='m426 1114.5c0-42.8 34.922-77.5 78-77.5s78 34.7 78 77.5-34.922 77.5-78 77.5-78-34.7-78-77.5z' fill='%23ED7D31' fill-rule='evenodd'/%3E%3Ctext transform='translate(480.68 1143)' fill='%23FFFFFF' font-family='Open Sans,Open Sans_MSFontService,sans-serif' font-size='83' font-weight='400'%3E2%3C/text%3E%3Cpath d='m481 1953c0-43.08 34.922-78 78-78s78 34.92 78 78-34.922 78-78 78-78-34.92-78-78z' fill='%23ED7D31' fill-rule='evenodd'/%3E%3Ctext transform='translate(535.68 1982)' fill='%23FFFFFF' font-family='Open Sans,Open Sans_MSFontService,sans-serif' font-size='83' font-weight='400'%3E4%3C/text%3E%3Cpath d='m1079 1704c0-43.08 34.92-78 78-78s78 34.92 78 78-34.92 78-78 78-78-34.92-78-78z' fill='%23ED7D31' fill-rule='evenodd'/%3E%3Ctext transform='translate(1133.5 1732)' fill='%23FFFFFF' font-family='Open Sans,Open Sans_MSFontService,sans-serif' font-size='83' font-weight='400'%3E5%3C/text%3E%3Cpath d='m1437 2029c0-43.08 34.7-78 77.5-78s77.5 34.92 77.5 78-34.7 78-77.5 78-77.5-34.92-77.5-78z' fill='%23ED7D31' fill-rule='evenodd'/%3E%3Ctext transform='translate(1491 2058)' fill='%23FFFFFF' font-family='Open Sans,Open Sans_MSFontService,sans-serif' font-size='83' font-weight='400'%3E6%3C/text%3E%3Cpath d='m1766 1557c0-43.08 34.92-78 78-78s78 34.92 78 78-34.92 78-78 78-78-34.92-78-78z' fill='%23ED7D31' fill-rule='evenodd'/%3E%3Ctext transform='translate(1820.3 1586)' fill='%23FFFFFF' font-family='Open Sans,Open Sans_MSFontService,sans-serif' font-size='83' font-weight='400'%3E8%3C/text%3E%3Cpath d='m1938 1899.5c0-42.8 34.92-77.5 78-77.5s78 34.7 78 77.5-34.92 77.5-78 77.5-78-34.7-78-77.5z' fill='%23ED7D31' fill-rule='evenodd'/%3E%3Ctext transform='translate(1992.5 1928)' fill='%23FFFFFF' font-family='Open Sans,Open Sans_MSFontService,sans-serif' font-size='83' font-weight='400'%3E7%3C/text%3E%3Cpath d='m1766 883c0-43.078 34.92-78 78-78s78 34.922 78 78-34.92 78-78 78-78-34.922-78-78z' fill='%23ED7D31' fill-rule='evenodd'/%3E%3Ctext transform='translate(1820.3 912)' fill='%23FFFFFF' font-family='Open Sans,Open Sans_MSFontService,sans-serif' font-size='83' font-weight='400'%3E9%3C/text%3E%3Cpath d='m165.5 862.5 229.17 174.2' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3Cpath transform='matrix(1 0 0 -1 165.5 1352.2)' d='m0 0 241.94 159.73' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3Cpath transform='matrix(1 0 0 -1 631.5 1850.3)' d='m0 0 334.44 395.76' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3Cpath d='m812.06 1193.3-180.56-50.76' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3Cpath transform='matrix(1 0 0 -1 686.5 1905.6)' d='m0 0 375.35 123.06' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3Cpath d='m1145.6 1595.2-84.1-141.69' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3Cpath d='m1437.2 1952.1-196.74-169.58' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3Cpath d='m1733 1479.6-394.51-127.06' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3Cpath transform='matrix(-1 0 0 1 1907.7 1916.5)' d='m0 0 291.16 87.155' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3Cpath d='m1893.5 1650.5 74.59 140.88' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3Cpath transform='matrix(-1 0 0 1 1732.7 948.5)' d='m0 0 423.21 238.01' fill='none' fill-rule='evenodd' stroke='%237F7F7F' stroke-miterlimit='8' stroke-width='1.1458'/%3E%3C/g%3E%3C/svg%3E%0A">

### Beacons
The network will be distilled from a mesh/star-type network to a tree/star network first. This is done by sending out beacon messages. These beacon messages are sent out from the central node (gateway). In this message, the `next_uid` field is filled `node_uid` of the sender. When a sensor node receives the beacon message from the gateway, it saves the uid in the `next_uid` field (when direct connection to the gateway: `0x00`). After this, the sensor node will adapt the `next_uid` to its own `node_uid` (and increase the hop count) and forward the beacon message further into the network. 

When multiple beacons are received on a node, only the best beacon message is saved. This decision is based on the amount of hops to the gateway (recorded in the beacon message) and the SNR value of the received beacon. At the moment, an SNR value difference of at least 10dB is required to outperform a beacon with only one hop extra.

When this process is complete, all sensor nodes should have saved a `node_uid`, which states the most direct connection to the gateway. This process should be repeated every so often, to assure network stability. 

| 0        | 2    | 3    | 4   | 5   | 6    | 7+   |
| -------- |----- | ---- | ----| ----| ---- | ---- |
| MESG_UID | TYPE | HOPS | SRC | DST | LEN  | DATA |

### Routed messages
Routed messages find their way through the gateway by using the proposed routing protocol. Each sensor node will forward packages, if the packages have not been received yet and if the `next_uid` matches its own `node_uid`. To improve energy efficiency, each sensor node will wait for a predefined time: appending extra incoming (own/forwarded) data to be appended to the to-forward-message. 

**When to transmit** A node will start its procedure to transmit a message on 2 occasions: (1) timer, own sensor data is ready or (2) incoming data to be forwarded. Either way, the node will wait for a predefined time for other packages to come in: between `PRESET_MAX_LATENCY` and `PRESET_MIN_LATENCY`. 
```
    START                             START+PRESET_XX_LATENCY
------|----------------------------------------|------------->
                possible incoming 
                messages to append
```

The `PRESET_XX_LATENCY` is calculated based on both random values and a history of sending or forwarding data. When the node does not receive a lot of to-forward-messages, the `PRESET_XX_LATENCY` will evolve towards `PRESET_MIN_LATENCY`, otherwise (lots of to-forwarded-messages) towards `PRESET_MIN_LATENCY`. The step up/down is defined by `PRESET_LATENCY_UP_STEP`/`PRESET_LATENCY_DOWN_STEP`. This latency is also randomized in a window of +/- `PRESET_MAX_LATENCY_RAND_WINDOW`.

**Message format**
Messages that need forwarding, and arrive inside the latency window, will be encapsulated inside the message that was waiting to be send. 


| 0        | 2    | 3    | 4                       | 5        | 6                                              | 7         | 9               |
| -------- |----- | ---- | ----------------------- | -------- | -----------------------------------------------| --------- | --------------- |
| MESG_UID | TYPE | HOPS | NEXT_UID = PREVIOUS_UID | NODE_UID | LEN = 4 bit own length, 5bit forwarded length  | OWN DATA  | FORWARDED  <table><thead><tr><th>9</th><th>10</th><th>11</th><th>12</th></tr></thead><tbody><tr><td>NODE_UID</td><td>LEN</td><td>OWN DATA</td><td>FORWARDED<table><thead><tr><th>11</th><th>12</th><th>13</th><th>14</th></tr></thead><tbody><tr><td>NODE_UID</td><td>LEN</td><td>OWN</td><td>FORWARDED</td></tr></tbody></table></td></tr></tbody></table>  |



### Power consumption analysis
| 0        | Current    | Duration    | Remark    |
| -------- |----------- | ----------- | --------- |
| Sleep    | 5.11uA 		| - 		  |  |
| Wake & Stabilize    | 3.42mA 		| 31ms, 1+CAD_STABILIZE 		  | Note: can be shorter? |
| CAD perform    | 18.7mA 		| 7.64ms 		  |  |
| CAD processing    | 15.2mA 		| 5ms 		  |  |
| RX    | 29.8mA 		| - 		  |  |
| TX    | 62.9mA 		| - 		  |  |
| Message processing    | 14.6mA 		| 5.56ms 		  |  |

![Current profile](https://github.com/DRAMCO/LoRaMultiHop/blob/main/Measurements/current.png?raw=true "Current profile")
