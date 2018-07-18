
#ifndef _RDP_CPU_RING_DEFS_H
#define _RDP_CPU_RING_DEFS_H

typedef enum
{
    OWNERSHIP_RUNNER,
    OWNERSHIP_HOST
}E_DESCRIPTOR_OWNERSHIP;

#ifndef _BYTE_ORDER_LITTLE_ENDIAN_
typedef struct
{
    union{
        uint32_t word0;
        struct{
            uint32_t    flow_id:12;
            uint32_t    reserved0:1;
            uint32_t    source_port:5;
            uint32_t    packet_length:14;
        };
    };
    union{
        uint32_t word1;
        struct{
            uint32_t    payload_offset_flag:1;
            uint32_t    reason:6;
            uint32_t    dst_ssid:16;
            uint32_t    reserved1:5;
            uint32_t    descriptor_type:4;
            //uint32_t  abs_flag:1;
            //uint32_t  flow_id:8;

        };
    };
    union{
        uint32_t word2;
        struct{
            uint32_t    ownership:1;
            uint32_t    reserved2:2;
            uint32_t    host_buffer_data_pointer:29;
        };
    };
    union{
        uint32_t word3;
        struct{
            uint16_t    reserved4:12;
            uint16_t    ip_sync_1588_idx:4;
            union {
                uint16_t wl_metadata;
                struct{
                    uint16_t    reserved5:4;
                    uint16_t    wl_tx_priority:4;
                    uint16_t    wl_chain_id:8;
                };
            };
        };
    };
}
CPU_RX_DESCRIPTOR;
#else
typedef struct
{
    union{
        uint32_t word0;
        struct{
            uint32_t    packet_length:14;
            uint32_t    source_port:5;
            uint32_t    reserved0:1;
            uint32_t    flow_id:12;
            //uint32_t  descriptor_type:4;
            //uint32_t  reserved0:9;
        };
    };

    union{
        uint32_t word1;
        struct{
            //uint32_t  flow_id:8;
            //uint32_t  abs_flag:1;
            uint32_t    descriptor_type:4;
            uint32_t    reserved1:5;
            uint32_t    dst_ssid:16;
            uint32_t    reason:6;
            uint32_t    payload_offset_flag:1;
        };
    };

    union{
        uint32_t word2;
                uint8_t* data_ptr;
        struct{
            uint32_t    host_buffer_data_pointer:31;
            uint32_t    ownership:1;
        };
    };
    union{
        uint32_t word3;
        struct{
            union {
                uint16_t free_index;
                uint16_t wl_metadata;
                struct {
                    uint16_t    wl_chain_id:8;
                    uint16_t    wl_tx_priority:4;
                    uint16_t    reserved5:4;
                };
            };
            uint16_t    ip_sync_1588_idx:4;
            uint16_t    reserved4:11;
            uint16_t    is_rx_offload:1;
        };
    };
}
CPU_RX_DESCRIPTOR;
#endif

#endif //_RDP_CPU_RING_DEFS_H
