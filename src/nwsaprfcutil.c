#include <Python.h>
#include "structmember.h"
#include <string.h>
#include <signal.h>

#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif


/* SAP flag for Windows NT or 95 */
#ifdef _WIN32
#  ifndef SAPonNT
#    define SAPonNT
#  endif
#endif

#include <sapnwrfc.h>

#if defined(SAPonNT)
#include "windows.h"
#endif


/* fake up a definition of bool if it doesnt exist */
#ifndef bool
typedef SAP_RAW    bool;
#endif

/* create my true and false */
#ifndef false
typedef enum { false, true } mybool;
#endif


typedef struct SAPNW_CONN_INFO_rec {
                  RFC_CONNECTION_HANDLE handle;
	                RFC_CONNECTION_PARAMETER * loginParams;
									unsigned loginParamsLength;
									/*
									unsigned refs;
									*/
} SAPNW_CONN_INFO;

typedef struct SAPNW_FUNC_DESC_rec {
                  RFC_FUNCTION_DESC_HANDLE handle;
									SAPNW_CONN_INFO * conn_handle;
									/*
									unsigned refs;
									*/
									char * name;
} SAPNW_FUNC_DESC;

typedef struct SAPNW_FUNC_rec {
                  RFC_FUNCTION_HANDLE handle;
									SAPNW_FUNC_DESC * desc_handle;
} SAPNW_FUNC;


PyObject *E_RFC_COMMS, *E_RFC_SERVER, *E_RFC_FUNCCALL;


staticforward PyTypeObject sapnwrfc_ConnType;
typedef struct {
     PyObject_HEAD
		 SAPNW_CONN_INFO *connInfo;
} sapnwrfc_ConnObject;

staticforward PyTypeObject sapnwrfc_FuncDescType;
typedef struct {
     PyObject_HEAD
		 SAPNW_FUNC_DESC *handle;
		 PyObject *name;
		 PyObject *parameters;
} sapnwrfc_FuncDescObject;

staticforward PyTypeObject sapnwrfc_FuncCallType;
typedef struct {
     PyObject_HEAD
		 SAPNW_FUNC *handle;
		 PyObject *name;
		 PyObject *parameters;
		 PyObject *function_descriptor;
} sapnwrfc_FuncCallObject;



PyMODINIT_FUNC initnwsaprfcutil(void);
void stop_execution (int sig);
static void * make_space(int len);
static void * make_strdup(PyObject *value);
SAP_UC * u8to16c(char * str);
SAP_UC * u8to16(PyObject *str);
PyObject* u16to8c(SAP_UC * str, int len);
PyObject* u16to8(SAP_UC * str);
static PyObject* SAPNW_rfc_conn_error(PyObject* msg, int code, PyObject* key, PyObject* message);
static PyObject* SAPNW_rfc_serv_error(PyObject* msg, int code, PyObject* key, PyObject* message);
static PyObject* SAPNW_rfc_call_error(PyObject* msg, int code, PyObject* key, PyObject* message);

static PyObject * get_field_value(DATA_CONTAINER_HANDLE hcont, RFC_FIELD_DESC fieldDesc);
void set_field_value(DATA_CONTAINER_HANDLE hcont, RFC_FIELD_DESC fieldDesc, PyObject * value);
static PyObject * get_table_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name);
void set_table_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value);


static void * make_space(int len){

    char * ptr;
    ptr = malloc( len + 1 );
    if ( ptr == NULL )
	    return NULL;
    memset(ptr, 0, len + 1);
    return ptr;
}

/* copy the value of a parameter to a new pointer variable to be passed back onto the
   parameter pointer argument without the length supplied */
static void * make_strdup(PyObject *value){

    char * ptr;
    int len = PyString_Size(value);
		ptr = make_space(len);
		memcpy((char *)ptr, PyString_AsString(value), len);
    return ptr;
}

void
stop_execution (int sig){
  fprintf(stderr, "CTL-C pressed - exiting...\n");
  exit(1);
}


SAP_UC * u8to16c(char * str) {
  RFC_RC rc;
	RFC_ERROR_INFO errorInfo;
	SAP_UC *sapuc;
	unsigned sapucSize, resultLength;

  sapucSize = strlen(str) + 1;
  sapuc = mallocU(sapucSize);
  memsetU(sapuc, 0, sapucSize);

	resultLength = 0;

  rc = RfcUTF8ToSAPUC((RFC_BYTE *)str, strlen(str), sapuc, &sapucSize, &resultLength, &errorInfo);
	return sapuc;
}


SAP_UC * u8to16(PyObject *str) {
  RFC_RC rc;
	RFC_ERROR_INFO errorInfo;
	SAP_UC *sapuc;
	unsigned sapucSize, resultLength;

  sapucSize = PyString_Size(str) + 1;
  sapuc = mallocU(sapucSize);
  memsetU(sapuc, 0, sapucSize);

	resultLength = 0;

  rc = RfcUTF8ToSAPUC((RFC_BYTE *)PyString_AsString(str), PyString_Size(str), sapuc, &sapucSize, &resultLength, &errorInfo);
	return sapuc;
}


PyObject* u16to8c(SAP_UC * str, int len) {
  RFC_RC rc;
	RFC_ERROR_INFO errorInfo;
	unsigned utf8Size, resultLength;
	char * utf8;
	PyObject * py_str;

  utf8Size = len * 2;
  utf8 = malloc(utf8Size + 1);
  memset(utf8, 0, utf8Size + 1);

	resultLength = 0;

  rc = RfcSAPUCToUTF8(str, len, (RFC_BYTE *)utf8, &utf8Size, &resultLength, &errorInfo);
  py_str = PyString_FromStringAndSize(utf8, resultLength);
	free(utf8);
  return py_str;
}


PyObject* u16to8(SAP_UC * str) {
  RFC_RC rc;
	RFC_ERROR_INFO errorInfo;
	unsigned utf8Size, resultLength;
	char * utf8;
	PyObject * py_str;

  utf8Size = strlenU(str) * 2;
  utf8 = malloc(utf8Size + 1);
  memset(utf8, 0, utf8Size + 1);

	resultLength = 0;

  rc = RfcSAPUCToUTF8(str, strlenU(str), (RFC_BYTE *)utf8, &utf8Size, &resultLength, &errorInfo);
  py_str = PyString_FromStringAndSize(utf8, resultLength);
	free(utf8);
  return py_str;
}


static PyObject* SAPNW_rfc_conn_error(PyObject* msg, int code, PyObject* key, PyObject* message) {

  PyErr_Format(E_RFC_COMMS, "RFC COMMUNICATION ERROR: %s / %d / %s / %s", PyString_AsString(msg), code, PyString_AsString(key), PyString_AsString(message));
	return NULL;
}


static PyObject* SAPNW_rfc_serv_error(PyObject* msg, int code, PyObject* key, PyObject* message) {

  PyErr_Format(E_RFC_COMMS, "RFC SERVER ERROR: %s / %d / %s / %s", PyString_AsString(msg), code, PyString_AsString(key), PyString_AsString(message));
	return NULL;
}


static PyObject* SAPNW_rfc_call_error(PyObject* msg, int code, PyObject* key, PyObject* message) {

  PyErr_Format(E_RFC_COMMS, "RFC FUNCTION CALL ERROR: %s / %d / %s / %s", PyString_AsString(msg), code, PyString_AsString(key), PyString_AsString(message));
	return NULL;
}

static PyObject* SAPNW_rfc_call_error1(char * msg, char * part1) {

  PyErr_Format(E_RFC_COMMS, "RFC FUNCTION CALL ERROR: %s %s", msg, part1);
	return NULL;
}


static PyObject* conn_handle_close(SAPNW_CONN_INFO *ptr) {
  RFC_ERROR_INFO errorInfo;
  RFC_RC rc = RFC_OK;

  if (ptr == NULL || ptr->handle == NULL)
    return Py_BuildValue( "i", ( int ) 1);

  rc = RfcCloseConnection(ptr->handle, &errorInfo);
  if (rc != RFC_OK) {
		ptr->handle = NULL;
	  return SAPNW_rfc_conn_error(PyString_FromString("Problem closing RFC connection handle"),
	                       errorInfo.code,
		  									 u16to8(errorInfo.key),
		  									 u16to8(errorInfo.message));
	} else {
		ptr->handle = NULL;
    return Py_BuildValue( "i", ( int ) 1);
	}
}


/* Disconnect from an SAP system */
static PyObject* sapnwrfc_conn_close(sapnwrfc_ConnObject* self){

  SAPNW_CONN_INFO *ptr;

	ptr = self->connInfo;

  return conn_handle_close(ptr);
}


/* Get the attributes of a connection handle */
static PyObject* sapnwrfc_connection_attributes(sapnwrfc_ConnObject *self, PyObject *args){

  SAPNW_CONN_INFO *hptr;
	RFC_ATTRIBUTES attribs;
  RFC_ERROR_INFO errorInfo;
	PyObject* attrib_hash;
	RFC_RC rc = RFC_OK;


	hptr = self->connInfo;

	rc = RfcGetConnectionAttributes(hptr->handle, &attribs, &errorInfo);

  /* bail on a bad return code */
  if (rc != RFC_OK) {
	  return SAPNW_rfc_conn_error(PyString_FromString("getting connection attributes "),
	                       errorInfo.code,
		  									 u16to8(errorInfo.key),
		  									 u16to8(errorInfo.message));
	}

  /* else return a hash of connection attributes */
	attrib_hash = PyDict_New();
  PyDict_SetItemString(attrib_hash, "dest", u16to8(attribs.dest));
  PyDict_SetItemString(attrib_hash, "host", u16to8(attribs.host));
  PyDict_SetItemString(attrib_hash, "partnerHost", u16to8(attribs.partnerHost));
  PyDict_SetItemString(attrib_hash, "sysNumber", u16to8(attribs.sysNumber));
  PyDict_SetItemString(attrib_hash, "sysId", u16to8(attribs.sysId));
  PyDict_SetItemString(attrib_hash, "client", u16to8(attribs.client));
  PyDict_SetItemString(attrib_hash, "user", u16to8(attribs.user));
  PyDict_SetItemString(attrib_hash, "language", u16to8(attribs.language));
  PyDict_SetItemString(attrib_hash, "trace", u16to8(attribs.trace));
  PyDict_SetItemString(attrib_hash, "isoLanguage", u16to8(attribs.isoLanguage));
  PyDict_SetItemString(attrib_hash, "codepage", u16to8(attribs.codepage));
  PyDict_SetItemString(attrib_hash, "partnerCodepage", u16to8(attribs.partnerCodepage));
  PyDict_SetItemString(attrib_hash, "rfcRole", u16to8(attribs.rfcRole));
  PyDict_SetItemString(attrib_hash, "type", u16to8(attribs.type));
  PyDict_SetItemString(attrib_hash, "rel", u16to8(attribs.rel));
  PyDict_SetItemString(attrib_hash, "partnerType", u16to8(attribs.partnerType));
  PyDict_SetItemString(attrib_hash, "partnerRel", u16to8(attribs.partnerRel));
  PyDict_SetItemString(attrib_hash, "kernelRel", u16to8(attribs.kernelRel));
  PyDict_SetItemString(attrib_hash, "cpicConvId", u16to8(attribs.cpicConvId));
  PyDict_SetItemString(attrib_hash, "progName", u16to8(attribs.progName));

  return attrib_hash;
}


static void sapnwrfc_Conn_dealloc(sapnwrfc_ConnObject* self) {

    sapnwrfc_conn_close(self);
	self->ob_type->tp_free((PyObject*)self);
}


/* build a connection to an SAP system */
/*
 * must call this from within Base.connect
 *   in Base.connect it allocates empty SAPNW::Handle which gets tainted with the handle struct
 *   this then becomes an attribute of a new SAPNW::Connection which should also
 *   contain a copy of the connection parameters used to make the connection - incase a reconnect is needed
 */




static PyObject *
sapnwrfc_Conn_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    sapnwrfc_ConnObject *self;

    self = (sapnwrfc_ConnObject *)type->tp_alloc(type, 0);

		// return NULL is a PyError
    if (self != NULL) {
      self->connInfo = NULL;
    }

    return (PyObject *)self;
}



static int sapnwrfc_Conn_init(sapnwrfc_ConnObject *self, PyObject *args) {
  RFC_ERROR_INFO errorInfo;
  PyObject *connParms, *hval, *parm;
	SAPNW_CONN_INFO *hptr;
	RFC_CONNECTION_PARAMETER * loginParams;
	int idx, i;
	Py_ssize_t pos;
	bool server;

  if (! PyArg_Parse(args, "(O)", &connParms)) {
		  Py_DECREF(args);
      return -1;
	}

	hptr = malloc(sizeof(SAPNW_CONN_INFO));
	memset(hptr, 0, sizeof(SAPNW_CONN_INFO));
	hptr->handle = NULL;
	/*
	hptr->refs = 0;
	*/

  if (!PyDict_Check(connParms)) {
	  PyErr_Format(PyExc_TypeError, "Connection parameters must be a Dict");
		//return NULL;
		return -1;
	}

  idx = PyDict_Size(connParms);
	if (idx == 0) {
	  PyErr_Format(PyExc_RuntimeError, "No Connection Parameters");
		return -1;
	}

	loginParams = malloc(idx*sizeof(RFC_CONNECTION_PARAMETER));
	memset(loginParams, 0,idx*sizeof(RFC_CONNECTION_PARAMETER));

  server = false;
	pos = 0;
	i = 0;
	while (PyDict_Next(connParms, &pos, &parm, &hval)) {
		 if (strcmp(PyString_AsString(hval), "tpname") == 0)
		   server = true;
     loginParams[i].name = (SAP_UC *) u8to16(parm);
     loginParams[i].value = (SAP_UC *) u8to16(hval);
		 i++;
  }

  if (server) {
	  fprintf(stderr, "RfcRegisterServer ...\n");
	  hptr->handle = RfcRegisterServer(loginParams, idx, &errorInfo);
		hptr->loginParams = loginParams;
		hptr->loginParamsLength = idx;
	} else {
	  hptr->handle = RfcOpenConnection(loginParams, idx, &errorInfo);
	};

	if (! server || hptr->handle == NULL) {
		hptr->loginParams = NULL;
		hptr->loginParamsLength = 0;
	  for (i = 0; i < idx; i++) {
       free((char *) loginParams[i].name);
       free((char *) loginParams[i].value);
    }
	  free(loginParams);
	}
	if (hptr->handle == NULL) {
	  SAPNW_rfc_conn_error(PyString_FromString("RFC connection open failed "),
	                       errorInfo.code,
		  									 u16to8(errorInfo.key),
		  									 u16to8(errorInfo.message));
	  return -1;
	}
  self->connInfo = hptr;

	return 0;
}


static void sapnwrfc_FuncDesc_dealloc(sapnwrfc_FuncDescObject* self) {

  RFC_ERROR_INFO errorInfo;
  RFC_RC rc = RFC_OK;
  SAPNW_FUNC_DESC *ptr;
	/*
	VALUE errkey, errmsg;
	*/

	ptr = self->handle;
//  fprintf(stderr, "func_desc_handle_free: -> start %p\n", ptr);
  rc = RfcDestroyFunctionDesc(ptr->handle, &errorInfo);
	ptr->handle = NULL;
  if (rc != RFC_OK) {
	  fprintfU(stderr, cU("RFC ERR %s: %s\n"), errorInfo.key, errorInfo.message);
    //errkey = u16to8(ptr->conn_handle, errorInfo.key); errmsg = u16to8(ptr->conn_handle, errorInfo.message);
		//rb_raise(rb_eRuntimeError, "Problem in RfcDestroyFunctionDesc code: %d key: %s message: %s\n",
		 //                          errorInfo.code, StringValueCStr(errkey), StringValueCStr(errmsg));
	}
	/*
	ptr->conn_handle->refs --;
	*/
	ptr->conn_handle = NULL;
	free(ptr->name);
	/*
	if (ptr->refs != 0) {
	  fprintf(stderr, "Still have FUNC_DESC references in FUNC_CALLs (%d) \n", ptr->refs);
		exit(-1);
	}
	*/
	free(ptr);
	self->ob_type->tp_free((PyObject*)self);
}


static PyObject *
sapnwrfc_FuncDesc_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    sapnwrfc_FuncDescObject *self;

    self = (sapnwrfc_FuncDescObject *)type->tp_alloc(type, 0);

		// return NULL is a PyError
    if (self != NULL) {
      self->handle = NULL;
    }

    return (PyObject *)self;
}


static int sapnwrfc_FuncDesc_init(sapnwrfc_FuncDescObject *self, PyObject *args) {
  PyObject *conn, *func;
	SAPNW_CONN_INFO *hptr;
	SAPNW_FUNC_DESC *dptr;

  if (! PyArg_Parse(args, "(OO)", &conn, &func)) {
		  Py_DECREF(args);
      return -1;
	}

	dptr = malloc(sizeof(SAPNW_FUNC_DESC));
	memset(dptr, 0, sizeof(SAPNW_FUNC_DESC));
	//dptr->handle = func_desc_handle;
	dptr->handle = NULL;
	hptr = ((sapnwrfc_ConnObject *)conn)->connInfo;
	dptr->conn_handle = hptr;
	/*
	dptr->refs = 0;
	dptr->conn_handle->refs ++;
	*/
	dptr->name = make_strdup(func);
	self->handle = dptr;
	return 0;
}


/* Get the Metadata description of a Function Module */
static PyObject * sapnwrfc_function_lookup(sapnwrfc_ConnObject *self, PyObject *args){

  SAPNW_CONN_INFO *hptr;
	SAPNW_FUNC_DESC *dptr;
	RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	PyObject *func, *parm_name, *parameters, *parameter;
  sapnwrfc_FuncDescObject* function_def;
	SAP_UC * fname;
	RFC_FUNCTION_DESC_HANDLE func_desc_handle;
	RFC_ABAP_NAME func_name;
	RFC_PARAMETER_DESC parm_desc;
	unsigned parm_count, i;
	//int i;

  if (! PyArg_Parse(args, "(O)", &func)) {
		  Py_DECREF(args);
      return NULL;
	}

	hptr = self->connInfo;

  if (!PyString_Check(func)) {
	  PyErr_Format(PyExc_TypeError, "Function Name in function_lookup must be a String");
		return NULL;
	}

	func_desc_handle = RfcGetFunctionDesc(hptr->handle, (fname = u8to16(func)), &errorInfo);
	free((char *)fname);

  /* bail on a bad lookup */
  if (func_desc_handle == NULL) {
		return SAPNW_rfc_conn_error(PyString_FromFormat("Problem looking up RFC: %s", PyString_AsString(func)),
	                       errorInfo.code,
		  									 u16to8(errorInfo.key),
		  									 u16to8(errorInfo.message));
	}

  /* wrap in SAPNW::RFC::FunctionDescriptor  Object */
	function_def = (sapnwrfc_FuncDescObject *) PyObject_CallFunction((PyObject *)&sapnwrfc_FuncDescType, "OO", self, func);
	Py_INCREF(function_def);

	dptr = function_def->handle;
	dptr->handle = func_desc_handle;

  /* read back the function name */
	rc = RfcGetFunctionName(dptr->handle, func_name, &errorInfo);

  /* bail on a bad RfcGetFunctionName */
  if (rc != RFC_OK) {
	  return SAPNW_rfc_conn_error(PyString_FromFormat("Problem in RfcGetFunctionName: %s", PyString_AsString(func)),
	                       errorInfo.code,
		  									 u16to8(errorInfo.key),
		  									 u16to8(errorInfo.message));
	}
	PyObject_SetAttrString((PyObject*)function_def, "name", u16to8(func_name));

  /* Get the parameter details */
	rc = RfcGetParameterCount(dptr->handle, &parm_count, &errorInfo);

  /* bail on a bad RfcGetParameterCount */
  if (rc != RFC_OK) {
	  return SAPNW_rfc_conn_error(PyString_FromFormat("Problem in RfcGetParameterCount: %s", PyString_AsString(func)),
	                       errorInfo.code,
		  									 u16to8(errorInfo.key),
		  									 u16to8(errorInfo.message));
	}

	parameters = PyDict_New();
	PyObject_SetAttrString((PyObject*)function_def, "parameters", parameters);
	for (i = 0; i < parm_count; i++) {
	  rc = RfcGetParameterDescByIndex(dptr->handle, i, &parm_desc, &errorInfo);
    /* bail on a bad RfcGetParameterDescByIndex */
    if (rc != RFC_OK) {
	    return SAPNW_rfc_conn_error(PyString_FromFormat("Problem in RfcGetParameterDescByIndex: %s", PyString_AsString(func)),
		                       errorInfo.code,
			  									 u16to8(errorInfo.key),
			  									 u16to8(errorInfo.message));
  	}

		/* create a new parameter obj */
    parm_name = u16to8(parm_desc.name);
	  parameter = PyDict_New();
    PyDict_SetItemString(parameter, "name", parm_name);
    PyDict_SetItemString(parameter, "direction", PyInt_FromLong(parm_desc.direction));
    PyDict_SetItemString(parameter, "type", PyInt_FromLong(parm_desc.type));
    PyDict_SetItemString(parameter, "len", PyInt_FromLong(parm_desc.nucLength));
    PyDict_SetItemString(parameter, "ulen", PyInt_FromLong(parm_desc.ucLength));
    PyDict_SetItemString(parameter, "decimals", PyInt_FromLong(parm_desc.decimals));
    PyDict_SetItem(parameters, parm_name, parameter);
  }

	return (PyObject*)function_def;
}


/* Create a Function Module handle to be used for an RFC call */
static PyObject * sapnwrfc_create_function_call(PyObject *self, PyObject *args){

	SAPNW_FUNC_DESC *dptr;
  RFC_ERROR_INFO errorInfo;
	RFC_FUNCTION_HANDLE func_handle;
  sapnwrfc_FuncCallObject* function;
	PyObject *parameters;

	dptr = ((sapnwrfc_FuncDescObject *) self)->handle;


	func_handle = RfcCreateFunction(dptr->handle, &errorInfo);

  /* bail on a bad lookup */
  if (func_handle == NULL) {
		return SAPNW_rfc_conn_error(PyString_FromFormat("Problem Creating Function Data Container RFC: %s", dptr->name),
		                     errorInfo.code,
												 u16to8(errorInfo.key),
												 u16to8(errorInfo.message));
	}

  /* wrap in SAPNW::RFC::FunctionCall  Object */
	function = (sapnwrfc_FuncCallObject *) PyObject_CallFunction((PyObject *)&sapnwrfc_FuncCallType, "O", self);
	//Py_INCREF(function);
  function->handle->handle = func_handle;
	PyObject_SetAttrString((PyObject*)function, "name", PyString_FromString(dptr->name));
	PyObject_SetAttrString((PyObject*)function, "function_descriptor", self);
	parameters = PyDict_New();
	PyObject_SetAttrString((PyObject*)function, "parameters", parameters);

  return (PyObject*)function;
}


static PyObject *
sapnwrfc_FuncCall_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    sapnwrfc_FuncCallObject *self;

    self = (sapnwrfc_FuncCallObject *)type->tp_alloc(type, 0);

		// return NULL is a PyError
    if (self != NULL) {
      self->handle = NULL;
    }

    return (PyObject *)self;
}


static int sapnwrfc_FuncCall_init(sapnwrfc_FuncCallObject *self, PyObject *args) {
  PyObject *func_desc;
	SAPNW_FUNC *fptr;

  if (! PyArg_Parse(args, "(O)", &func_desc)) {
		  Py_DECREF(args);
      return -1;
	}

  /* wrap in SAPNW::RFC::FunctionCall  Object */
	fptr = malloc(sizeof(SAPNW_FUNC));
	memset(fptr, 0, sizeof(SAPNW_FUNC));
	fptr->handle = NULL;
	fptr->desc_handle = ((sapnwrfc_FuncDescObject *)func_desc)->handle;
	self->handle = fptr;
	return 0;
}


static void sapnwrfc_FuncCall_dealloc(sapnwrfc_FuncCallObject* self) {
  RFC_ERROR_INFO errorInfo;
  RFC_RC rc = RFC_OK;
  SAPNW_FUNC *ptr;

	ptr = self->handle;

  rc = RfcDestroyFunction(ptr->handle, &errorInfo);
	ptr->handle = NULL;
  if (rc != RFC_OK) {
	  fprintfU(stderr, cU("RfcDestroyFunction: %d - %s - %s\n"),
		                     errorInfo.code,
												 u16to8(errorInfo.key),
												 u16to8(errorInfo.message));
	    SAPNW_rfc_conn_error(PyString_FromFormat("Problem in RfcDestroyFunction: %s", ptr->desc_handle->name),
		                     errorInfo.code,
												 u16to8(errorInfo.key),
												 u16to8(errorInfo.message));
	}
	/*
	ptr->desc_handle->refs --;
	*/
	ptr->desc_handle = NULL;
	free(ptr);
	self->ob_type->tp_free((PyObject*)self);
}


static PyObject * get_time_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_TIME timeBuff;
	PyObject * val;

  rc = RfcGetTime(hcont, name, timeBuff, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetTime: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }
  val = u16to8c(timeBuff, 6);
	return val;
}


static PyObject * get_date_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_DATE dateBuff;
	PyObject * val;

  rc = RfcGetDate(hcont, name, dateBuff, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetDate: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }
  val = u16to8c(dateBuff, 8);
	return val;
}


static PyObject * get_int_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_INT rfc_int;

  rc = RfcGetInt(hcont, name, &rfc_int, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetInt: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }
  return Py_BuildValue( "i", ( int ) rfc_int);
}


static PyObject * get_int1_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_INT1 rfc_int1;

  rc = RfcGetInt1(hcont, name, &rfc_int1, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetInt1: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }
  return Py_BuildValue( "i", ( int ) rfc_int1);
}


static PyObject * get_int2_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_INT2 rfc_int2;

  rc = RfcGetInt2(hcont, name, &rfc_int2, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetInt2: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }
  return Py_BuildValue( "i", ( int ) rfc_int2);
}


static PyObject * get_float_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_FLOAT rfc_float;

  rc = RfcGetFloat(hcont, name, &rfc_float, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetFloat: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }
  return PyFloat_FromDouble(( double ) rfc_float);
}


static PyObject * get_string_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	PyObject * val;
	unsigned strLen, retStrLen;
	char * buffer;

  rc = RfcGetStringLength(hcont, name, &strLen, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetStringLength: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }

  /* bail out if string is empty */
  if (strLen == 0)
    return Py_BuildValue("s", ( char * ) "");

  buffer = make_space(strLen*2 + 2);
  rc = RfcGetString(hcont, name, (SAP_UC *)buffer, strLen + 2, &retStrLen, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetString: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }

  //val = u16to8c((SAP_UC *)buffer, retStrLen*2);
  val = u16to8c((SAP_UC *)buffer, retStrLen);
	free(buffer);
	return val;
}


static PyObject * get_xstring_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	PyObject * val;
	unsigned strLen, retStrLen;
	char * buffer;

  rc = RfcGetStringLength(hcont, name, &strLen, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetStringLength in XSTRING: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }

  /* bail out if string is empty */
  if (strLen == 0)
    return Py_BuildValue("s", ( char * ) "");

  buffer = make_space(strLen);
  rc = RfcGetXString(hcont, name, (SAP_RAW *)buffer, strLen, &retStrLen, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetXString: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }

  val = PyString_FromStringAndSize(buffer, strLen);
	free(buffer);
	return val;
}



static PyObject * get_num_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, unsigned ulen){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	char * buffer;
	PyObject * val;

  buffer = make_space(ulen*2+1); /* seems that you need 2 null bytes to terminate a string ...*/
  rc = RfcGetNum(hcont, name, (RFC_NUM *)buffer, ulen, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetNum: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }
	val = u16to8((SAP_UC *)buffer);
  free(buffer);

	return val;
}


static PyObject * get_bcd_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	PyObject * val;
	unsigned strLen, retStrLen;
	char * buffer;

  /* select a random long length for a BCD */
  strLen = 100;

  buffer = make_space(strLen*2);
  rc = RfcGetString(hcont, name, (SAP_UC *)buffer, strLen, &retStrLen, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetString in NUMC: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }

  val = u16to8c((SAP_UC *)buffer, retStrLen);
	free(buffer);
	// XXX need to convert this to a float
  return val;
}


static PyObject * get_char_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, unsigned ulen){

    RFC_RC rc = RFC_OK;
    RFC_ERROR_INFO errorInfo;
	char * buffer;
	PyObject * val;
	unsigned utf8Size, resultLength;
	char * utf8;

    buffer = make_space((ulen + 1)*4);

    rc = RfcGetChars(hcont, name, (RFC_CHAR *)buffer, ulen, &errorInfo);
    if (rc != RFC_OK) {
   	    return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetChars: %s",
		                                    PyString_AsString(u16to8(name))),
                                            errorInfo.code,
   											u16to8(errorInfo.key),
          							        u16to8(errorInfo.message));
    }
    //fprintfU(stderr, cU("length of %s appears to be %d/%d\n"), name, strlenU((SAP_UC*)buffer), ulen);
    //fprintfU(stderr, cU("value of %s appears to be %s\n"), name, (SAP_UC*)buffer);

    // make space and do the UTF-16 to UTF-8 conversion
    utf8Size = ulen * 4;
    utf8 = malloc(utf8Size + 4);
    memset(utf8, 0, utf8Size + 4);
	resultLength = 0;
    rc = RfcSAPUCToUTF8((SAP_UC *)buffer, ulen, (RFC_BYTE *)utf8, &utf8Size, &resultLength, &errorInfo);
    val = PyString_FromStringAndSize(utf8, resultLength);
	free(utf8);
    free(buffer);

	return val;
}


static PyObject * get_byte_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, unsigned len){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	char * buffer;
	PyObject * val;

  buffer = make_space(len);
  rc = RfcGetBytes(hcont, name, (SAP_RAW *)buffer, len, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetBytes: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }
  val = PyString_FromStringAndSize(buffer, len);
  free(buffer);

	return val;
}


static PyObject * get_structure_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_STRUCTURE_HANDLE line;
	RFC_TYPE_DESC_HANDLE typeHandle;
	RFC_FIELD_DESC fieldDesc;
	unsigned fieldCount, i;
	PyObject * val;

  rc = RfcGetStructure(hcont, name, &line, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetStructure: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }

	typeHandle = RfcDescribeType(line, &errorInfo);
  if (typeHandle == NULL) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcDescribeType: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }

	rc = RfcGetFieldCount(typeHandle, &fieldCount, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetFieldCount: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }

  val = PyDict_New();
  for (i = 0; i < fieldCount; i++) {
	  rc = RfcGetFieldDescByIndex(typeHandle, i, &fieldDesc, &errorInfo);
    if (rc != RFC_OK) {
   	  return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetFieldDescByIndex: %s",
		                                   PyString_AsString(u16to8(name))),
                           errorInfo.code,
   		  									 u16to8(errorInfo.key),
            							 u16to8(errorInfo.message));
    }

    /* process each field type ...*/
    PyDict_SetItem(val, u16to8(fieldDesc.name), get_field_value(line, fieldDesc));
	}

	return val;
}


static PyObject * get_field_value(DATA_CONTAINER_HANDLE hcont, RFC_FIELD_DESC fieldDesc){

  PyObject * pvalue;
  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_TABLE_HANDLE tableHandle;

  pvalue = NULL;
  switch (fieldDesc.type) {
    case RFCTYPE_DATE:
		  pvalue = get_date_value(hcont, fieldDesc.name);
		  break;
    case RFCTYPE_TIME:
		  pvalue = get_time_value(hcont, fieldDesc.name);
		  break;
    case RFCTYPE_NUM:
		  pvalue = get_num_value(hcont, fieldDesc.name, fieldDesc.nucLength);
		  break;
    case RFCTYPE_BCD:
		  pvalue = get_bcd_value(hcont, fieldDesc.name);
		  break;
    case RFCTYPE_CHAR:
		  pvalue = get_char_value(hcont, fieldDesc.name, fieldDesc.nucLength);
		  break;
    case RFCTYPE_BYTE:
		  pvalue = get_byte_value(hcont, fieldDesc.name, fieldDesc.nucLength);
		  break;
    case RFCTYPE_FLOAT:
		  pvalue = get_float_value(hcont, fieldDesc.name);
		  break;
    case RFCTYPE_INT:
		  pvalue = get_int_value(hcont, fieldDesc.name);
		  break;
    case RFCTYPE_INT2:
		  pvalue = get_int2_value(hcont, fieldDesc.name);
		  break;
    case RFCTYPE_INT1:
		  pvalue = get_int1_value(hcont, fieldDesc.name);
		  break;
    case RFCTYPE_STRUCTURE:
		  pvalue = get_structure_value(hcont, fieldDesc.name);
		  break;
    case RFCTYPE_TABLE:
      rc = RfcGetTable(hcont, fieldDesc.name, &tableHandle, &errorInfo);
      if (rc != RFC_OK) {
       	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetTable: %s",
		                                   PyString_AsString(u16to8(fieldDesc.name))),
                         errorInfo.code,
 												 u16to8(errorInfo.key),
      									 u16to8(errorInfo.message));
 	    }
		  pvalue = get_table_value(tableHandle, fieldDesc.name);
		  break;
    case RFCTYPE_XMLDATA:
		  fprintf(stderr, "shouldnt get a XMLDATA type parameter - abort\n");
			exit(1);
		  break;
    case RFCTYPE_STRING:
		  pvalue = get_string_value(hcont, fieldDesc.name);
		  break;
    case RFCTYPE_XSTRING:
		  pvalue = get_xstring_value(hcont, fieldDesc.name);
		  break;
		default:
		  fprintf(stderr, "This type is not implemented (%d) - abort\n", fieldDesc.type);
			exit(1);
		  break;
  }

  return pvalue;
}


static PyObject * get_table_line(RFC_STRUCTURE_HANDLE line){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_TYPE_DESC_HANDLE typeHandle;
	RFC_FIELD_DESC fieldDesc;
	unsigned fieldCount, i;
	PyObject * val;

	typeHandle = RfcDescribeType(line, &errorInfo);
  if (typeHandle == NULL) {
   	return SAPNW_rfc_call_error(PyString_FromString("Problem with RfcDescribeType"),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }

	rc = RfcGetFieldCount(typeHandle, &fieldCount, &errorInfo);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromString("Problem with RfcGetFieldCount"),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
          							 u16to8(errorInfo.message));
  }

  val = PyDict_New();
  for (i = 0; i < fieldCount; i++) {
	  rc = RfcGetFieldDescByIndex(typeHandle, i, &fieldDesc, &errorInfo);
    if (rc != RFC_OK) {
   	  return SAPNW_rfc_call_error(PyString_FromString("Problem with RfcGetFieldDescByIndex: "),
                           errorInfo.code,
   		  									 u16to8(errorInfo.key),
            							 u16to8(errorInfo.message));
    }

    /* process each field type ...*/
    PyDict_SetItem(val, u16to8(fieldDesc.name), get_field_value(line, fieldDesc));
	}

	return val;
}


static PyObject * get_table_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	PyObject * val;
	unsigned tabLen, r;
	RFC_STRUCTURE_HANDLE line;

	rc = RfcGetRowCount(hcont, &tabLen, NULL);
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetRowCount: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
   											 u16to8(errorInfo.key),
	     									 u16to8(errorInfo.message));
  }
	val = PyList_New(tabLen);
  for (r = 0; r < tabLen; r++){
	  RfcMoveTo(hcont, r, NULL);
	  line = RfcGetCurrentRow(hcont, NULL);
		 PyList_SetItem(val, r, get_table_line(line));
	}

	return val;
}


static PyObject * get_parameter_value(PyObject * name, SAPNW_FUNC *fptr){

	//SAPNW_CONN_INFO *cptr;
	SAPNW_FUNC_DESC *dptr;
  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_TABLE_HANDLE tableHandle;
  RFC_PARAMETER_DESC paramDesc;
	SAP_UC *p_name;
  PyObject * pvalue;

	dptr = fptr->desc_handle;
	//cptr = dptr->conn_handle;

  /* get the parameter description */
  rc = RfcGetParameterDescByName(dptr->handle, (p_name = u8to16(name)), &paramDesc, &errorInfo);

  /* bail on a bad call for parameter description */
  if (rc != RFC_OK) {
	  free(p_name);
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetParameterDescByName: %s",
		                                   PyString_AsString(name)),
                         errorInfo.code,
												 u16to8(errorInfo.key),
												 u16to8(errorInfo.message));
	}

  pvalue = NULL;
  switch (paramDesc.type) {
    case RFCTYPE_DATE:
		  pvalue = get_date_value(fptr->handle, p_name);
		  break;
    case RFCTYPE_TIME:
		  pvalue = get_time_value(fptr->handle, p_name);
		  break;
    case RFCTYPE_NUM:
		  pvalue = get_num_value(fptr->handle, p_name, paramDesc.nucLength);
		  break;
    case RFCTYPE_BCD:
		  pvalue = get_bcd_value(fptr->handle, p_name);
		  break;
    case RFCTYPE_CHAR:
		  pvalue = get_char_value(fptr->handle, p_name, paramDesc.nucLength);
		  break;
    case RFCTYPE_BYTE:
		  pvalue = get_byte_value(fptr->handle, p_name, paramDesc.nucLength);
		  break;
    case RFCTYPE_FLOAT:
		  pvalue = get_float_value(fptr->handle, p_name);
		  break;
    case RFCTYPE_INT:
		  pvalue = get_int_value(fptr->handle, p_name);
		  break;
    case RFCTYPE_INT2:
		  pvalue = get_int2_value(fptr->handle, p_name);
		  break;
    case RFCTYPE_INT1:
		  pvalue = get_int1_value(fptr->handle, p_name);
		  break;
    case RFCTYPE_STRUCTURE:
		  pvalue = get_structure_value(fptr->handle, p_name);
		  break;
    case RFCTYPE_TABLE:
      rc = RfcGetTable(fptr->handle, p_name, &tableHandle, &errorInfo);
      if (rc != RFC_OK) {
       	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetTable: %s",
		                                   PyString_AsString(u16to8(p_name))),
                         errorInfo.code,
 												 u16to8(errorInfo.key),
      									 u16to8(errorInfo.message));
 	    }
		  pvalue = get_table_value(tableHandle, p_name);
		  break;
    case RFCTYPE_XMLDATA:
		  fprintf(stderr, "shouldnt get a XMLDATA type parameter - abort\n");
			exit(1);
		  break;
    case RFCTYPE_STRING:
		  pvalue = get_string_value(fptr->handle, p_name);
		  break;
    case RFCTYPE_XSTRING:
		  pvalue = get_xstring_value(fptr->handle, p_name);
		  break;
		default:
		  fprintf(stderr, "This type is not implemented (%d) - abort\n", paramDesc.type);
			exit(1);
		  break;
  }
	free(p_name);


  return pvalue;
}


void set_date_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	SAP_UC *p_value;
	RFC_DATE date_value;

	if (! PyString_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetDate invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}
	if (PyString_Size(value) != 8) {
   	SAPNW_rfc_call_error1("RfcSetDate invalid date format:", PyString_AsString(value));
		return;
	}
  p_value = u8to16(value);
	memcpy((char *)date_value+0, (char *)p_value, 16);
  free(p_value);

  rc = RfcSetDate(hcont, name, date_value, &errorInfo);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetDate: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_time_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	SAP_UC *p_value;
	RFC_TIME time_value;

	if (! PyString_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetTime invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}
	if (PyString_Size(value) != 6) {
   	SAPNW_rfc_call_error1("RfcSetTime invalid date format:", PyString_AsString(value));
		return;
	}
  p_value = u8to16(value);
	memcpy((char *)time_value+0, (char *)p_value, 12);
  free(p_value);

  rc = RfcSetTime(hcont, name, time_value, &errorInfo);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetTime: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_num_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value, unsigned max){

    RFC_RC rc = RFC_OK;
    RFC_ERROR_INFO errorInfo;
    SAP_UC *p_value;

	if (! PyString_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetNum invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}
	if ((int)PyString_Size(value) > (int)max) {
   	SAPNW_rfc_call_error1("RfcSetNum string too long:", PyString_AsString(value));
		return;
	}

  p_value = u8to16(value);
  rc = RfcSetNum(hcont, name, (RFC_NUM *)p_value, strlenU(p_value), &errorInfo);
  free(p_value);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetNum: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_bcd_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	SAP_UC *p_value;

  /* make sure that the BCD source value is a string */
	if (! PyString_Check(value)){
   	SAPNW_rfc_call_error1("(bcd)RfcSetString invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}

  p_value = u8to16(value);
  rc = RfcSetString(hcont, name, p_value, strlenU(p_value), &errorInfo);
  free(p_value);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetBCD: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_char_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value, unsigned max){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	SAP_UC *p_value;

	if (! PyString_Check(value)){
       	SAPNW_rfc_call_error1("RfcSetChar invalid Input value type:", PyString_AsString(value));
		return;
	}

    p_value = u8to16(value);
    rc = RfcSetChars(hcont, name, p_value, strlenU(p_value), &errorInfo);
    free(p_value);
    if (rc != RFC_OK) {
   	    SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetChars: %s",
		                                     PyString_AsString(u16to8(name))),
                                             errorInfo.code,
  											 u16to8(errorInfo.key),
           							         u16to8(errorInfo.message));
		return;
    }

	return;
}


void set_byte_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value, unsigned max){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;

	if (! PyString_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetByte invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}
	if ((int)PyString_Size(value) > (int)max) {
   	SAPNW_rfc_call_error1("RfcSetByte string too long:", PyString_AsString(value));
		return;
	}
  rc = RfcSetBytes(hcont, name, (SAP_RAW *)PyString_AsString(value), PyString_Size(value), &errorInfo);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetBytes: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_float_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;

	if (! PyFloat_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetFloat invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}
  rc = RfcSetFloat(hcont, name, (RFC_FLOAT) PyFloat_AsDouble(value), &errorInfo);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetFloat: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_int_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;

	if (! PyInt_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetInt invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}
  rc = RfcSetInt(hcont, name, (RFC_INT) PyInt_AsLong(value), &errorInfo);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetInt: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_int1_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;

	if (! PyInt_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetInt1 invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}
	if (PyInt_AsLong(value) > 255){
   	SAPNW_rfc_call_error1("RfcSetInt1 invalid Input value too big on:", PyString_AsString(u16to8(name)));
		return;
	}
  rc = RfcSetInt1(hcont, name, (RFC_INT1) PyInt_AsLong(value), &errorInfo);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetInt1: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_int2_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;

	if (! PyInt_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetInt2 invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}
	if (PyInt_AsLong(value) > 4095){
   	SAPNW_rfc_call_error1("RfcSetInt2 invalid Input value too big on:", PyString_AsString(u16to8(name)));
		return;
	}
  rc = RfcSetInt2(hcont, name, (RFC_INT2) PyInt_AsLong(value), &errorInfo);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetInt2: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_string_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	SAP_UC *p_value;

	if (! PyString_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetString invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}

  p_value = u8to16(value);
  rc = RfcSetString(hcont, name, p_value, strlenU(p_value), &errorInfo);
  free(p_value);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetString: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_xstring_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;

	if (! PyString_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetXString invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
	}

  rc = RfcSetXString(hcont, name, (SAP_RAW *)PyString_AsString(value), PyString_Size(value), &errorInfo);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetXString: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	return;
}


void set_structure_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_STRUCTURE_HANDLE line;
	RFC_TYPE_DESC_HANDLE typeHandle;
	RFC_FIELD_DESC fieldDesc;
	SAP_UC *p_name;
	unsigned i, idx;
	Py_ssize_t pos;
	PyObject * key, * val;

	if (! PyDict_Check(value)){
   	SAPNW_rfc_call_error1("RfcSetStructure invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
  }

  idx = PyDict_Size(value);

  rc = RfcGetStructure(hcont, name, &line, &errorInfo);
  if (rc != RFC_OK) {
   	SAPNW_rfc_call_error(PyString_FromFormat("(set)Problem with RfcGetStructure: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	typeHandle = RfcDescribeType(line, &errorInfo);
  if (typeHandle == NULL) {
   	SAPNW_rfc_call_error(PyString_FromFormat("(set)Problem with RfcDescribeType: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;
  }

	pos = 0;
	i = 0;
	while (PyDict_Next(value, &pos, &key, &val)) {
	  rc = RfcGetFieldDescByName(typeHandle, (p_name = u8to16(key)), &fieldDesc, &errorInfo);
    if (rc != RFC_OK) {
   	  SAPNW_rfc_call_error(PyString_FromFormat("(set)Problem with RfcGetFieldDescByName: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
	   	return;
    }
// XXX bad copy
    memcpy(fieldDesc.name, p_name, strlenU(p_name)*2+2);
		free(p_name);
    set_field_value(line, fieldDesc, val);
		if (PyErr_Occurred() != NULL)
		  return;
		i++;
	}

	return;
}


void set_field_value(DATA_CONTAINER_HANDLE hcont, RFC_FIELD_DESC fieldDesc, PyObject * value){
  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_TABLE_HANDLE tableHandle;

  switch (fieldDesc.type) {
    case RFCTYPE_DATE:
		  set_date_value(hcont, fieldDesc.name, value);
		  break;
    case RFCTYPE_TIME:
		  set_time_value(hcont, fieldDesc.name, value);
		  break;
    case RFCTYPE_NUM:
		  set_num_value(hcont, fieldDesc.name, value, fieldDesc.nucLength);
		  break;
    case RFCTYPE_BCD:
		  set_bcd_value(hcont, fieldDesc.name, value);
		  break;
    case RFCTYPE_CHAR:
		  set_char_value(hcont, fieldDesc.name, value, fieldDesc.nucLength);
		  break;
    case RFCTYPE_BYTE:
		  set_byte_value(hcont, fieldDesc.name, value, fieldDesc.nucLength);
		  break;
    case RFCTYPE_FLOAT:
		  set_float_value(hcont, fieldDesc.name, value);
		  break;
    case RFCTYPE_INT:
		  set_int_value(hcont, fieldDesc.name, value);
		  break;
    case RFCTYPE_INT2:
		  set_int2_value(hcont, fieldDesc.name, value);
		  break;
    case RFCTYPE_INT1:
		  set_int1_value(hcont, fieldDesc.name, value);
		  break;
    case RFCTYPE_STRUCTURE:
		  set_structure_value(hcont, fieldDesc.name, value);
		  break;
    case RFCTYPE_TABLE:
      rc = RfcGetTable(hcont, fieldDesc.name, &tableHandle, &errorInfo);
      if (rc != RFC_OK) {
       	  SAPNW_rfc_call_error(PyString_FromFormat("(set_table)Problem with RfcGetTable: %s",
		                                   PyString_AsString(u16to8(fieldDesc.name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
	       	return;
    	}
		  set_table_value(tableHandle, fieldDesc.name, value);
		  break;
    case RFCTYPE_XMLDATA:
		  fprintf(stderr, "shouldnt get a XMLDATA type parameter - abort\n");
			exit(1);
		  break;
    case RFCTYPE_STRING:
		  set_string_value(hcont, fieldDesc.name, value);
		  break;
    case RFCTYPE_XSTRING:
		  set_xstring_value(hcont, fieldDesc.name, value);
		  break;
		default:
		  fprintf(stderr, "Set field - This type is not implemented (%d) - abort\n", fieldDesc.type);
			exit(1);
		  break;
  }

  return;
}


void set_table_line(RFC_STRUCTURE_HANDLE line, PyObject * value){

  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_TYPE_DESC_HANDLE typeHandle;
	RFC_FIELD_DESC fieldDesc;
	unsigned i, idx;
	SAP_UC * p_name;
	Py_ssize_t pos;
	PyObject * key, * val;


	if (! PyDict_Check(value)){
   	SAPNW_rfc_call_error1("set_table_line invalid Input value type", "");
		return;
  }
  idx = PyDict_Size(value);

	typeHandle = RfcDescribeType(line, &errorInfo);
  if (typeHandle == NULL) {
   	SAPNW_rfc_call_error(PyString_FromString("(set_table_line)Problem with RfcDescribeType"),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
		return;

  }

	pos = 0;
	i = 0;
	while (PyDict_Next(value, &pos, &key, &val)) {
	  rc = RfcGetFieldDescByName(typeHandle, (p_name = u8to16(key)), &fieldDesc, &errorInfo);
    if (rc != RFC_OK) {
   	  SAPNW_rfc_call_error(PyString_FromFormat("(set_table_line)Problem with RfcGetFieldDescByName: %s",
		                                   PyString_AsString(key)),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
	   	return;
    }
// XXX - bad copy
    memcpy(fieldDesc.name, p_name, strlenU(p_name)*2+2);
		free(p_name);
    set_field_value(line, fieldDesc, val);
		if (PyErr_Occurred() != NULL)
		  return;
		i++;
	}

	return;
}


void set_table_value(DATA_CONTAINER_HANDLE hcont, SAP_UC *name, PyObject * value){

  RFC_ERROR_INFO errorInfo;
	RFC_STRUCTURE_HANDLE line;
	unsigned r;
	PyObject * row;

	if (! PyList_Check(value)){
   	SAPNW_rfc_call_error1("set_table invalid Input value type:", PyString_AsString(u16to8(name)));
		return;
  }
	for (r = 0; (int)r < (int)PyList_Size(value); r++) {
		row = PyList_GetItem(value, r);
	  line = RfcAppendNewRow(hcont, &errorInfo);
    if (line == NULL) {
   	  SAPNW_rfc_call_error(PyString_FromFormat("(set_table)Problem with RfcAppendNewRow: %s",
		                                   PyString_AsString(u16to8(name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
	   	return;
    }
		set_table_line(line, row);
		if (PyErr_Occurred() != NULL)
		  return;
	}

	return;
}


void set_parameter_value(SAPNW_FUNC *fptr, PyObject * name, PyObject * value){

	SAPNW_CONN_INFO *cptr;
	SAPNW_FUNC_DESC *dptr;
  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_TABLE_HANDLE tableHandle;
  RFC_PARAMETER_DESC paramDesc;
	SAP_UC *p_name;

  if (value == Py_None) {
	  return;
	}

	dptr = fptr->desc_handle;
	cptr = dptr->conn_handle;

  /* get the parameter description */
  rc = RfcGetParameterDescByName(dptr->handle, (p_name = u8to16(name)), &paramDesc, &errorInfo);

  /* bail on a bad call for parameter description */
  if (rc != RFC_OK) {
	  free(p_name);
   	SAPNW_rfc_call_error(PyString_FromFormat("(set)Problem with RfcGetParameterDescByName: %s",
		                                   PyString_AsString(name)),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
	  return;
	}

  switch (paramDesc.type) {
    case RFCTYPE_DATE:
		  set_date_value(fptr->handle, p_name, value);
		  break;
    case RFCTYPE_TIME:
		  set_time_value(fptr->handle, p_name, value);
		  break;
    case RFCTYPE_NUM:
		  set_num_value(fptr->handle, p_name, value, paramDesc.nucLength);
		  break;
    case RFCTYPE_BCD:
		  set_bcd_value(fptr->handle, p_name, value);
		  break;
    case RFCTYPE_CHAR:
		  set_char_value(fptr->handle, p_name, value, paramDesc.nucLength);
		  break;
    case RFCTYPE_BYTE:
		  set_byte_value(fptr->handle, p_name, value, paramDesc.nucLength);
		  break;
    case RFCTYPE_FLOAT:
		  set_float_value(fptr->handle, p_name, value);
		  break;
    case RFCTYPE_INT:
		  set_int_value(fptr->handle, p_name, value);
		  break;
    case RFCTYPE_INT2:
		  set_int2_value(fptr->handle, p_name, value);
		  break;
    case RFCTYPE_INT1:
		  set_int1_value(fptr->handle, p_name, value);
		  break;
    case RFCTYPE_STRUCTURE:
		  set_structure_value(fptr->handle, p_name, value);
		  break;
    case RFCTYPE_TABLE:
      rc = RfcGetTable(fptr->handle, p_name, &tableHandle, &errorInfo);
      if (rc != RFC_OK) {
       	  SAPNW_rfc_call_error(PyString_FromFormat("(set_table)Problem with RfcGetTable: %s",
		                                   PyString_AsString(u16to8(p_name))),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
	       	return;
    	}
		  set_table_value(tableHandle, p_name, value);
		  break;
    case RFCTYPE_XMLDATA:
		  fprintf(stderr, "shouldnt get a XMLDATA type parameter - abort\n");
			exit(1);
		  break;
    case RFCTYPE_STRING:
		  set_string_value(fptr->handle, p_name, value);
		  break;
    case RFCTYPE_XSTRING:
		  set_xstring_value(fptr->handle, p_name, value);
		  break;
		default:
		  fprintf(stderr, "This type is not implemented (%d) - abort\n", paramDesc.type);
			exit(1);
		  break;
  }
	free(p_name);

  return;
}


/* Create a Function Module handle to be used for an RFC call */
static PyObject * sapnwrfc_set_active(sapnwrfc_FuncCallObject* self, PyObject * name, PyObject * active){

	SAPNW_CONN_INFO *cptr;
	SAPNW_FUNC_DESC *dptr;
	SAPNW_FUNC *fptr;
  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	SAP_UC *p_name;

	fptr = self->handle;
	dptr = fptr->desc_handle;
	cptr = dptr->conn_handle;

  rc = RfcSetParameterActive(fptr->handle, (p_name = u8to16(name)), PyLong_AsLong(active), &errorInfo);
	free(p_name);
  if (rc != RFC_OK)
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcSetParameterActive: %s",
		                                   PyString_AsString(name)),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));

  return Py_BuildValue( "i", ( int ) 1);
}



/* Create a Function Module handle to be used for an RFC call */
static PyObject * sapnwrfc_invoke(sapnwrfc_FuncCallObject* self){

	SAPNW_CONN_INFO *cptr;
	SAPNW_FUNC_DESC *dptr;
	SAPNW_FUNC *fptr;
  RFC_RC rc = RFC_OK;
  RFC_ERROR_INFO errorInfo;
	RFC_TABLE_HANDLE tableHandle;
	PyObject *parameters, *parm, *name, *value, *row;
	SAP_UC *p_name;
	int idx, r;
	unsigned tabLen;
	RFC_STRUCTURE_HANDLE line;
	Py_ssize_t pos;


	fptr = self->handle;
	dptr = fptr->desc_handle;
	cptr = dptr->conn_handle;

	/* loop through all Input/Changing/tables parameters and set the values in the call */
	parameters = PyObject_GetAttrString((PyObject*)self, "parameters");
	/*
	fprintf(stderr, "parameters(in invoke): ");
  PyObject_Print(parameters, stderr, Py_PRINT_RAW);
	fprintf(stderr, "\n");
	*/
  idx = PyDict_Size(parameters);
	//fprintf(stderr, "parameters: %d\n", idx);

	pos = 0;
	while (PyDict_Next(parameters, &pos, &name, &parm)) {
	   switch(PyLong_AsLong(PyObject_GetAttrString(parm, "direction"))) {
     	 case RFC_EXPORT:
			   break;
     	 case RFC_IMPORT:
			 case RFC_CHANGING:
			   value = PyObject_GetAttrString(parm, "value");
				 set_parameter_value(fptr, name, value);
			   break;
     	 case RFC_TABLES:
			   value = PyObject_GetAttrString(parm, "value");
				 if (value == Py_None)
				   continue;
	       if (! PyList_Check(value))
           return SAPNW_rfc_call_error1("RFC_TABLES requires LIST:", PyString_AsString(name));
         rc = RfcGetTable(fptr->handle, (p_name = u8to16(name)), &tableHandle, &errorInfo);
         if (rc != RFC_OK) {
   	       return SAPNW_rfc_call_error(PyString_FromFormat("(set)Problem with RfcGetTable: %s",
		                                   PyString_AsString(name)),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
       	 }
	       for (r = 0; r < PyList_Size(value); r++) {
       		 row = PyList_GetItem(value, r);
					 line = RfcAppendNewRow(tableHandle, &errorInfo);
           if (line == NULL) {
   	         return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcAppendNewRow: %s",
		                                   PyString_AsString(name)),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
         	 }
					 set_table_line(line, row);
		       if (PyErr_Occurred() != NULL)
        		  return NULL;
				 }

				 free(p_name);
			   break;
     	 default:
			    fprintf(stderr, "should NOT get here!\n");
					exit(1);
			   break;
		 }
		 if (PyErr_Occurred() != NULL){
		   /* TODO: How to do logging from extension modules?
		    * fprintf(stderr, "Ooops!\n"); */
       return NULL;
		 }
  }

	rc = RfcInvoke(cptr->handle, fptr->handle, &errorInfo);

  /* bail on a bad RFC Call */
  if (rc != RFC_OK) {
   	return SAPNW_rfc_call_error(PyString_FromFormat("Problem Invoking RFC: %s",
		                                 dptr->name),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
	}

	pos = 0;
	while (PyDict_Next(parameters, &pos, &name, &parm)) {
	   switch(PyLong_AsLong(PyObject_GetAttrString(parm, "direction"))) {
     	 case RFC_IMPORT:
			   break;
     	 case RFC_EXPORT:
     	 case RFC_CHANGING:
				 value = get_parameter_value(name, fptr);
	       PyObject_SetAttrString((PyObject*)parm, "value", value);
			   break;
     	 case RFC_TABLES:
         rc = RfcGetTable(fptr->handle, (p_name = u8to16(name)), &tableHandle, &errorInfo);
         if (rc != RFC_OK) {
   	       return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetTable: %s",
		                                   PyString_AsString(name)),
                         errorInfo.code,
  											 u16to8(errorInfo.key),
           							 u16to8(errorInfo.message));
       	 }
				 rc = RfcGetRowCount(tableHandle, &tabLen, NULL);
         if (rc != RFC_OK) {
   	       return SAPNW_rfc_call_error(PyString_FromFormat("Problem with RfcGetRowCount: %s",
		                                   PyString_AsString(name)),
                               errorInfo.code,
       												 u16to8(errorInfo.key),
			       									 u16to8(errorInfo.message));
       	 }
	       value = PyList_New(tabLen);
         for (r = 0; (int)r < (int)tabLen; r++){
	         RfcMoveTo(tableHandle, r, NULL);
			     line = RfcGetCurrentRow(tableHandle, NULL);
		       PyList_SetItem(value, r, get_table_line(line));
		       if (PyErr_Occurred() != NULL)
             return NULL;
				 }
				 free(p_name);
	       PyObject_SetAttrString((PyObject*)parm, "value", value);
			   break;
		 }
		 if (PyErr_Occurred() != NULL)
       return NULL;
  }
  return Py_BuildValue( "i", ( int ) 1);
}







static PyMemberDef sapnwrfc_Conn_members[] = {
    {"handle", T_OBJECT_EX, offsetof(sapnwrfc_ConnObject, connInfo), 0, "conection handle"},
    {NULL}  /* Sentinel */
};


static PyMethodDef sapnwrfc_Conn_methods[] = {
//    {"new_connection", (PyCFunction)sapnwrfc_conn_new, METH_VARARGS, "Create a new Connection object."},
    {"connection_attributes", (PyCFunction)sapnwrfc_connection_attributes, METH_VARARGS, "Connection Attributes"},
    {"function_lookup", (PyCFunction)sapnwrfc_function_lookup, METH_VARARGS, "discover an RFC"},
    {"close", (PyCFunction)sapnwrfc_conn_close, METH_VARARGS, "Close a Connection object."},
    {NULL, NULL, 0, NULL}
};


static PyTypeObject sapnwrfc_ConnType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "nwsaprfcutil.Conn",             /*tp_name*/
    sizeof(sapnwrfc_ConnObject),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)sapnwrfc_Conn_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "sapnwrfc Conn objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    sapnwrfc_Conn_methods,             /* tp_methods */
    sapnwrfc_Conn_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)sapnwrfc_Conn_init,      /* tp_init */
    0,                         /* tp_alloc */
    sapnwrfc_Conn_new,                 /* tp_new */
};










static PyMemberDef sapnwrfc_FuncDesc_members[] = {
    {"handle", T_OBJECT_EX, offsetof(sapnwrfc_FuncDescObject, handle), 0, "FuncDesc handle"},
    {"name", T_OBJECT_EX, offsetof(sapnwrfc_FuncDescObject, name), 0, "FuncDesc name"},
    {"parameters", T_OBJECT_EX, offsetof(sapnwrfc_FuncDescObject, parameters), 0, "FuncDesc parameters"},
    {NULL}  /* Sentinel */
};


static PyMethodDef sapnwrfc_FuncDesc_methods[] = {
    {"create_function_call", (PyCFunction)sapnwrfc_create_function_call, METH_VARARGS, "Create a Function Call Instance"},
    {NULL, NULL, 0, NULL}
};


static PyTypeObject sapnwrfc_FuncDescType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "nwsaprfcutil.FuncDesc",             /*tp_name*/
    sizeof(sapnwrfc_FuncDescObject),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)sapnwrfc_FuncDesc_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "sapnwrfc Function Descriptor objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    sapnwrfc_FuncDesc_methods,             /* tp_methods */
    sapnwrfc_FuncDesc_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)sapnwrfc_FuncDesc_init,      /* tp_init */
    0,                         /* tp_alloc */
    sapnwrfc_FuncDesc_new,                 /* tp_new */
};



static PyMemberDef sapnwrfc_FuncCall_members[] = {
    {"handle", T_OBJECT_EX, offsetof(sapnwrfc_FuncCallObject, handle), 0, "FuncCall handle"},
    {"name", T_OBJECT_EX, offsetof(sapnwrfc_FuncCallObject, name), 0, "FuncCall name"},
    {"parameters", T_OBJECT_EX, offsetof(sapnwrfc_FuncCallObject, parameters), 0, "FuncCall parameters"},
    {"function_descriptor", T_OBJECT_EX, offsetof(sapnwrfc_FuncCallObject, function_descriptor), 0, "FuncCall function_descriptor"},
    {NULL}  /* Sentinel */
};


static PyMethodDef sapnwrfc_FuncCall_methods[] = {
    {"set_active", (PyCFunction)sapnwrfc_set_active, METH_VARARGS, "Activate/Deactivate a parameter"},
    {"invoke", (PyCFunction)sapnwrfc_invoke, METH_VARARGS, "Do the Function Call"},
    {NULL, NULL, 0, NULL}
};



static PyTypeObject sapnwrfc_FuncCallType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "nwsaprfcutil.FuncCall",             /*tp_name*/
    sizeof(sapnwrfc_FuncCallObject),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)sapnwrfc_FuncCall_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "sapnwrfc Fuction Call objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    sapnwrfc_FuncCall_methods,             /* tp_methods */
    sapnwrfc_FuncCall_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)sapnwrfc_FuncCall_init,      /* tp_init */
    0,                         /* tp_alloc */
    sapnwrfc_FuncCall_new,                 /* tp_new */
};


static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};



#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initnwsaprfcutil(void)
{
    PyObject* m;
    PyObject* s;


    E_RFC_COMMS = PyErr_NewException("sapnwrfc.RFCCommunicationError", NULL, NULL);
    E_RFC_SERVER = PyErr_NewException("sapnwrfc.RFCServerError", NULL, NULL);
    E_RFC_FUNCCALL = PyErr_NewException("sapnwrfc.RFCFunctionCallError", NULL, NULL);

    if (PyType_Ready(&sapnwrfc_ConnType) < 0)
        return;

    if (PyType_Ready(&sapnwrfc_FuncDescType) < 0)
        return;

    if (PyType_Ready(&sapnwrfc_FuncCallType) < 0)
        return;

    m = Py_InitModule3("nwsaprfcutil", module_methods,
                       "nwsaprfc extension type.");

    if (m == NULL)
      return;

    Py_INCREF(&sapnwrfc_ConnType);
    PyModule_AddObject(m, "Conn", (PyObject *)&sapnwrfc_ConnType);
    Py_INCREF(&sapnwrfc_FuncDescType);
    PyModule_AddObject(m, "FuncDesc", (PyObject *)&sapnwrfc_FuncDescType);
    Py_INCREF(&sapnwrfc_FuncCallType);
    PyModule_AddObject(m, "FuncCall", (PyObject *)&sapnwrfc_FuncCallType);

    // find sapnwrfc by name and then add the errors to it
    s = PyImport_Import(PyString_FromString("sapnwrfc"));
    Py_INCREF(E_RFC_COMMS);
    PyModule_AddObject(s, "RFCCommunicationError", E_RFC_COMMS);
    Py_INCREF(E_RFC_SERVER);
    PyModule_AddObject(s, "RFCServerError", E_RFC_SERVER);
    Py_INCREF(E_RFC_FUNCCALL);
    PyModule_AddObject(s, "RFCFunctionCallError", E_RFC_FUNCCALL);

}

