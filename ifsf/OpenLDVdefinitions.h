//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//	OpenLDV Developer Example
/// <OpenLDV-Example Revision="1"/>
//	Copyright (c) 2004 Echelon Corporation. All rights reserved.
//
//	This file is defined as Example Software in, and use governed by, the
//	OpenLDV Software License Agreement.
//
//	ECHELON MAKES NO REPRESENTATION, WARRANTY, OR CONDITION OF
//	ANY KIND, EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE OR IN
//	ANY COMMUNICATION WITH YOU, INCLUDING, BUT NOT LIMITED TO,
//	ANY IMPLIED WARRANTIES OF MERCHANTABILITY, SATISFACTORY
//	QUALITY, FITNESS FOR ANY PARTICULAR PURPOSE,
//	NONINFRINGEMENT, AND THEIR EQUIVALENTS.
//
//	OpenLDVdefinitions.h: header file containing commonly used type
//	definitions, enumerations and constants useful to access and use
//	the OpenLDV API.
//
//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#pragma once
#ifndef _NI_COMM_H
#define _NI_COMM_H

//
//	Note the types and structures defined here must match the ANSI/EIA/CEA 709.1
//	protocol specification. Data types defined there (or in the related literature)
//	typically assume a data packing of 1 (one) byte, a padding of 0 (zero), and
//	big-endian ordering for both aggregated types and bitfields.
//	When adding more definitions, when using this header file with a compiler
//	other than Microsoft Visual Studio .NET 2003, or when porting these definitions
//	to a programming language other than ANSI-C or ANSI-C++, care must be taken to
//	model these types such that the host compiler produces the layout and alignment
//	required by protocol.
//
#pragma pack(push,1)

#ifdef __cplusplus
extern "C"
{            /* Assume C declarations for C++ */
    namespace ldv
    {
#endif  /* __cplusplus */

#define niDEFAULT_WAIT_TIME 10			/* Seconds */
#define	NEURON_ID_LEN		6			/* 6 bytes per Neuron ID */
#define	PROGRAM_ID_LEN		8			/* 8 bytes per program ID */
#define DOMAIN_ID_LEN		6			// maximum domain ID length (possible values: 0, 1, 3, 6)
#define	AUTH_KEY_LEN		6			// authentication key size per domain table index

        typedef unsigned char	Byte;
        typedef unsigned short	Word;
        typedef Byte			Bits;
        typedef Byte			Bool;

        /*
        ****************************************************************************
        * Application buffer structures for sending and receiving messages to and
        * from a network interface.  The 'ExpAppBuffer' and 'ImpAppBuffer'
        * structures define the application buffer structures with and without
        * explicit addressing.  These structures have up to four parts:
        *
        *   Network Interface Command (NI_Hdr)                        (2 Bytes)
        *   Message Header (MsgHdr)                                   (3 Bytes)
        *   Network Address (ExplicitAddr)                            (11 Bytes)
        *   Data (MsgData)                                            (varies)
        *
        * Network Interface Command (NI_Hdr):
        *
        *   The network interface command is always present.  It contains the
        *   network interface command and queue specifier.  This is the only
        *   field required for local network interface commands such as niRESET.
        *
        * Message Header (MsgHdr: union of NetVarHdr and ExpMsgHdr):
        *
        *   This field is present if the buffer is a data transfer or a completion
        *   event.  The message header describes the type of LONTALK message
        *   contained in the data field.
        *
        *   NetVarHdr is used if the message is a network variable message and
        *   network interface selection is enabled.
        *
        *   ExpMsgHdr is used if the message is an explicit message, or a network
        *   variable message and host selection is enabled (this is the default
        *   for the SLTA).
        *
        * Network Address (ExplicitAddr:  SendAddrDtl, RcvAddrDtl, or RespAddrDtl)
        *
        *   This field is present if the message is a data transfer or completion
        *   event, and explicit addressing is enabled.  The network address
        *   specifies the destination address for downlink application buffers,
        *   or the source address for uplink application buffers.  Explicit
        *   addressing is the default for the SLTA.
        *
        *   SendAddrDtl is used for outgoing messages or NV updates.
        *
        *   RcvAddrDtl is used  for incoming messages or unsolicited NV updates.
        *
        *   RespAddrDtl is used for incoming responses or NV updates solicited
        *   by a poll.
        *
        * Data (MsgData: union of UnprocessedNV, ProcessedNV, and ExplicitMsg)
        *
        *   This field is present if the message is a data transfer or completion
        *   event.
        *
        *   If the message is a completion event, then the first two Bytes of the
        *   data are included.  This provides the NV index, the NV selector, or the
        *   message code as appropriate.
        *
        *   UnprocessedNV is used if the message is a network variable update, and
        *   host selection is enabled. It consists of a two-Byte header followed by
        *   the NV data.
        *
        *   ProcessedNV is used if the message is a network variable update, and
        *   network interface selection is enabled. It consists of a two-Byte header
        *   followed by the NV data.
        *
        *   ExplicitMsg is used if the message is an explicit message.  It consists
        *   of a one-Byte code field followed by the message data.
        *
        * Note - the fields defined here are for a little-endian (Intel-style)
        * host processor, such as the 80x86 processors used in PC compatibles.
        * Bit fields are allocated right-to-left within a Byte.
        * For a big-endian (Motorola-style) host, bit fields are typically
        * allocated left-to-right.  For this type of processor, reverse
        * the bit fields within each Byte.  Compare the NEURON C include files
        * ADDRDEFS.H and MSG_ADDR.H, which are defined for the big-endian NEURON
        * CHIP.
        ****************************************************************************
        */


        /*
        ****************************************************************************
        * Network Interface Command data structure.  This is the application-layer
        * header used for all messages to and from a LONWORKS network interface.
        ****************************************************************************
        */
        typedef enum
        {               //  possible return codes from Ni* functions.
            NI_OK = 0,
            NI_NO_DEVICE,
            NI_DRIVER_NOT_OPEN,
            NI_DRIVER_NOT_INIT,
            NI_DRIVER_NOT_RESET,
            NI_DRIVER_ERROR,
            NI_NO_RESPONSES,
            NI_RESET_FAILS,
            NI_TIMEOUT,
            NI_UPLINK_CMD,
            NI_INTERNAL_ERR,
            NI_FILE_OPEN_ERR,

            // Add new error codes above here:
            NI_NUM_ERRS
        } NICode;
         // domain_struct is used by both the query domain and the update domain commands
        typedef struct {
            Byte        id[DOMAIN_ID_LEN];
            Byte        subnet;
            Bits        node        : 7;
            Bits        normal		: 1;    // this bit must be set to 1 for a regular domain configuration, and 0 for clone domain
            Byte        len;
            Byte        key[AUTH_KEY_LEN];
        } domain_struct;
        // The outgoing data to request an update to the domain table:
       typedef struct {
         //   Byte            code;
            Byte            index;
            domain_struct	domain;
        } NM_update_domain_request;

        typedef struct{
            Byte            change;
            Byte            mode;
        } NM_set_node_mode_request;
        // TNM_query_domain_response describes the response to a query_id request; see FindDevices() for a detailed example.
        typedef struct {
            Byte code;
            Byte nid[NEURON_ID_LEN], pid[PROGRAM_ID_LEN];
        } TNM_query_domain_response;

        // heartbeat
        typedef struct{
            Byte            subnet;
            Byte            node;
            Byte            domain;
            Byte            domain2;
        } NM_heart_beat;

        typedef struct sendMsq
        {
            Byte LNAR;
            Byte LNAO;
            Byte IFSF_MC;
            Byte BL;
            Byte M_St;
            Byte M_lg;
            Byte DB_AD_lg;
            Byte DB_D;
            Byte DATA_ID;
            Byte DATA_Lg;
            Byte Data_El;
        };
         typedef enum
        {
            APPL_OFFLINE	= 0,	// soft-offline
            APPL_ONLINE		= 1,
            APPL_RESET		= 2,
            CHANGE_STATE	= 3
        } ENodeMode;

        typedef enum
        {
            NO_CHANGE		= 0,
            APPL_UNCNFG		= 2,
            NO_APPL_UNCNFG  = 3,
            CNFG_ONLINE		= 4,
            CNFG_OFFLINE	= 6		// hard-offline
        } ENodeChangeState;
          /* Literals for the 'cmd.q.queue' nibble of NI_Hdr. */
        typedef enum
        {
                niTQ          = 2,     /* Transaction queue                     */
                niTQ_P        = 3,     /* Priority transaction queue            */
                niNTQ         = 4,     /* Non-transaction queue                 */
                niNTQ_P       = 5,     /* Priority non-transaction queue        */
                niRESPONSE    = 6,     /* Response msg & completion event queue */
                niINCOMING    = 8      /* Received message queue                */
        } NI_Queue;

        /* Literals for the 'cmd.q.q_cmd' nibble of NI_Hdr. */

        typedef enum
        {
                niCOMM    = 1,    /* Data transfer to/from network        */
                niNETMGMT = 2     /* Local network management/diagnostics */
        } NI_QueueCmd;

        /* Literals for the 'cmd.noq' Byte of NI_Hdr. */

        typedef enum
        {
                niNULL           = 0x00,
                niTIMEOUT        = 0x30,        /* Not used */
                niCRC            = 0x40,        /* Not used */
                niRESET          = 0x50,
                niFLUSH_COMPLETE = 0x60,        /* Uplink   */
                niFLUSH_CANCEL   = 0x60,        /* Downlink */
                niONLINE         = 0x70,
                niOFFLINE        = 0x80,
                niFLUSH          = 0x90,
                niFLUSH_IGN      = 0xA0,
                niSLEEP          = 0xB0,        /* SLTA only */
                niACK            = 0xC0,
                niNACK           = 0xC1,        /* SLTA only */
                niSSTATUS        = 0xE0,        /* SLTA only */
                niPUPXOFF        = 0xE1,
                niPUPXON         = 0xE2,
                niPTRHROTL       = 0xE4,        /* Not used  */
                niSERVICE        = 0xE6,
                niTXID           = 0xE8,
                niXDRVESC		 = 0xEF,		/* xDriver escape command, use XNI_EscapeCmd*/
              //  niDRV_CMD        = 0xF0         /* Not used  */
        } NI_NoQueueCmd;

        typedef enum
        {   // some selected network management command and request codes:
                NM_send_heart_beat     = 0x01,
                NM_query_id            = 0x61,
                NM_respond_to_query_id = 0x62,
                NM_update_domain       = 0x63,
                NM_leave_domain        = 0x64,
                NM_query_nv_cnfg       = 0x68,
                NM_query_domain        = 0x6A,
                NM_update_nv_cnfg      = 0x6B,
                NM_set_node_mode       = 0x6C,
                NM_write_memory        = 0x6E,
                NM_wink                = 0x70,
                NM_query_si            = 0x72,
                NM_nv_fetch            = 0x73,
                NM_service_pin         = 0x7F
        } ENMCommands;

        /* Literals for msg_hdr.noqData[] in combination with niXDRVESC */
        typedef enum
        {
                niEncryptionOnSend	= 0x02,		// enable encryption of packets to the remote NI
                niEncryptionOffSend = 0x03,		// disable encryption of packets to the remote NI
                niEncryptionOnReceive = 0x04,	// enable encryption of packets sent by the remote NI
                niEncryptionOffReceive = 0x05	// disable encryption of packets sent by the remote NI
        } XNI_EscapeCmd;

        /*
        * MAX_NETMSG_DATA specifies the maximum size of the data portion of an
        * application buffer.  MAX_NETVAR_DATA specifies the maximum size of the
        * data portion of a network variable update.  The values specified here
        * are the absolute maximums,based on the LONTALK protocol. Actual limits
        * are based on the buffer sizes defined on the attached NEURON CHIP.
        */

#define MAX_NETMSG_DATA		228	// max payload for application message
#define MAX_NETVAR_DATA		31	// max payload for network variable
#define	MAX_OPENLDV_DATA	257	// max transfer unit size to/from OpenLDV driver

        /*
        * Header for network interface messages.  The header is a union of
        * two command formats: the 'q' format is used for the niCOMM and
        * niNETMGMT commands that require a queue specification; the 'noq'
        * format is used for all other network interface commands.
        * Both formats have a length specification where:
        *
        *      length = header (3) + address field (11 if present) + data field
        *
        * WARNING:  The fields shown in this structure do NOT reflect the actual
        * structure required by the network interface.  Depending on the network
        * interface, the network driver may change the order of the data and add
        * additional fields to change the application-layer header to a link-layer
        * header.  See the description of the link-layer header in Chapter 2 of the
        * Host Application Programmer's Guide.
        */

        typedef union
        {
            struct {
                Bits queue  :4;          /* Network interface message queue      */
                /* Use value of type 'NI_Queue'         */
                Bits q_cmd  :4;          /* Network interface command with queue */
                /* Use value of type 'NI_QueueCmd'      */
                Bits length :8;          /* Length of the buffer to follow       */
            } q;                       /* Queue option                         */
            struct {
                Byte     cmd;           /* Network interface command w/o queue  */
                /* Use value of type 'NI_NoQueueCmd'    */
                Byte     length;        /* Length of the buffer to follow       */
            } noq;                     /* No queue option                      */
        } NI_Hdr;


        /*
        ****************************************************************************
        * Message Header structure for sending and receiving explicit
        * messages and network variables which are not processed by the
        * network interface (host selection enabled).
        ****************************************************************************
        */
        typedef enum
        {          // internal enumeration for addressing mode
            eLocal,
            eImplicit,
            eExplicit
        } AddrMode;
        /* Literals for 'st' fields of ExpMsgHdr and NetVarHdr. */

        typedef enum
        {
                ACKD            = 0,
                UNACKD_RPT      = 1,
                UNACKD          = 2,
                REQUEST         = 3
        } ServiceType;
        typedef Byte service_type;

        /* Literals for 'cmpl_code' fields of ExpMsgHdr and NetVarHdr. */

        typedef enum
        {
            MSG_NOT_COMPL  = 0,             /* Not a completion event            */
                MSG_SUCCEEDS   = 1,             /* Successful completion event       */
                MSG_FAILS      = 2              /* Failed completion event           */
        } ComplType;

        /* Explicit message and Unprocessed NV Application Buffer. */

        typedef struct {
            Bits   tag       :4;        /* Message tag for implicit addressing  */
            /* Magic cookie for explicit addressing */
            Bits   auth      :1;        /* 1 => Authenticated                   */
            Bits   st        :2;        /* Service Type - see 'ServiceType'     */
            Bits   msg_type  :1;        /* 0 => explicit message                */
            /*      or unprocessed NV               */
            /*--------------------------------------------------------------------------*/
            Bits   response  :1;        /* 1 => Response, 0 => Other            */
            Bits   pool      :1;        /* 0 => Outgoing                        */
            Bits   alt_path  :1;        /* 1 => Use path specified in 'path'    */
            /* 0 => Use default path                */
            Bits   addr_mode :1;        /* 1 => Explicit addressing,            */
            /* 0 => Implicit                        */
            /* Outgoing buffers only                */
            Bits   cmpl_code :2;        /* Completion Code - see 'ComplType'    */
            Bits   path      :1;        /* 1 => Use alternate path,             */
            /* 0 => Use primary path                */
            /*      (if 'alt_path' is set)          */
            Bits   priority  :1;        /* 1 => Priority message                */
            /*--------------------------------------------------------------------------*/
            Byte   length;              /* Length of msg or NV to follow        */
            /* not including any explicit address   */
            /* field, includes code Byte or         */
            /* selector Bytes                       */
        } ExpMsgHdr;

        /*
        ****************************************************************************
        * Message Header structure for sending and receiving network variables
        * that are processed by the network interface (network interface
        * selection enabled).
        ****************************************************************************
        */

        typedef struct {
            Bits   tag       :4;        /* Magic cookie for correlating         */
            /* responses and completion events      */
            Bits   rsvd0     :2;
            Bits   poll      :1;        /* 1 => Poll, 0 => Other                */
            Bits   msg_type  :1;        /* 1 => Processed network variable      */
            /*--------------------------------------------------------------------------*/
            Bits   response  :1;        /* 1 => Poll response, 0 => Other       */
            Bits   pool      :1;        /* 0 => Outgoing                        */
            Bits   trnarnd   :1;        /* 1 => Turnaround Poll, 0 => Other     */
            Bits   addr_mode :1;        /* 1 => Explicit addressing,            */
            /* 0 => Implicit addressing             */
            Bits   cmpl_code :2;        /* Completion Code - see above          */
            Bits   path      :1;        /* 1 => Used alternate path             */
            /* 0 => Used primary path               */
            /*      (incoming only)                 */
            Bits   priority  :1;        /* 1 => Priority msg (incoming only)    */
            /*--------------------------------------------------------------------------*/
            Byte   length;              /* Length of network variable to follow */
            /* not including any explicit address   */
            /* not including index and rsvd0 Byte   */
        } NetVarHdr;

        /* Union of all message headers. */

        typedef union
        {
            ExpMsgHdr  exp;
            NetVarHdr  pnv;
            Byte       noqData[3]; /* Message data for NI_NoQueueCmd */
        } MsgHdr;

        /*
        ****************************************************************************
        * Network Address structures for sending messages with explicit addressing
        * enabled.
        ****************************************************************************
        */

        /* Literals for 'type' field of destination addresses for outgoing messages. */

        typedef enum
        {
                NI_UNASSIGNED     = 0,
                NI_SUBNET_NODE    = 1,
                NI_NEURON_ID      = 2,
                NI_BROADCAST      = 3,
                NI_IMPLICIT       = 126,    /* not a real destination type */
                NI_LOCAL          = 127     /* not a real destination type */
        } AddrType;

        /* Group address structure.  Use for multicast destination addresses. */

        typedef struct {
            Bits   size      :7;        /* Group size (0 => huge group)         */
            Bits   type      :1;        /* 1 => Group                           */

            Bits   member    :7;        /* Member ID, not used on sends.        */
            Bits   domain    :1;        /* Domain index                         */

            Bits   retry     :4;        /* Retry count                          */
            Bits   rpt_timer :4;        /* Retry repeat timer                   */

            Bits   tx_timer  :4;        /* Transmit timer index                 */
            Bits   rsvd0     :4;

            Byte       group;           /* Group ID                             */
        } SendGroup;

        /* Subnet/node ID address.  Use for a unicast destination address. */
        typedef struct {
            Byte   type;                /* NI_SUBNET_NODE                          */

            Bits   node      :7;        /* Node number                          */
            Bits   domain    :1;        /* Domain index                         */

            Bits   retry     :4;        /* Retry count                          */
            Bits   rpt_timer :4;        /* Retry repeat timer                   */

            Bits   tx_timer  :4;        /* Transmit timer index                 */
            Bits   rsvd0     :4;

            Bits   subnet    :8;        /* Subnet ID                            */
        } SendSnode;

        /* 48-bit NEURON ID destination address. */
        typedef struct {
            Byte   type;                /* NI_NEURON_ID                            */

            Bits   rsvd0      :7;
            Bits   domain     :1;       /* Domain index                         */

            Bits   retry      :4;       /* Retry count                          */
            Bits   rpt_timer  :4;       /* Retry repeat timer                   */

            Bits   tx_timer   :4;       /* Transmit timer index                 */
            Bits   rsvd1      :4;

            Bits   subnet     :8;       /* Subnet ID, 0 => pass all routers     */
            Byte   nid[ NEURON_ID_LEN ];  /* NEURON ID                          */
        } SendNrnid;

        /* Broadcast destination address. */
        typedef struct {
            Byte   type;                /* NI_BROADCAST                            */

            Bits   backlog     :6;      /* Backlog                              */
            Bits   rsvd0       :1;
            Bits   domain      :1;      /* Domain index                         */

            Bits   retry       :4;      /* Retry count                          */
            Bits   rpt_timer   :4;      /* Retry repeat timer                   */

            Bits   tx_timer    :4;      /* Transmit timer index                 */
            Bits   rsvd1       :4;

            Bits   subnet      :8;      /* Subnet ID, 0 => domain-wide          */
        } SendBcast;

        /* Address formats for special host addresses.                          */
        typedef struct {
            Byte   type;                /*  NI_LOCAL         */
        } SendLocal;

        typedef struct {
            Byte    type;                /* NI_IMPLICIT */
            Byte    msg_tag;             /* address table entry number */
        } SendImplicit;

        /* Union of all destination addresses. */
        typedef union
        {
            SendGroup      gp;
            SendSnode      sn;
            SendBcast      bc;
            SendNrnid      id;
            SendLocal      lc;
            SendImplicit   im;
        } SendAddrDtl;


        /*
        ****************************************************************************
        * Network Address structures for receiving messages with explicit
        * addressing enabled.
        ****************************************************************************
        */

        /* Received subnet/node ID destination address.  Used for unicast messages. */

        typedef struct {
            Bits       subnet :8;
            Bits       node   :7;
Bits              :1;
        } RcvSnode;

        /* Received 48-bit NEURON ID destination address. */

        typedef struct {
            Byte   subnet;
            Byte   nid[NEURON_ID_LEN];
        } RcvNrnid;

        /* Union of all received destination addresses. */
        typedef union
        {
            Byte       gp;                  /* Group ID for multicast destination   */
            RcvSnode   sn;                  /* Subnet/node ID for unicast           */
            RcvNrnid   id;                  /* 48-bit NEURON ID destination address */
            Byte       subnet;              /* Subnet ID for broadcast destination, 0 => domain-wide */
        } RcvDestAddr;

        /* Source address of received message.  Identifies */
        /* network address of node sending the message.    */
        typedef struct {
            Bits   subnet  :8;
            Bits   node    :7;
            Bits           :1;
        } RcvSrcAddr;

        /* Literals for the 'format' field of RcvAddrDtl. */
        typedef enum
        {
            ADDR_RCV_BCAST  = 0,
                ADDR_RCV_GROUP  = 1,
                ADDR_RCV_SNODE  = 2,
                ADDR_RCV_NRNID  = 3
        } RcvDstAddrFormat;

        /* Address field of incoming message. */
        typedef struct {
            Bits    format      :6;     /* Destination address type             */
            /* See 'RcvDstAddrFormat'               */
            Bits    flex_domain :1;     /* 1 => broadcast to unconfigured node  */
            Bits    domain      :1;     /* Domain table index                   */
            RcvSrcAddr  source;         /* Source address of incoming message   */
            RcvDestAddr dest;           /* Destination address of incoming msg  */
        } RcvAddrDtl;

        /*
        ****************************************************************************
        * Network Address structures for receiving responses with explicit
        * addressing enabled.
        ****************************************************************************
        */

        /* Source address of response message. */

        typedef struct {
            Bits   subnet   :8;
            Bits   node     :7;
            Bits   is_snode :1;        /* 0 => Group response,   */
            /* 1 => snode response    */
        } RespSrcAddr;

        /* Destination of response to unicast request. */
        typedef struct {
            Bits   subnet   :8;
            Bits   node     :7;
Bits            :1;
        } RespSnode;

        /* Destination of response to multicast request. */

        typedef struct {
            Bits   subnet   :8;

            Bits   node     :7;
Bits            :1;

            Bits   group    :8;

            Bits   member   :6;
Bits            :2;
        } RespGroup;

        /* Union of all response destination addresses. */

        typedef union
        {
            RespSnode  sn;
            RespGroup  gp;
        } RespDestAddr;

        /* Address field of incoming response. */

        typedef struct {
Bits                 :6;
            Bits     flex_domain :1;    /* 1=> Broadcast to unconfigured node   */
            Bits     domain      :1;    /* Domain table index                   */
            RespSrcAddr  source;        /* Source address of incoming response  */
            RespDestAddr dest;          /* Destination address of incoming resp */
        } RespAddrDtl;

        /* Explicit address field if explicit addressing is enabled. */

        typedef union
        {
            RcvAddrDtl  rcv;
            SendAddrDtl snd;
            RespAddrDtl rsp;
        } ExplicitAddr;

        /*
        ****************************************************************************
        * Data field structures for explicit messages and network variables.
        ****************************************************************************
        */

        /* Data field for network variables (host selection enabled). */

        typedef struct {
            Bits   NV_selector_hi :6;
            Bits   direction      :1;     /* 1 => output NV, 0 => input NV      */
            Bits   must_be_one    :1;     /* Must be set to 1 for NV            */
            Bits   NV_selector_lo :8;
            Byte   data[MAX_NETVAR_DATA]; /* Network variable data              */
        } UnprocessedNV;

        /* Data field for explicit messages. */

        typedef struct {
            Byte       code;                  /* Message code                     */
            Byte       data[MAX_NETMSG_DATA]; /* Message data                     */
        } ExplicitMsg;

        /* Data field for any driver-specific data. This field also denotes the maximum		*
        * size of the transfer unit to and from the OpenLDV driver, which is 257 byte.		*
        * Although that maximum transfer unit is not typically used, defining the			*
        * ExpAppBuffer structure (below) such that is covers that entire maximum transfer	*
        * unit size allows for all clients of all possible OpenLDV interfaces to declare	*
        * a sufficiently large data storage.												*/

        typedef struct {
            Byte	data[MAX_OPENLDV_DATA-sizeof(ExplicitAddr)-sizeof(MsgHdr)-sizeof(NI_Hdr)];
        } LdvData;

        /* Union of all data fields. */
        typedef union
        {
            UnprocessedNV unv;
            ExplicitMsg   exp;
            LdvData		 ldv;
        } MsgData;

        /*
        ****************************************************************************
        * Message buffer types.
        ****************************************************************************
        */

        /* Application buffer when using explicit addressing. */

        typedef struct {
            NI_Hdr       ni_hdr;            /* Network interface header */
            MsgHdr       msg_hdr;           /* Message header */
            ExplicitAddr addr;              /* Network address */
            MsgData      data;              /* Message data */
        } ExpAppBuffer;

#ifdef __cplusplus
    }	// namespace
}	// extern
#endif  /* __cplusplus */

#pragma pack(pop)

#endif
