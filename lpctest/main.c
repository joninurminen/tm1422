#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>

#define VERSION  "V2.02-test"

#define IOCTL_VERSION   0x43504C00

// Explicite message codes for network management

#define NM_query_status                 0x51
#define NM_query_status_succ            0x31
#define NM_query_status_fail            0x11

#define NM_update_address               0x66
#define NM_update_address_succ          0x26
#define NM_update_address_fail          0x06

#define NM_query_address                0x67
#define NM_query_address_succ           0x27
#define NM_query_address_fail           0x07

#define NM_update_nv_config             0x6B
#define NM_update_nv_config_succ        0x2B
#define NM_update_nv_config_fail        0x0B

#define NM_query_nv_config              0x68
#define NM_query_nv_config_succ         0x28
#define NM_query_nv_config_fail         0x08

#define NM_update_domain                0x63
#define NM_update_domain_succ           0x23
#define NM_update_domain_fail           0x03

#define NM_leave_domain                 0x64
#define NM_leave_domain_succ            0x24
#define NM_leave_domain_fail            0x04

#define NM_query_domain                 0x6A
#define NM_query_domain_succ            0x2A
#define NM_query_domain_fail            0x0A

#define NM_read_memory                  0x6D
#define NM_read_memory_succ             0x2D
#define NM_read_memory_fail             0x0D

#define NM_write_memory                 0x6E
#define NM_write_memory_succ            0x2E
#define NM_write_memory_fail            0x0E

#define NM_set_node_mode                0x6C
#define NM_set_node_mode_succ           0x2C
#define NM_set_node_mode_fail           0x0C

#define NM_query_snvt                   0x72
#define NM_query_snvt_succ              0x32
#define NM_query_snvt_fail              0x12

#define NM_nv_fetch                     0x73
#define NM_nv_fetch_succ                0x33
#define NM_nv_fetch_fail                0x13

#define NM_wink                         0x70
#define NM_wink_succ                    0x30
#define NM_wink_fail                    0x10

#define NM_file_xfer                    0x3E

// Service types */

#define SVC_ackd            0x00
#define SVC_unackd_rpt      0x20
#define SVC_unackd          0x40
#define SVC_request         0x60
#define SVC_MASK            0x60

// Default transaction timer values

#define DEFAULT_RPTRETRY    0x43
#define DEFAULT_TXTIMER     0x05

// Network interface commands

#define niTQ                0x12
#define niTQ_P              0x13
#define niNTQ               0x14
#define niNTQ_P             0x15
#define niRESPONSE          0x16
#define niINCOMING          0x18
#define niLOCAL             0x22
#define niRESET             0x50
#define niFLUSH_CANCEL      0x60
#define niONLINE            0x70
#define niOFFLINE           0x80
#define niFLUSH_IGN         0xA0

// Mixed

#define LENMASK             0x1F
#define CFG                 0x80
#define DIR                 0x40
#define XDIR                0x4000
#define AUTH                0x10
#define PRIO                0x80
#define EXPLICIT            8

typedef unsigned char u8;

typedef struct
    {
    u8 cmq;     // cmd[7..4]                    queue[3..0]
    u8 len;
    u8 svc_tag; // 0[7] Service[6..5] auth[4]   tag[3..0]
    u8 flags;   // prio path cplcode[5..4] expl altp pool resp
    u8 data_len;
    u8 format;  // rcv: domain[7] flex[6]

    union
        {
        struct
            {
            u8 dom_node;  // domain[7] node/memb[6..0]
            u8 rpt_retry; // rpt_timer[7..4]      retry[3..0]
            u8 tx_timer;  //                      tx_timer[3..0]
            u8 dnet_grp;  // destination subnet or group
            u8 nid[6];    // NEURON ID
            } send;

        struct
            {
            u8 snet;         // source subnet
            u8 snode;        // source node
            u8 dnet_grp;     // destination subnet or group
            u8 dnode_nid[7]; // destination node or NEURON ID
            } rcv;

        struct
            {
            u8 snet;  // source subnet
            u8 snode; // source node
            u8 dnet;  // destination subnet
            u8 dnode; // destination node
            u8 group;
            u8 member;
            u8 reserved[4];
            } resp;
        } adr;
    u8 code;       // message code or selector MSB
    u8 data[239];
    } ExpAppBuf;   // Total size must be 256, else alignment mismatch


ExpAppBuf msg_out; /* Explicit message buffer for outgoing messages */
ExpAppBuf msg_in;  /* Explicit message buffer for incoming messages */
ExpAppBuf msg_rsp; /* Explicit message buffer for response messages */
int ni_handle;

u8 ha_online, min_rspcode;

void DumpBuffer(const char *Prefix, void *pvBuffer, u8 len)
    {
    u8	*ptr = (u8*) pvBuffer;
    printf("%s", Prefix);
    int pos = 0;
    while (len--)
        {
        if (pos > 0)
            {
            if ((pos & 0x1F) == 0)       printf("\n");
            else if ((pos & 0x07) == 0)  printf(" ");
            }
        pos++;
        printf(" %02X", *ptr++);
        }
    printf("\n");
    }

/*************************************************************************
 *  Function   : do_response( code, len );
 *  Parameters : code = response code.
 *               len  = number of data bytes in the reponse message.
 *  Description:
 *  Sends a response message for an incoming request. To create a response
 *  message only msg_in.data[] has to be modified because the message
 *  header is modified by the function itself.
 *
 *************************************************************************/

void do_response(u8 code, int len)
    {
    if ((msg_in.svc_tag &SVC_MASK) == SVC_request)
        {
        msg_in.cmq = (msg_in.flags &PRIO) ? niNTQ_P : niNTQ;
        msg_in.code = code;
        msg_in.len = len + 16;
        msg_in.data_len = len + 1;
        msg_in.svc_tag &= 0x6F;
        msg_in.flags = (msg_in.flags &PRIO)+0x01;
        write(ni_handle, &msg_in, len + 18);
        }
    }


/*************************************************************************
 *  Function   : handle_incoming();
 *  Description:
 *  Handles any incoming message (NV, network management or application
 *  message).
 *
 *************************************************************************/

static void handle_incoming()
    {
    if (msg_in.code & 0x80)
        {
        return;
        }

    switch (msg_in.code)
        {
        }
    }


/*************************************************************************
 *  Function   : send_msg( len );
 *  Parameters : Length of data field (message code not included).
 *  Ret.Value  : Completion code.
 *  Description:
 *  Sends any message over the network. If the NEURON has made a reset, the
 *  host application is set online and the transaction breaks.
 *  Message header and address field must be initialized while msg_out.cmq
 *  and all length are initialized by this function.
 *
 *************************************************************************/

int send_msg(int len)
    {
    int ldv_err, timeout;
    u8 cpl;

    msg_out.cmq = (msg_out.flags &PRIO) ? niTQ_P : niTQ;
    if ((msg_out.svc_tag &SVC_MASK) == SVC_unackd)
        msg_out.cmq += 2;
    msg_out.len = len + 15;
    msg_out.data_len = len + 1;

    if (write(ni_handle, &msg_out, len + 17) < 0)
        return 0;
    min_rspcode = 128;
    timeout = 1000;
#if 01

    while (timeout)
        {
        ldv_err = read(ni_handle, &msg_in, sizeof(msg_in));

        if (ldv_err >= 0)
            {
            if (msg_in.cmq == niRESET)
                { // local reset
                ha_online = 1;
                return 0;
                }
            if (msg_in.cmq == niINCOMING)
                handle_incoming();
            if (msg_in.cmq == niRESPONSE)
                {
                cpl = msg_in.flags & 0x30;

                if (cpl)
                    return (cpl >> 4); // Completion event
                else
                    {
                    if ((msg_in.svc_tag &SVC_MASK) == SVC_request)
                        {
                        memcpy(&msg_rsp, &msg_in, msg_in.len + 2);

                        if (min_rspcode > msg_rsp.code)
                            min_rspcode = msg_rsp.code; /* for FTP  */
                        }
                    }
                }
            }
        else
            {
            // Wait a little bit...
            // timeout--;
            }
        }
#else
    return 1;
#endif
    return 0;
    }


/*************************************************************************
 *  Function   : send_local( len );
 *  Parameters : Length of data field (message code not included).
 *  Ret.Value  : Completion code.
 *  Description:
 *  Same functionality as send_msg() but it is used to address the local
 *  NEURON chip. Message header and address field must not be set and
 *  Request/Response service is always used.
 *
 *************************************************************************/

int send_local(int len, int WaitForAnswer)
    {
    int ldv_err, timeout = 40;
    static u8 tag = 0;
    tag++;

    msg_out.cmq = niLOCAL;
    msg_out.svc_tag = SVC_request | (tag & 0x0F);
    msg_out.flags = 8;
    msg_out.len = len + 15;
    msg_out.data_len = len + 1;

    DumpBuffer("> ", &msg_out, len + 17);

    if (write(ni_handle, &msg_out, len + 17) < 0)
        return 0;

    if (WaitForAnswer)
      {
      if (WaitForAnswer == 2)
        sleep(3);
      while (timeout)
        {
        ldv_err = read(ni_handle, &msg_in, 256);

        if (ldv_err >= 0)
            {
            DumpBuffer("< ", &msg_in, ldv_err);

            if (msg_in.cmq == niRESET)
                { // Local reset
                ha_online = 1;
                return 0;
                }
            if (msg_in.cmq == niINCOMING)
                handle_incoming();
            if (msg_in.cmq == niRESPONSE)
                {
                memcpy(&msg_rsp, &msg_in, msg_in.len + 2);
                return 1; // Ok
                }
            }
        else
            {
            //poll( NULL, 0, 50 );
            timeout--;
            }
        }
      }
    else
      return 1;
    return 0;
    }

int recv_local()
    {
    int ldv_err, timeout = 40;

    while (timeout)
        {
        ldv_err = read(ni_handle, &msg_in, 256);

        if (ldv_err >= 0)
            {
            DumpBuffer("< ", &msg_in, ldv_err);

            if (msg_in.cmq == niRESET)
                { // Local reset
                ha_online = 1;
                return 0;
                }
            if (msg_in.cmq == niINCOMING)
                handle_incoming();
            if (msg_in.cmq == niRESPONSE)
                {
                memcpy(&msg_rsp, &msg_in, msg_in.len + 2);
                return 1; // Ok
                }
            }
        else
            {
            //poll( NULL, 0, 50 );
            timeout--;
            }
        }
    }

void init_local_node(u8 node)
    {
    msg_out.code = NM_update_domain;
    msg_out.data[0] = 1;                // Domain index 0
    memset(&msg_out.data[1], 0, 6);     // Domain ID
    msg_out.data[7] = 10;                // Subnet
    msg_out.data[8] = node | 0x80;      // Node
    msg_out.data[9] = 1;                // Domain length
    memset(&msg_out.data[10], 0xFF, 6); // Authentication key
    send_local(16, 1);

    msg_out.code = NM_set_node_mode;
    msg_out.data[0] = 3; // Set state
    msg_out.data[1] = 4; // Configured online
    send_local(2, 1);
    }

void receive_packets()
    {
    int i = 0;
    struct pollfd fds;

    fds.fd = ni_handle;
    fds.events = POLLIN;

    while (1)
        {
        msg_in.code = 0;
        fds.revents = 0;

        if (!poll(&fds, 1, 1000))
            {
            printf("%d\n", i);
            continue;
            }
        read(ni_handle, &msg_in, 256);
        i++;

        if (!(i % 10))
            printf("%d\n", i);
        }
    }

void print_usage_exit()
{
    fprintf(stderr, "lpctest - Test the network interface - Version " VERSION "\n");
    fprintf(stderr, "Usage: lpctest [-r ] [-n Anz] [-l Length] DevName\n");
    fprintf(stderr, "\t-r Receive test\n");
    fprintf(stderr, "\t-n Number of telegrams (default = 100)\n");
    fprintf(stderr, "\t-l Number of additional bytes after \"QueryStatus\" (default = 0)\n");
    fprintf(stderr, "\tDevName The name of the LON-device (e.g.: /dev/lon/lonusb0)\n");
    exit(1);
}

int main(int argc, char ** argv)
    {
    char    version[80];
    uint    i;
    int     ret;
    struct  timezone tz;
    struct  timeval t0, t1;
    int     msec;
    u8      node = 1;
    char   *DevName;
    int     DoReceive   = 0;
    uint    Anz         = 2;
    int     AddLength   = 0;
/*
    if (argc < 2)
        {
        printf("Syntax: ./lpctest n          (n=lon1..lon9) to send unackd packets\n");
        printf("    or: ./lpctest n anything (n=lon1..lon9) to count received packets\n");
        return 1;
        }
*/
    int ch, errflg = 0;

    while ((ch = getopt(argc, argv, "rn:l:")) != EOF)
        switch (ch)
        {
        case 'r':
            DoReceive = 1;
            break;
        case 'n':
            Anz = atoi(optarg);
            break;
        case 'l':
            AddLength = atoi(optarg);
            break;

        case '?':
            errflg++;
        }
    if (errflg || optind >= argc)
        {
        print_usage_exit();
        return -1;
        }
    if (optind < argc)
        {
        //printf("optind=%d argc=%d argv[%d]=%s\n", optind, argc, optind, argv[optind]);
        DevName = argv[optind];
        }

#if 0

    node = argv[1][0] - '0';

    if ((node < 1) || (node > 9))
        {
        printf("Argument must be between 1 and 9\n");
        return 1;
        }
    DevName[8] = node + '0';

#else

//    DevName = argv[1];

#endif

    printf("Open device %s\n", DevName);
    ni_handle = open(DevName, O_RDWR | O_SYNC);

    if (ni_handle < 0)
        {
        printf("Cannot open the device\n");
        return 1;
        }
    ret = ioctl(ni_handle, IOCTL_VERSION, (unsigned long) &version);

    if (ret < 0)
        printf("ioctl(IOCTL_VERSION)=%d\n", ret); else
        printf("%s\n", version);
#if 01

    ha_online = 1;

    printf("init_local_node():\n");
    init_local_node(1);






    printf("QueryStatus():\n");

    msg_out.svc_tag = SVC_unackd;
    msg_out.flags = EXPLICIT;
    msg_out.format = 1;            // Broadcast
    msg_out.adr.send.dom_node = 1; // Backlog
    msg_out.adr.send.rpt_retry = DEFAULT_RPTRETRY;
    msg_out.adr.send.tx_timer = DEFAULT_TXTIMER;
    msg_out.adr.send.dnet_grp = 1;

    //  msg_out.code = 1;
    msg_out.code = NM_query_status;
    for (i = 0; i <= sizeof(msg_out.data); i++)
        {
        msg_out.data[i] = i+1;
        }

    tz.tz_minuteswest = 0;
    tz.tz_dsttime = 0;
    gettimeofday(&t0, &tz);


    for (i = 1; i <= Anz; i++)
        {
        if( !send_local(0, 0) )
        //if (!send_msg(AddLength))
            {
            printf("send_local() failed\n");
            }

        if (!(i % 10))
            printf("%d\n", i);
        }

    for (i = 1; i <= Anz; i++)
        {
        if( !recv_local() )
        //if (!send_msg(AddLength))
            {
            printf("recv_local() failed\n");
            }

        if (!(i % 10))
            printf("%d\n", i);
        }

    gettimeofday(&t1, &tz);

    msec = (t1.tv_sec - t0.tv_sec) * 1000;
    msec += (t1.tv_usec / 1000);
    msec -= (t0.tv_usec / 1000);
    if (msec == 0)  msec = 1;

    printf("Anz=%d AddLen=%d  Zeit: %d ms. %d Transactions/s\n", Anz, AddLength, msec, (Anz * 1000) / msec);
//DumpBuffer( &msg_rsp.code, msg_rsp.data_len );
#endif
/*
    char msg_out[] = {0x14,0x13,0x40,0x08,0x05,0x03,0x00,0x04,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x0A,0x01,0x01,0x00};

    if (write(ni_handle, &msg_out, sizeof(msg_out)) < 0)
        printf("\nError sending a message\n");
    else
        printf("\nHeartbeat sent\n");

    sleep(4);
*/

/*
      if (DoReceive)
      receive_packets();
*/


    u8 cpl;
    while (1)
        {




        usleep(100);
        int ldv_err = read(ni_handle, &msg_in, sizeof(msg_in));

        if (ldv_err >= 0)
            {
            if (msg_in.cmq == niRESET)
                { // local reset
                ha_online = 1;
                return 0;
                }
            if (msg_in.cmq == niINCOMING)
                handle_incoming();
            if (msg_in.cmq == niRESPONSE)
                {
                cpl = msg_in.flags & 0x30;

                if (cpl)
                    return (cpl >> 4); // Completion event
                else
                    {
                    if ((msg_in.svc_tag &SVC_MASK) == SVC_request)
                        {
                        memcpy(&msg_rsp, &msg_in, msg_in.len + 2);

                        if (min_rspcode > msg_rsp.code)
                            min_rspcode = msg_rsp.code;
                        }
                    }
                }
                else
                {
                      DumpBuffer("> ", &msg_in, ldv_err);
                }
            }
        else
            {
            // Wait a little bit...
            // timeout--;
            }
        }


    printf("\nClosing device\n");
    close(ni_handle);

    return 0;
    }
