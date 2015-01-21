
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
 #include <stdio.h>
 #include <stdlib.h>
 #include <utils/Log.h>
 #include "ViewRightWebClient.h"
 #include "VCASCommunication.h"



 typedef enum _hprotocol
 {
   PROTOCOL_HTTP,
   PROTOCOL_HTTPS,
   PROTOCOL_FTP
 } hprotocol_t;

#define URL_DEFAULT_PORT_HTTP 80
#define URL_DEFAULT_PORT_HTTPS 81
#define URL_DEFAULT_PORT_FTP 120

#define URL_MAX_HOST_SIZE      120
#define URL_MAX_CONTEXT_SIZE  1024

 typedef struct _hurl
 {
   /**
	 The transfer protocol. 
	 Note that only PROTOCOL_HTTP is supported by nanohttp.
   */
   hprotocol_t protocol;
 
   /**
	 The port number. If no port number was given in the URL,
	 one of the default port numbers will be selected. 
	 URL_HTTP_DEFAULT_PORT	  
	 URL_HTTPS_DEFAULT_PORT   
	 URL_FTP_DEFAULT_PORT	 
   */
   int port;
 
   /** The hostname */
   char host[URL_MAX_HOST_SIZE];
 
   /** The string after the hostname. */
   char context[URL_MAX_CONTEXT_SIZE];
 } hurl_t;


 int hurl_parse(hurl_t * url, const char *urlstr)
 {
   int iprotocol;
   int ihost;
   int iport;
   int len;
   int size;
   char tmp[8];
   char protocol[1024];
 
   iprotocol = 0;
   len = strlen(urlstr);
 
   /* find protocol */
   while (urlstr[iprotocol] != ':' && urlstr[iprotocol] != '\0')
   {
	   iprotocol++;
   }
 
   if (iprotocol == 0)
   {
	   ALOGE("no protocol");
	   return -1;
   }
   if (iprotocol + 3 >= len)
   {
	   ALOGE("no host");
	   return -1;
   }
   if (urlstr[iprotocol] != ':'
	   && urlstr[iprotocol + 1] != '/' && urlstr[iprotocol + 2] != '/')
   {
	   ALOGE("no protocol");
	   return -1;
   }
   /* find host */
   ihost = iprotocol + 3;
   while (urlstr[ihost] != ':'
		  && urlstr[ihost] != '/' && urlstr[ihost] != '\0')
   {
	 ihost++;
   }
 
   if (ihost == iprotocol + 1)
   {
	   ALOGE("no host");
	   return -1;
   }
   /* find port */
   iport = ihost;
   if (ihost + 1 < len)
   {
	 if (urlstr[ihost] == ':')
	 {
	   while (urlstr[iport] != '/' && urlstr[iport] != '\0')
	   {
		 iport++;
	   }
	 }
   }
 
   /* find protocol */
   strncpy(protocol, urlstr, iprotocol);
   protocol[iprotocol] = '\0';
   if (strcasecmp(protocol, "http"))
	   url->protocol = PROTOCOL_HTTP;
   else if (strcasecmp(protocol, "https"))
	   url->protocol = PROTOCOL_HTTPS;
   else if (strcasecmp(protocol, "ftp"))
	   url->protocol = PROTOCOL_FTP;
   else
	   return -1;
 
   /* TODO (#1#): add max of size and URL_MAX_HOST_SIZE */
   size = ihost - iprotocol - 3;
   strncpy(url->host, &urlstr[iprotocol + 3], size);
   url->host[size] = '\0';
 
   if (iport > ihost)
   {
	 size = iport - ihost;
	 strncpy(tmp, &urlstr[ihost + 1], size);
	 url->port = atoi(tmp);
   }
   else
   {
	 switch (url->protocol)
	 {
	 case PROTOCOL_HTTP:
		 url->port = URL_DEFAULT_PORT_HTTP;
		 break;
	 case PROTOCOL_HTTPS:
		 url->port = URL_DEFAULT_PORT_HTTPS;
		 break;
	 case PROTOCOL_FTP:
		 url->port = URL_DEFAULT_PORT_FTP;
		 break;
	 }
   }
 
   len = strlen(urlstr);
   if (len > iport)
   {
	   /* TODO (#1#): find max of size and URL_MAX_CONTEXT_SIZE */
	   size = len - iport;
	   strncpy(url->context, &urlstr[iport], size);
	   url->context[size] = '\0';
   }
   else
   {
	   url->context[0] = '\0';
   }
 
   return 0;
 }

 void VRClientHTTPCallback(void * pUserParam, const char * url, ViewRightWebClient::VRWebClientError_t reason)
 {
 	if(reason != ViewRightWebClient::VR_Success)
 		ALOGE("VRClientHTTPCallback: URL: %s - Failed Retrieve %u\n", url, reason);
 }
 
 int SetupVCASConnection(char * keyurl)
 {
 	//mediaserver is media user and has write permisson to this path
 	#define STOREPATH "/data/misc/media"
 	
 	ViewRightWebClient::VRWebClientError_t commErr;
 	commErr=ViewRightWebClient::GetInstance()->InitializeSSL();
 	if(commErr!=ViewRightWebClient::VR_Success)
 		 ALOGD("SetupVCASConnection: InitializeSSL fail%u\n", commErr);
	
 	//set correct VCAS server
 
 	if(keyurl){	        
 		 hurl_t          mHurl;
 		 if (hurl_parse(&mHurl, keyurl) != 0){							
 		         ALOGE("something wrong for parsing server url,use default key url\n");
		         ViewRightWebClient::GetInstance()->SetVCASCommunicationHandlerSettings("public-ott-nodrm.verimatrix.com", STOREPATH);
 		 }else{
 		          ALOGE("SetupVCASConnection:  VCAS server url %s\n",mHurl.host);
			ViewRightWebClient::GetInstance()->SetVCASCommunicationHandlerSettings(mHurl.host, STOREPATH);
 		 }
	}
		 
 	ViewRightWebClient::GetInstance()->SetLogging(true);
 	ViewRightWebClient::GetInstance()->SetKeyRetrievalStatusCallback(NULL, (KeyRetrCallback) VRClientHTTPCallback);

 	commErr = ViewRightWebClient::GetInstance()->IsDeviceProvisioned();
 	if(commErr==ViewRightWebClient::VR_Success)
 	{
 		ALOGD("SetupVCASConnection: Device Already Provisioned.\n");
 		commErr = ViewRightWebClient::GetInstance()->CheckVCASConnection();
 		if(commErr != ViewRightWebClient::CertificateExpired)
 		{
 			ALOGD("SetupVCASConnection: CheckVCASConnection Returned %u\n", commErr);
 			return (int)commErr;
 		}
 		ALOGD("SetupVCASConnection: CheckVCASConnection Failed, Cert Expired - %u - Reprovisioning\n", commErr);
 	}
 	else
 	{
 		ALOGD("SetupVCASConnection: Device Not Provisioned - %u - Reprovisioning\n", commErr);
 	}
 
 	commErr = ViewRightWebClient::GetInstance()->ConnectAndProvisionDevice();
 	if(commErr != ViewRightWebClient::VR_Success)
 	{
 		ALOGD("SetupVCASConnection: VCASCommunicatoinHandler.ConnectAndProvisionDevice Failed - %u\n", commErr);
 	}
 	else
 	{
 		commErr = ViewRightWebClient::GetInstance()->CheckVCASConnection();
 		ALOGD("SetupVCASConnection: CheckVCASConnection Returned %u\n", commErr);
 	}
 
 	return (int) commErr;
 }
 
 int getvrkey(char* keyurl, uint8_t* keydat)
 {
 	char * id = NULL;
 	int length = 0;
 	//TODO only need to call SetupVCASConnection at application startup
 	if(ViewRightWebClient::GetInstance()->GetUniqueIdentifier(&id, &length) == ViewRightWebClient::VR_Success)
 		ALOGV("Unique Identifier: \"%s\"\n", id);
 
 	int ret = SetupVCASConnection(keyurl);
 	if (ret)
 		ALOGD("SetupVCASConnection Returned %d\n", ret);
 
 	if (ret == 0) {
 		ret = ViewRightWebClient::GetInstance()->RetrieveKeyFile(keyurl, (char*)keydat);
 		ALOGD("RetrieveKeyFile Returned %d\n", ret);
 	}
 
 	return ret;
 }

