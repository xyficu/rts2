#ifndef __RTS2_SOAPCLI__
#define __RTS2_SOAPCLI__

#include "../utils/rts2devclient.h"
#include "soapH.h"

#define EVENT_SOAP_TEL_GETEQU    	RTS2_LOCAL_EVENT+1000
#define EVENT_SOAP_TEL_GETALT   	RTS2_LOCAL_EVENT+1001
#define EVENT_SOAP_TEL_GET       	RTS2_LOCAL_EVENT+1002
#define EVENT_SOAP_EXEC_GETST    	RTS2_LOCAL_EVENT+1003
#define EVENT_SOAP_EXEC_SET_NEXT	RTS2_LOCAL_EVENT+1004
#define EVENT_SOAP_EXEC_SET_NOW		RTS2_LOCAL_EVENT+1005
#define EVENT_SOAP_DOME_GETST    	RTS2_LOCAL_EVENT+1006
#define EVENT_SOAP_CAMS_GET	 	RTS2_LOCAL_EVENT+1007
#define EVENT_SOAP_CAMD_GET	 	RTS2_LOCAL_EVENT+1008
#define EVENT_SOAP_FOC_GET		RTS2_LOCAL_EVENT+1009
#define EVENT_SOAP_FOC_SET		RTS2_LOCAL_EVENT+1010
#define EVENT_SOAP_FOC_STEP		RTS2_LOCAL_EVENT+1011
#define EVENT_SOAP_FW_GET		RTS2_LOCAL_EVENT+1012
#define EVENT_SOAP_FW_SET		RTS2_LOCAL_EVENT+1013

typedef struct soapExecGetst
{
  rts2__getExecResponse *res;
  soap *in_soap;
};

typedef struct soapExecNext
{
  int next;
  rts2__setNextResponse *res;
  soap *in_soap;
};

typedef struct soapExecNow
{
  int now;
  rts2__setNowResponse *res;
  soap *in_soap;
};

typedef struct soapCameraGet
{
  rts2__getCameraResponse *res;
  soap *in_soap;
};

typedef struct soapCamerasGet
{
  rts2__getCamerasResponse *res;
  soap *in_soap;
};

typedef struct soapFilterSet
{
  int filter;
  rts2__setFilterResponse *res;
  soap *in_soap;
};

typedef struct soapFocusSet
{
  int pos;
  rts2__setFocSetResponse *res;
  soap *in_soap;
};

typedef struct soapFocusStep
{
  int step;
  rts2__setFocStepResponse *res;
  soap *in_soap;
};

class Rts2DevClientTelescopeSoap:public Rts2DevClientTelescope
{
public:
  Rts2DevClientTelescopeSoap (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
  void getObs (struct ln_lnlat_posn *obs);
  void getEqu (struct ln_equ_posn *tel);
  double getLocalSiderealDeg ();
  void getAltAz (struct ln_hrz_posn *hrz);
};

class Rts2DevClientExecutorSoap:public Rts2DevClientExecutor
{
public:
  Rts2DevClientExecutorSoap (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientDomeSoap:public Rts2DevClientDome
{
public:
  Rts2DevClientDomeSoap (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientCameraSoap:public Rts2DevClientCamera
{
public:
  Rts2DevClientCameraSoap (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientFocusSoap:public Rts2DevClientFocus
{
public:
  Rts2DevClientFocusSoap (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

class Rts2DevClientFilterSoap:public Rts2DevClientFilter
{
public:
  Rts2DevClientFilterSoap (Rts2Conn * in_connection);
  virtual void postEvent (Rts2Event * event);
};

void fillTarget (int in_tar_id, struct soap *in_soap,
		 rts2__target * out_target);

void nullTarget (rts2__target * out_target);

#endif /*!__RTS2_SOAPCLI__ */
