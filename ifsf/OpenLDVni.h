//  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  OpenLDV Developer Example
/// <OpenLDV-Example Revision="1"/>
//
// 	Copyright (c) 2004-2011 Echelon Corporation.  All rights reserved.
//
//	Use of this source code is subject to the terms of
//	the Echelon Example Software License Agreement
//	which is available at www.echelon.com/license/examplesoftware/.
//
//  COpenLDVni  implements the network interface protocol, providing 
//  convenient functions to send messages, wait for responses, etc. 
//  Note this is an abstract class; see the abstract virtual NiDispatch() 
//  member functions for details. This OpenLDV Developer Example provides
//  an implementation of the NiDispatch method with the 
//  COpenLDVexampleDispatcher class; see there for details about the
//  application-specific message dispatcher implementation.
//  See below for more details.
//                                                                                  
//  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//
//  Note you cannot create an instance of the COpenLDVni class. The class is
//  abstract. You must derive your own class from COpenLDVni and implement
//  your application-specific message dispatcher; see below for more detailed
//  comments about the dispatcher service, and see COpenLDVexampleDispatcher
//  for an example implementation. 
//
//  Also note COpenLDVni does not support a default constructor, as it always 
//  requires a reference to a COpenLDVapi object. The lifetime of the referred
//  COpenLDVapi object must exceed that of the last Ni* method invocation. 
//
//  The COpenLDVni class provides the following:
//
//  1.  NiInit() and NiClose() calls to initialize and terminate a session.
//      These function calls should be used in place of COpenLDVapi::Open()
//      or COpenLDVapi::Close() calls when using the COpenLDVni class. 
//      The related NiIsOpen() utility may be used to query the current 
//      open/close status.
//
//  2.  NiSendImmediate() to send simple, one-byte, immediate commands to the 
//      network interface. Commands that may be used with this API include 
//      the niRESET or niFLUSH commands; a complete listing can be found in
//      the COpenLDVdefinitions.h file, enumeration NI_NoQueueCmd.
//
//  3.  NiEncryption() to enable and disable encryption of packets sent to 
//      and from the network interface, allowing normal messageing traffic
//      to pass through unencrypted (for maximum performance), and confidential
//      messages, such as those setting the LonTalk authentication keys on 
//      a remote network interface, to be send securely.
//
//  4.  NiSendResponse() to respond to a recently arrived request.
//
//  5.  NiSendMsgWait() to initiate a transaction by sending a message, and 
//      to await completion of that transaction by either success or failure.
//      NiSendMsgWait() is the recommended API for most OpenLDV messaging.
//      When initiating a request, NiSendMsgWait() returns upon completion of
//      the transaction and provides the first response received, if any.
//      The NiGetNextResponse() API may be used to retrieve further responses
//      that relate to the same transaction.
//
//  6.  NiProcessMsg() is used to handle incoming messages that were not 
//      consumed by the NiSendMsgWait() function. NiProcessMsg() typically
//      calls NiDispatch(), see there for more.
//  
//  7.  NiDispatch() is used to process any incoming message that is not
//      processed elsewhere. Message categories NiDispatch is in charge or
//      include the network variable related messages, application and 
//      foreign frame messages, and a range of network management and 
//      diagnostics messages.
//      Note COpenLDVni does not implement NiDispatch; the COpenLDVni class
//      is an abstract class therefore. See COpenLDVexampleDispatcher
//      for an example implementation of this dispatching service.
//
//  8.  An application message pump is implemented via the 
//      COpenLDVni::COpenLDVmessagePump thread. The application message pump
//      subscribes to uplink events provided by the COpenLDVapi wrapper class.
//      These events signal the availability of incoming messages. The pump
//      thread employs the NiProcessMsg() API to retrieve and handle these
//      messages. 
//      This allows for complete asynchronous processing of the Windows user 
//      interface and the LonWorks network interface.
//      Note that the application message dispatcher is typically, but not 
//      exclusively, executed in the context of the application message
//      pump thread therefore.
//      Thread control is handled fully transparently in most cases, however,
//      for those cases that do require explicit and exclusive access to the
//      network interface, the NiPauseMessagePump() and NiContinueMessagePump()
//      API may be used to temporarily pause the pump, or to continue pumping
//      services, respectively.
//      The COpenLDVtools::FindDevices() method shows an example of such a case.
//

#pragma once

#include "openLDVapi.h"
#include "OpenLdvDefinitions.h"
#include "OpenLDVthread.h"

using namespace ldv;

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


class COpenLDVni {
    //  ----------------------------------------------
    //  types and constants:
    //  ----------------------------------------------
public:
    static const unsigned REFID_ANY = 15;       // Use this in calls to SendMsgWait() if you don't care about the refID

    typedef enum
    {   // some selected network management command and request codes:
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

protected:
    // The caller may send an outgoing message and await responses as a result of that message. 
    // This is typically done via the NiSendMsgWait() method. While this method waits for response(s)
    // and completion of the pending transaction, responses to other requests as well as other
    // unexpected messages might arrive. This class stores these messages in an interim buffer using
    // the following type definitions:
    struct msgListStruct {
        struct msgListStruct * nextMsg; // linked list pointer
        ExpAppBuffer           Msg;
    };                                  // linked list of pending messages

    typedef struct msgListStruct*   TpMsgList;

    typedef enum
    {          // internal enumeration for addressing mode
        eLocal,
        eImplicit,
        eExplicit
    } AddrMode;

    static const unsigned niWAIT_TIME  = 5;     // 5 seconds to wait for transaction completion
    static const unsigned niPAUSE_TIME = 25;    // 25 milliseconds to wait before re-trying to access the network interface

    //  COpenLDVmessagePump is the application message pump thread; see
    //  exhaustive comments above for details.
    //  Note the message pump thread is a friend to the COpenLDVni class.
    class COpenLDVmessagePump : public COpenLDVthread {
private:
        static UINT MsgPumpThread(LPVOID lpParam);
public:
        COpenLDVmessagePump();
        virtual ~COpenLDVmessagePump();
        virtual bool Start(COpenLDVni* pOpenLDVni);     // use this to start the message pump; see CLdvThread for more operations
protected:
        virtual bool StartX(LPVOID lParam);             // type-unsafe start routine, see COpenLDVthread::Start()
    };  // class COpenLDVmessagePump

    friend COpenLDVmessagePump;

    //  ----------------------------------------------
    //  private and protected data members
    //  ----------------------------------------------
private:
    COpenLDVapi&    m_oLdv;                 // reference to the COpenLDVapi class.
    COpenLDVmessagePump m_oMessagePump;
    CRITICAL_SECTION m_critReading;
    Byte            m_LastRefId;
    unsigned        m_MessagePumpPause;     // see PauseMessagePump()/ContinueMessagePump()

protected:
    // input and output buffers:
    ExpAppBuffer    m_msgIn;                // Incoming message buffer
    ExpAppBuffer    m_msgOut;               // Outgoing message buffer

    // lists for pending incoming messages and responses:
    TpMsgList       m_pPendingList;         // List of pending incoming msgs
    TpMsgList       m_pResponseList;        // List of subsequent responses

    // table to store the priority of incoming request messages:
    static const unsigned NUM_RCV_TRANSACTIONS = 16;
    bool            m_bRequestPriority[NUM_RCV_TRANSACTIONS];

    //  ----------------------------------------------
    //  construction/destruction
    //  ----------------------------------------------

public:
    COpenLDVni(COpenLDVapi& oLdv);          // standard c'tor. Note the absence of aa default c'tor
    virtual ~COpenLDVni(void);

    //  ----------------------------------------------
    //  public member functions
    //  ----------------------------------------------
public:
    //  NiSendMsgWait( ) - send a message and wait for completion
    NICode NiSendMsgWait(
        ServiceType       service,        // ACKD, UNACKD_RPT, UNACKD, REQUEST
        const SendAddrDtl * outAddr,      // address of outgoing message
        const MsgData     * outData,      // data of outgoing message
        Byte                outLength,    // length of outgoing message
        Bool                priority,     // outgoing message priority
        Bool                outAuth,      // outgoing message authenticated
        ComplType         * completion,   // MSG_SUCCEEDS or MSG_FAILS
        int               * numResponses, // number of received responses
        RespAddrDtl       * inAddr,       // address of first response
        MsgData           * inData,       // data of first response
        Byte              * inLength,     // length of first response
        Bits                refID);

    //  NiGetNextResponse( ) - get subsequent responses to a request message
    NICode NiGetNextResponse(// get subsequent responses here
        RespAddrDtl     * inAddr,
        MsgData         * inData,
        Byte            * inLength);

    //  NiSendResponse( ) - send a response to a request message
    NICode NiSendResponse(// send response to last received request
        MsgData       * outData,   // data for outgoing response
        Byte            outLength, // length of outgoing response
        Bits            refID);

    //  NiEncryption( ) - enable or disable RC4 encryption for remote network interfaces
    NICode NiEncryption(// enable/disable RC4 encryption
        bool            bEnable     // enable or disable
        );

    //  NiIsOpen()  says true if the network interface is open
    bool NiIsOpen() const;

    //  NiProcessMsg() - function to receive and dispatch a message.
    //  Called from the message pump thread. The optional pHadValidMsg will be
    //  set to true or false, reflecting whether the routine received a valid
    //  uplink message. 
    bool NiProcessMsg(bool *pHadValidMsg=NULL); // return true if message was processed successfully

    //  PauseMessagePump()  -   function to pause the message pump temporarily.
    //  Responds true if the pump is paused.
    //  You may want to temporarily pause the uplink message pump to prevent 
    //  asynchronous processing while handling a sequence of transactions. 
    //  Note PauseMessagePump calls must be followed with the same number of 
    //  ContinueMessagePumpt calls.
    bool NiPauseMessagePump();

    //  ContinueMessagePump()   - function to resume message pump operation after
    //  having been temporarily suspended using PauseMessagePump().
    //  Responds true if the message pump is actually running after this call.
    //  Note PauseMessagePump() and ContinueMessagePump() calls must be balanced.
    bool NiContinueMessagePump();

    //  ----------------------------------------------
    //  public overridables
    //  ----------------------------------------------
public:
    //  NiInit().   Initialize the network interface.  Called on program startup and when network interface errors occur
    virtual NICode  NiInit(LPCSTR pDeviceName);

    //  NiClose().  Close network interface.
    virtual void    NiClose(void);

    //  NiSendImmediate().  Send an immediate network interface command
    virtual NICode  NiSendImmediate(NI_NoQueueCmd command);

    // NiDispatch() this method handles all incoming messages except completion codes and responses. 
    // Note this is an abstract method; you must derive your own class from COpenLDVni and override 
    // the NiDispatch() method to dispatch and process these messages appropriately.
    // See COpenLDVexampleDispatcher class for an example of such a custom dispatcher.
    // NiDispatch() (and all overrides) return false if a message has been received but not been processed,
    // and return true in all other cases. 
    // Note NiDispatch() may be executed in any thread context: that of the message pump thread, or that
    // of the caller to NiSendMessageWait(). 
    virtual bool    NiDispatch(const ExpAppBuffer* const pMsgIn) = 0;

    //  ----------------------------------------------
    //  private and protected utilities
    //  ----------------------------------------------
protected:

    Byte GetNextRefId();
    void msgHdrInit(Byte msgSize, ServiceType service, Bool priority, Bool auth, AddrMode mode, Bits msgTag);
    void initResponseList(void);    // initialize list of responses;
    void initPendingList(void);     // initialize list of pending messages
    void saveResponse(Byte msgLength);
    void savePending(Byte msgLength);

    NICode getMsg(bool wait, bool ignorePending = false);
    NICode putMsg(void);
    NICode putMsg(void* pMsg, short Len);
};
