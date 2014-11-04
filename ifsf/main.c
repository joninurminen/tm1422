/***************************************************************************
 *   Author: 	Chris Bosomworth
 *   Company: 	Insyte Solutions Pty Ltd
 *   Email:		bosomworth@insyte-solutions.com
 ***************************************************************************/

/*

	Purpose: Example Application - Writes a Lonworks message approximately
	once per second with the data 'Hello World'.

	Change the U20_SERIAL macro to the serial number of your adapter before
	building and executing.

*/

#include <stdio.h>
#include <string.h>
#include <ldv.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "OpenLDVdefinitions.h"

#define FALSE 				0
#define TRUE				1

#define LON_DATA_CODE_SZ	1
#define U20_SERIAL			"FCC94A05"
#define EXAMPLE_DATA		"Hello World"
#define EXAMPLE_CODE		1


typedef enum {
	LON_ADDR_MODE_IMPLICIT,
	LON_ADDR_MODE_EXPLICIT
} lon_addr_mode_t;

ExpAppBuffer ldv_msg_out;
ExpAppBuffer ldv_msg_in;
NM_update_domain_request domainRequest;
NM_set_node_mode_request nodeModeRequest;
NM_heart_beat heartbeat;
LDV_HANDLE ldv_handle;
LDV_STATUS ldv_status;


int lon_write_msg (LDV_HANDLE ldv_handle, char code, char *data, int data_sz, Byte senderType, ExpAppBuffer ldv_msg_out, ServiceType stype);
int lon_read_msg (LDV_HANDLE ldv_handle, ExpAppBuffer ldv_msg_in);
int sendReset(LDV_HANDLE ldv_handle);
int lon_close (LDV_HANDLE ldv_handle);
void msgHdrInit(ExpAppBuffer *m_msgOut,Byte msgSize, ServiceType service, Bool priority, Bool auth, AddrMode mode, Bits msgTag);

void send_heart_beat();

int main(int argc, char *argv[]) {

	pthread_t thread;
	printf ("Opening U20 device with s/n (%s)\n", U20_SERIAL);

	if ((ldv_status = ldv_open (U20_SERIAL, &ldv_handle)) != LDV_OK) {

		printf ("error: unable to open U20 with s/n (%s), status code %d\n", U20_SERIAL, ldv_status);
		return (FALSE);
	}


        if (!sendReset(ldv_handle))
        {

			printf ("Application aborting.\n");
			return (lon_close (ldv_handle));
		}
        sleep (1);



        domainRequest.index = 0x00; // 0x00
        Byte id[] = {0x01,0x01,0x02,0x01,0x01,0x00};;
        memcpy(domainRequest.domain.id, id, DOMAIN_ID_LEN); // to, from , len
        domainRequest.domain.subnet = (Byte)10;
        domainRequest.domain.node = (Bits)1;
        domainRequest.domain.normal = (Bits)0;//bClone ? 0 : 1;
        domainRequest.domain.len = (Byte)1; // 6 to use whole id

        Byte pKey[] = {0x00,0x00,0x00,0x00,0x00,0x00};
        memcpy(domainRequest.domain.key, pKey, AUTH_KEY_LEN);

        if (!lon_write_msg (ldv_handle, NM_update_domain, &domainRequest, sizeof(domainRequest), NI_LOCAL, ldv_msg_out, REQUEST)) // Set Updatedomain
		{
			printf ("Application aborting.\n");
			return (lon_close (ldv_handle));
		}
        sleep (1);


        nodeModeRequest.change = APPL_ONLINE;
        nodeModeRequest.mode = CNFG_ONLINE;
		if (!lon_write_msg (ldv_handle, NM_set_node_mode, &nodeModeRequest, sizeof(nodeModeRequest), NI_LOCAL, ldv_msg_out, REQUEST)) // Set nodemode
		{

			printf ("Application aborting.\n");
			return (lon_close (ldv_handle));
		}


		sleep (1);
		int ret = pthread_create(&thread,NULL,send_heart_beat,NULL);
        if(ret)
        {
            printf("Error - phthread_create() return code; %d\n", ret);
        }

    while(1)
    {
		if (!lon_read_msg (ldv_handle,ldv_msg_in))
		{

			printf ("Application aborting.\n");
			return (lon_close (ldv_handle));
		}

		usleep (100);
    }

	return (lon_close(ldv_handle));
}

void send_heart_beat()
{

  //  while(1)
  //  {

    sleep(5);
       // Byte msg[] = {0x14,0x13,0x40,0x08,0x05,0x03,0x81,0x01,0x00,0x01,0x01,0x0A,0x01,0x01,0x00,0x00,0x01,0x0A,0x01,0x01,0x00};
        Byte msg[] = {0x14,0x13,0x40,0x08,0x05,0x03,0x00,0x04,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x0A,0x01,0x01,0x00};
        // the size of the mesage is the entire thing - so we have to add the size of the NI_Hdr struture as well.
        if ((ldv_status = ldv_write (ldv_handle, msg, sizeof(msg))) != LDV_OK)
        {

            printf ("error: Lonworks message send failed with status code %s\n", GetLdvAlpha(ldv_status));
            return (FALSE);
        }
        else
            printf("heartbeat sent..\n");


/*

        sleep(4);
        heartbeat.subnet = 0x0A;
        heartbeat.node = 0x01;
        heartbeat.domain = 0x01;
        heartbeat.domain2 = 0x00;

		if (!lon_write_msg (ldv_handle, NM_send_heart_beat, &heartbeat, sizeof(heartbeat), NI_BROADCAST, ldv_msg_out, UNACKD)) // Set nodemode
		{

			printf ("Sending heartbeat application aborting.\n");
			return (lon_close (ldv_handle));
		}
		else
            printf("heartbeat sent..\n");
*/
  //  }
}

int sendReset(LDV_HANDLE ldv_handle)
{
    LDV_STATUS ldv_status;

    ExpAppBuffer ldv_msg;
    ldv_msg.ni_hdr.noq.cmd = (Byte)niRESET; // reset command
    ldv_msg.ni_hdr.noq.length = 0;

    if ((ldv_status = ldv_write (ldv_handle, &ldv_msg, ldv_msg.ni_hdr.q.length + sizeof(NI_Hdr))) != LDV_OK)
    {

		printf ("error: Lonworks message send failed with status code %s\n", GetLdvAlpha(ldv_status));
		return (FALSE);
	}
	else
	{
        printf ("OK: Reset message sent with status code %s Bytes: %d\n", GetLdvAlpha(ldv_status), ldv_msg.ni_hdr.q.length + sizeof(NI_Hdr));
        return (TRUE);
	}
		usleep (1000);
}

int lon_write_msg (LDV_HANDLE ldv_handle, char code, char *data, int data_sz, Byte senderType, ExpAppBuffer ldv_msg_out, ServiceType stype ) {

	LDV_STATUS ldv_status;

	char sData[100] = {};

	//memset (&ldv_msg_out, 0, sizeof(ldv_msg_out));

    if(code == NM_set_node_mode || code == NM_update_domain)
        msgHdrInit(&ldv_msg_out, (data_sz + LON_DATA_CODE_SZ), stype, FALSE, FALSE, eLocal, GetNextRefId());
    else
        msgHdrInit(&ldv_msg_out, (data_sz + LON_DATA_CODE_SZ), stype, FALSE, FALSE, eExplicit, 0x40);


	ldv_msg_out.data.exp.code = code;
	memcpy (ldv_msg_out.data.exp.data, data, data_sz * sizeof(Byte) );

	if(code==NM_send_heart_beat)
	{
        Byte nid[] = {0x01, 0x0A, 0x01, 0x01, 0x00, 0x00}; // neuron id from nodeutil
        memcpy(ldv_msg_out.addr.snd.id.nid, nid, 6); // to, from , len
        ldv_msg_out.addr.snd.id.type = NI_BROADCAST;
        ldv_msg_out.addr.snd.id.domain =1;
        ldv_msg_out.addr.snd.id.retry=1;
        ldv_msg_out.addr.snd.id.rpt_timer = 0;
        ldv_msg_out.addr.snd.id.subnet = 1;
        ldv_msg_out.addr.snd.id.tx_timer = 0;
	}

	// the size of the mesage is the entire thing - so we have to add the size of the NI_Hdr struture as well.
	if ((ldv_status = ldv_write (ldv_handle, &ldv_msg_out, ldv_msg_out.ni_hdr.q.length + sizeof(NI_Hdr))) != LDV_OK) {

		printf ("error: Lonworks message send failed with status code %s\n", GetLdvAlpha(ldv_status));
		return (FALSE);
	}
	GetRawAlpha(&ldv_msg_out,ldv_msg_out.ni_hdr.q.length + sizeof(NI_Hdr),32,sData, 100);
    printf ("Write  %s ( %d Bytes: %s)\n", GetLdvAlpha(ldv_status), ldv_msg_out.ni_hdr.q.length + sizeof(NI_Hdr),sData);

	return (TRUE);
}

int lon_read_msg (LDV_HANDLE ldv_handle, ExpAppBuffer ldv_msg_in) {


	LDV_STATUS ldv_status;
//	ExpAppBuffer ldv_msg_in;
	char sData[100] = {};

	memset (&ldv_msg_in, 0, sizeof(ExpAppBuffer));


	//while ((ldv_status = ldv_read (ldv_handle, &ldv_msg_in, sizeof(ldv_msg_in))) == LDV_OK) {
     while ((ldv_status = ldv_read (ldv_handle, &ldv_msg_in, sizeof(ldv_msg_in))) == LDV_OK) {
  //  GetRawAlpha(&ldv_msg_in,ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),32,sData, 100);
  //      printf ("Suatana!  %s ( %d Bytes: %s)\n", GetLdvAlpha(ldv_status), ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),sData);
      //  printf("ldv_msg_in.ni_hdr.q.queue %d\n ",ldv_msg_in.ni_hdr.q.queue);
      //  printf("Message received %x command %x\n ",ldv_msg_in, ldv_msg_in.ni_hdr.noq.cmd);
		switch (ldv_msg_in.ni_hdr.q.queue)
		{

			case niRESPONSE:
			{

				if (ldv_msg_in.msg_hdr.exp.cmpl_code == 1)
				{

					printf ("Received completion event for sent message %x %d %d.\n",ldv_msg_in.data.exp.code, ldv_msg_in.ni_hdr.q.length, sizeof(NI_Hdr));
					GetRawAlpha(&ldv_msg_in,ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),32,sData, 100);
                    printf ("Suatana!  %s ( %d Bytes: %s)\n", GetLdvAlpha(ldv_status), ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),sData);
				}
				else
				{
                //    printf("Response code %x Len: %d NI_Hdr: %d ", ldv_msg_in.data.exp.code, ldv_msg_in.ni_hdr.q.length, sizeof(NI_Hdr));
                    printf("\n");
                    switch((int)GetResponseCode((int)ldv_msg_in.data.exp.code))
                    {
                    case NM_update_domain:
                        {
                            GetRawAlpha(&ldv_msg_in,ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),32,sData, 100);
                            printf ("Read  %s ( %d Bytes: %s)\n", GetLdvAlpha(ldv_status), ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),sData);
                            printf("UpdateDomain    ");
                            printf ("Request 0x%02X={ [%u] ID=%x (Length %u) S/N=%u/%u",
                            NM_update_domain, domainRequest.index,domainRequest.domain.id[0],domainRequest.domain.len,
                            domainRequest.domain.subnet,domainRequest.domain.node
                            );
                            int i;
                            printf(" Key=");
                            for(i = 0; i < AUTH_KEY_LEN; i++)
                            {
                                    printf("%X ", domainRequest.domain.key[i]);
                            }
                            printf ("}, Response 0x%02X: %s\n", ldv_msg_in.data.exp.code, GetAlphaResponse(NM_update_domain,ldv_msg_in.data.exp.code) );

                        break;
                        }
                    case NM_set_node_mode:
                        {
                            GetRawAlpha(&ldv_msg_in,ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),32,sData, 100);
                            printf ("Read  %s ( %d Bytes: %s)\n", GetLdvAlpha(ldv_status), ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),sData);
                            printf("SetNodeMode    ");
                            printf ("Request 0x%02X={Mode %u ChangeState=%u},",
                            NM_set_node_mode,nodeModeRequest.mode, nodeModeRequest.change
                            );
                            printf (" Response 0x%02X: %s\n", ldv_msg_in.data.exp.code, GetAlphaResponse(NM_set_node_mode,ldv_msg_in.data.exp.code) );

                        break;
                        }
                    case NM_query_id:
                        {
                        GetRawAlpha(&ldv_msg_in,ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),32,sData, 100);
                        printf ("Read  %s ( %d Bytes: %s)\n", GetLdvAlpha(ldv_status), ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),sData);
                        printf("QueryId    \n");
                        }
                        break;

                      default:
                        printf("Invalid messagetype\n");
                    }
				}
				break;
			}

			case niINCOMING:
			{

				GetRawAlpha(&ldv_msg_in,ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),32,sData, 100);
                printf ("Received Incoming Lonworks Message  %s ( %d Bytes: %s)\n", GetLdvAlpha(ldv_status), ldv_msg_in.ni_hdr.q.length + sizeof(NI_Hdr),sData);

				break;
			}

			default:
			{

				//printf ("Received message with q.queue of %x\n", ldv_msg_in.ni_hdr.q.queue);
			}
		}

		memset (&ldv_msg_in, 0, sizeof(ExpAppBuffer));
	}

	if ( ldv_status != LDV_NO_MSG_AVAIL ) {

		printf ("error: failure calling ldv_read(...), status: %x\n", ldv_status);
		return (FALSE);
	}

	return (TRUE);
}

void msgHdrInit(ExpAppBuffer *m_msgOut,Byte msgSize, ServiceType service, Bool priority, Bool auth, AddrMode mode, Bits msgTag)
{
    static const NI_Queue queue[ 2 ][ 2 ] =         // define network interface queue
    {
        {
            niTQ, niTQ_P
        },
        {
            niNTQ, niNTQ_P
        }
    };

    // Get correct queue
    m_msgOut->ni_hdr.q.queue  = queue[ service == service ][ priority ];

    // Data transfer for network interface or network
    m_msgOut->ni_hdr.q.q_cmd    = ( mode == eLocal ) ? niNETMGMT : niCOMM;

    m_msgOut->ni_hdr.q.length   = sizeof(ExpMsgHdr) + sizeof(ExplicitAddr) + msgSize;     // Header size

    m_msgOut->msg_hdr.exp.tag       = msgTag;
    m_msgOut->msg_hdr.exp.auth      = auth;
    m_msgOut->msg_hdr.exp.st        = service;
    m_msgOut->msg_hdr.exp.msg_type  = 0;              // the interface doesn't process NVs
    m_msgOut->msg_hdr.exp.response  = 0;             // Not a response message
    m_msgOut->msg_hdr.exp.pool      = 0;            // Must be zero
    m_msgOut->msg_hdr.exp.alt_path  = 0;          // Use default path
    m_msgOut->msg_hdr.exp.addr_mode = (mode == eExplicit); // Addressing mode
    m_msgOut->msg_hdr.exp.cmpl_code = MSG_NOT_COMPL;  // Zero
    m_msgOut->msg_hdr.exp.path      = 0;              // Use primary path
    m_msgOut->msg_hdr.exp.priority  = priority;
    m_msgOut->msg_hdr.exp.length    = msgSize;        // Message size

}



int lon_close (LDV_HANDLE ldv_handle) {

	LDV_STATUS ldv_status;

	if ((ldv_status = ldv_close (ldv_handle)) != LDV_OK) {

		printf ("error: unable to close U20, status code %x\n", ldv_status);
		return (FALSE);
	}

	return (TRUE);
}








