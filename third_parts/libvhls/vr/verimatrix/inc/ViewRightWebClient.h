/*
 * Copyright (c) 2013 Verimatrix, Inc.  All Rights Reserved.
 * The Software or any portion thereof may not be reproduced in any form
 * whatsoever except as provided by license, without the written consent of
 * Verimatrix.
 *
 * THIS NOTICE MAY NOT BE REMOVED FROM THE SOFTWARE BY ANY USER THEREOF.
 * NEITHER VERIMATRIX NOR ANY PERSON OR ORGANIZATION ACTING ON BEHALF OF
 * THEM:
 *
 * 1. MAKES ANY WARRANTY OR REPRESENTATION WHATSOEVER, EXPRESS OR IMPLIED,
 *    INCLUDING ANY WARRANTY OF MERCHANTABILITY OR FITNESS FOR ANY
 *    PARTICULAR PURPOSE WITH RESPECT TO THE SOFTWARE;
 *
 * 2. ASSUMES ANY LIABILITY WHATSOEVER WITH RESPECT TO ANY USE OF THE
 *    PROGRAM OR ANY PORTION THEREOF OR WITH RESPECT TO ANY DAMAGES WHICH
 *     MAY RESULT FROM SUCH USE.
 *
 * RESTRICTED RIGHTS LEGEND:  Use, duplication or disclosure by the
 * Government is subject to restrictions set forth in subparagraphs
 * (a) through (d) of the Commercial Computer SoftwareóRestricted Rights
 * at FAR 52.227-19 when applicable, or in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at
 * DFARS 252.227-7013, and in similar clauses in the NASA FAR supplement,
 * as applicable. Manufacturer is Verimatrix, Inc.
 */ 
#ifndef _VIEWRIGHTWEBCLIENT_H_
#define _VIEWRIGHTWEBCLIENT_H_

#ifndef WIN32
#define __cdecl
#endif

#define COMPANY_NAME_MAX_LENGTH 256

typedef void (__cdecl *KeyRetrCallback)(void *, const char *, unsigned long);

class VCASCommunicationHandler;

class ViewRightWebClient
{
public:
	enum VRWebClientError_t
	{
		VR_Success = 0,
		NoConnect,
		GeneralError,
		BadMemoryAlloc,
		BadRandNumGen,
		BadURL,
		BadReply,
		BadReplyMoved,
		BadVerifyCertificateChain,
		BadCreateKeyPair,
		NotEntitled,
		BadCreateStore,
		BadWriteStore,
		BadReadStore,
		BadStoreIntegrity,
		BadStoreFileNoExist,
		BadCert,
		BadINI,
		BadPrivateKey,
		BadConvertPEMToX509,
		BadPublicEncrypt,
		BadAddX509Entry,
		BadAddX509Subject,
		BadX509Sign,
		CantRetrieveBoot,
		CantProvision,
		BadArguments,
		BadKeyGeneration,
		NotProvisioned,
		CommunicationObjectNotInitialized,
		NoOTTSignature,
		BadOTTSignature,
		KeyFileNotEntitled,
		CertificateExpired,
		IntegrityCheckFailed,
		SecurityError,
		FeatureUnavailable,
		NoUniqueIdentifier,
        LAST_ERROR_CODE,
	};

	typedef struct VRWebClientStatus_Struct
	{
		bool				bIsProvisioned;
		bool				bIsCertExpired;
		unsigned long long	certExpirationTime;
		char				companyName[COMPANY_NAME_MAX_LENGTH];

	} VRWebClientStatus_t;

	virtual ~ViewRightWebClient();

	static ViewRightWebClient * GetInstance(); 

	// Initialization

	VRWebClientError_t SetVCASCommunicationHandlerSettings(const char * VCASBootAddress, const char * storePath, const char * companyName = "");
	VRWebClientError_t SetVCASCommunicationHandlerSettings(const char * VCASBootAddress, const wchar_t * storePath, const char * companyName = "");
	VRWebClientError_t InitializeSSL(void);

	VRWebClientError_t SetUniqueIdentifier(const char * uniqueID, char * validationString);      

	// Provisioning, unique identifier and Key Retrieval

	VRWebClientError_t IsDeviceProvisioned(void);
	VRWebClientError_t ConnectAndProvisionDevice(void);
	VRWebClientError_t CheckVCASConnection(void);
	VRWebClientError_t RetrieveKeyFile(const char * url, char * key);
	VRWebClientError_t GetUniqueIdentifier(char * * uniqueID, int * length);

	void SetKeyRetrievalStatusCallback(void * userData, KeyRetrCallback func);

	// Status

	VRWebClientError_t GetClientStatus(VRWebClientStatus_t * clientStatus);

	// Logging

	void SetLogging(bool OnOff);

	// Version Info

	const char * GetVersion(void);
    
private:
	static ViewRightWebClient * m_pInstance;
	VCASCommunicationHandler * m_pVCASCommunicationHandler;
	bool m_bSSLInitialized;

	ViewRightWebClient();
 };

#endif
