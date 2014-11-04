#include "messageHandler.h"
#include "OpenLDVdefinitions.h"
#include <stdlib.h>
#include <ldv.h>

Byte m_LastRefId = 0;

// Get responsecode from message
Byte *GetResponseCode(int respCode)
{
    int SoFMask = 0x40u;	// success or failure?
    int Code = 0;
    Code = (respCode | SoFMask);

    return Code;
}

char *GetAlphaResponse(Byte reqCode, Byte respCode)
{
    int ReqMask = 0x40u;	// request?
    int SoFMask = 0x20u;	// success or failure?
    int CodeMask = ~(ReqMask | SoFMask);
    char *pResult = "DON'T KNOW";

    if ((reqCode & CodeMask) == (respCode & CodeMask))
    {
        // request and response codes do actually belong together
        if (respCode & ReqMask) {
            pResult = "REQUEST   ";
        } else if (respCode & SoFMask) {
            pResult = "SUCCESS   ";
        } else {
            pResult = "FAILURE   ";
        }
    }
    return pResult;
}
// Parse message to Hex with given filler
void GetRawAlpha(const void * const pMsg, int uLen, char cFiller, char  sResult [], int sLen)
{
    char aHextab[] = "0123456789ABCDEF";
   // char* pBytes = reinterpret_cast<const unsigned char*>(const_cast<const void *>(pMsg));
    int stringLen = uLen;
    char* pBytes = (char *)(pMsg);
    int i = 0;
    while (uLen--)
    {
        sResult[i++] = aHextab[(*pBytes >> 4u) & 0x0Fu];
        sResult[i++] = aHextab[*pBytes & 0x0Fu];
        //appendchar(sResult,sLen,aHextab[(*pBytes >> 4u) & 0x0Fu],stringLen);
        //appendchar(sResult,sLen,aHextab[*pBytes & 0x0Fu],stringLen);
        if (cFiller && uLen)
        {
             sResult[i++] = cFiller;
           // appendchar(sResult,sLen,cFiller,stringLen);
        }
        ++pBytes;
    }
     sResult[i] = '\0';
}

// Get next Ref ID
Byte GetNextRefId()
{
    Byte uResult = m_LastRefId;
    ++m_LastRefId;
    m_LastRefId %= REFID_ANY;
    return uResult;
}

//	GetNiAlpha() returns a constant alphanumeric description of the given niCode. The
//	function always succeeds.
//
GetNiAlpha(int niCode)
{
    int pResult;

    char *sCodes[] = {
		"NI_OK",
		"NI_NO_DEVICE",
		"NI_DRIVER_NOT_OPEN",
		"NI_DRIVER_NOT_INIT",
		"NI_DRIVER_NOT_RESET",
		"NI_DRIVER_ERROR",
		"NI_NO_RESPONSES",
		"NI_RESET_FAILS",
		"NI_TIMEOUT",
		"NI_UPLINK_CMD",
		"NI_INTERNAL_ERR",
		"NI_FILE_OPEN_ERR"
	};

    if ((niCode >= NI_OK) && (niCode <= NI_FILE_OPEN_ERR)) {
        pResult = sCodes[niCode];
    }
    return pResult;
}

GetLdvAlpha(int ldvCode)
{
  int pResult;

  char *sLdvCodes[] = {
		 "LDV_OK",
		 "LDV_NOT_FOUND",
		 "LDV_ALREADY_OPEN",
		 "LDV_NOT_OPEN",
         "LDV_DEVICE_ERR",
		 "LDV_INVALID_DEVICE_ID",
		 "LDV_NO_MSG_AVAIL",
		 "LDV_NO_BUFF_AVAIL",
		 "LDV_NO_RESOURCES",
		 "LDV_INVALID_BUF_LEN",
		 "LDV_NOT_ENABLED",
		 "LDVX_INITIALIZATION_FAILED",
		 "LDVX_OPEN_FAILED",
		 "LDVX_CLOSE_FAILED",
		 "LDVX_READ_FAILED",
		 "LDVX_WRITE_FAILED",
		 "LDVX_REGISTER_FAILED",
		 "LDVX_INVALID_XDRIVER",
		 "LDVX_DEBUG_FAILED",
		 "LDVX_ACCESS_DENIED",
         "LDV_CAPABLE_DEVICE_NOT_FOUND",
         "LDV_NO_MORE_CAPABLE_DEVICES",
         "LDV_CAPABILITY_NOT_SUPPORTED",
         "LDV_INVALID_DRIVER_INFO",
         "LDV_INVALID_DEVICE_INFO",
         "LDV_DEVICE_IN_USE",
         "LDV_NOT_IMPLEMENTED",
         "LDV_INVALID_PARAMETER",
         "LDV_INVALID_DRIVER_ID",
         "LDV_INVALID_DATA_FORMAT",
         "LDV_INTERNAL_ERROR",
         "LDV_EXCEPTION",
         "LDV_DRIVER_UPDATE_FAILED",
         "LDV_DEVICE_UPDATE_FAILED",
         "LDV_STD_DRIVER_TYPE_READ_ONLY",
         "RESERVED_35",
         "RESERVED_36",
         "RESERVED_37",
         "RESERVED_38",
         "RESERVED_39",
         "LDV_OUTPUT_BUFFER_SIZE_MISMATCH",
         "LDV_INVALID_BUFFER_PARAMETER",
         "LDV_INVALID_BUFFER_COUNT",
         "LDV_PRIORITY_BUFFER_COUNT_MISMATCH",
         "LDV_BUFFER_SIZE_TOO_SMALL",
         "LDV_BUFFER_CONFIGURATION_TOO_LARGE",
         "LDV_WARNING_APP_BUFFER_SIZE_MISMATCH"
	};

    if ((ldvCode >= LDV_OK) && (ldvCode <= LDV_WARNING_APP_BUFFER_SIZE_MISMATCH))
    {
         pResult = sLdvCodes[ldvCode];
    }
    return pResult;
}
