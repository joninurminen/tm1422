#ifndef MESSAGEHANDLER
#define MESSAGEHANDLER

#define REFID_ANY           10

#include "OpenLDVdefinitions.h"
#include <stdlib.h>

char *GetAlphaResponse(Byte reqCode, Byte respCode);
Byte *GetResponseCode(int respCode);
void GetRawAlpha(const void * const pMsg, int uLen, char cFiller, char sResult[], int sLen);
Byte GetNextRefId();
int GetLdvAlpha(int ldvCode);	// returns ldvcode
int GetNiAlpha(int niCode);

#endif // MESSAGEHANDLER
