#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#if NODE_UID == 0x00
    #define COMPILE_FOR_GATEWAY
#else
    #define COMPILE_FOR_SENSOR
#endif

#if NODE_UID == 0x01
    // #define FIXED_VIA 
    // #define HARDWARE_VERSION
#elif NODE_UID == 0x02
    // #define FIXED_VIA 
    // #define HARDWARE_VERSION
#elif NODE_UID == 0x03
    #define FIXED_VIA 0x00
    #define FIXED_HOPS 0
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x04
    #define FIXED_VIA 0x20
    #define FIXED_HOPS 1
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x05
    #define FIXED_VIA 0x20
    #define FIXED_HOPS 1
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x06
    #define FIXED_VIA 0x20
    #define FIXED_HOPS 1
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x07
    #define FIXED_VIA 0x04
    #define FIXED_HOPS 2
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x08
    #define FIXED_VIA 0x04
    #define FIXED_HOPS 2
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x09
    #define FIXED_VIA 0x04
    #define FIXED_HOPS 2
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x10
    #define FIXED_VIA 0x04
    #define FIXED_HOPS 2
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x11
    #define FIXED_VIA 0x06
    #define FIXED_HOPS 2
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x12
    // #define FIXED_VIA 0x20
    // #define HARDWARE_VERSION 1
#elif NODE_UID == 0x13
    #define FIXED_VIA 0x09
    #define FIXED_HOPS 3
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x14
    #define FIXED_VIA 0x09
    #define FIXED_HOPS 3
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x15
    #define FIXED_VIA 0x13
    #define FIXED_HOPS 4
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x16
    // #define FIXED_VIA 0x20
    // #define HARDWARE_VERSION 1
#elif NODE_UID == 0x17
    #define FIXED_VIA 0x22
    #define FIXED_HOPS 1
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x18
    #define FIXED_VIA 0x09
    #define FIXED_HOPS 3
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x19
    // #define FIXED_VIA 0x20
    // #define HARDWARE_VERSION 2
#elif NODE_UID == 0x20
    #define FIXED_VIA 0x00
    #define FIXED_HOPS 0
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x21
    #define FIXED_VIA 0x00
    #define FIXED_HOPS 0
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x22
    #define FIXED_VIA 0x05
    #define FIXED_HOPS 2
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x23
    // #define FIXED_VIA 0x20
    // #define HARDWARE_VERSION 1
#elif NODE_UID == 0x24
    // #define FIXED_VIA 0x20
    // #define HARDWARE_VERSION 1
#elif NODE_UID == 0x25
    // #define FIXED_VIA 0x20
    // #define HARDWARE_VERSION 1
#elif NODE_UID == 0x26
    #define FIXED_VIA 0x13
    #define FIXED_HOPS 4
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x27
    #define FIXED_VIA 0x26
    #define FIXED_HOPS 5
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x28
    // #define FIXED_VIA 0x20
    // #define HARDWARE_VERSION 1
#elif NODE_UID == 0x29
    // #define FIXED_VIA 0x20
    // #define HARDWARE_VERSION 1
#elif NODE_UID == 0x30
    // #define FIXED_VIA 0x20
    // #define HARDWARE_VERSION 1
#endif

#endif /*__CONSTANTS_H__*/
