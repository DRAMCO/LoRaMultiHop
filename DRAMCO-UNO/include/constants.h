#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#if NODE_UID == 0x00
    #define COMPILE_FOR_GATEWAY
#else
    #define COMPILE_FOR_SENSOR
#endif

#if NODE_UID == 0x01
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA
    #endif 
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x02
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 
    #endif 
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x03
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x00
        #define FIXED_HOPS 0
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x04
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x20
        #define FIXED_HOPS 1
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x05
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x20
        #define FIXED_HOPS 1
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x06
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x20
        #define FIXED_HOPS 1
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x07
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x04
        #define FIXED_HOPS 2
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x08
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x04
        #define FIXED_HOPS 2
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x09
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x04
        #define FIXED_HOPS 2
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x10
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x04
        #define FIXED_HOPS 2
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x11
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x06
        #define FIXED_HOPS 2
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x12
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    // #define HARDWARE_VERSION 1
#elif NODE_UID == 0x13
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x09
        #define FIXED_HOPS 3
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x14
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x09
        #define FIXED_HOPS 3
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x15
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x13
        #define FIXED_HOPS 4
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x16
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    // #define HARDWARE_VERSION 1
#elif NODE_UID == 0x17
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x22
        #define FIXED_HOPS 1
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x18
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x09
        #define FIXED_HOPS 3
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x19
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x20
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x00
        #define FIXED_HOPS 0
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x21
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x00
        #define FIXED_HOPS 0
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x22
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x05
        #define FIXED_HOPS 2
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x23
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x24
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    // #define HARDWARE_VERSION 1
#elif NODE_UID == 0x25
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x26
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x13
        #define FIXED_HOPS 4
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x27
    #ifdef USE_FIXED_ROUTE
        #define FIXED_VIA 0x26
        #define FIXED_HOPS 5
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x28
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x29
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    #define HARDWARE_VERSION 1
#elif NODE_UID == 0x30
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x31
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    #define HARDWARE_VERSION 2
#elif NODE_UID == 0x32
    #ifdef USE_FIXED_ROUTE
        // #define FIXED_VIA 0x20
    #endif
    #define HARDWARE_VERSION 1
#endif

#endif /*__CONSTANTS_H__*/
