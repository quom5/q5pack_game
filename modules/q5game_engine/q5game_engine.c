#include <system/macros.h>

// quom includes
#include <core/q5.h>
#include <core/q5modules.h>
#include <core/q5core.h>
#include <core/q5auth.h>

// entc includes
#include <adbo.h>
#include <adbo_context.h>

#include <tools/ecjson.h>
#include <types/ecmap.h>
#include <tools/ecdata.h>
#include <utils/ecmessages.h>


//================================================================================================

static Q5ModuleProperties properties = {"GAEGN", "game engine", 10001, 0x4001};

typedef struct {
  
  // reference
  Q5Core core;
  
  Q5ModuleInstance* instance;
  
  EcListNode logger_node;
  
  AdboContext adbo;
  
  EcUdc root;
  
  EcMap tokens;

  EcMutex mutex;
    
} Q5Module;

typedef struct {
  
  EcString userid;
  
  EcString name;
  
} Q5AuthToken;

// add exported methods of this shared library
#include <core/q5module.inc>

//================================================================================================

int module_get_userinfo (Q5Module* self, const EcString username, const EcString realm, EcString* crc, EcUserInfo userInfo)
{
  EcUdc values;
  EcUdc item;
  EcUdc table;
  void* cursor = NULL;

  //eclogger_msg (LL_DEBUG, properties.name, "userinfo", "start");
  
  if (isNotAssigned (self->root))
  {
    eclogger_msg (LL_ERROR, properties.name, "userinfo", "root is not assigned");
    return FALSE;    
  }
  
  table = adbo_get_table (self->root, "users");
  if (isNotAssigned (table))
  {
    eclogger_msg (LL_ERROR, properties.name, "userinfo", "no table users");
    return FALSE;    
  }
  
  {
    EcUdc params = ecudc_create (ENTC_UDC_NODE, "data");
    // username
    ecudc_add_asString(params, "username", username);
    ecudc_add_asString(params, "realm", realm);
    
    adbo_item_fetch (table, params, self->adbo);
    
    ecudc_destroy(&params);
  }  
  
  values = adbo_item_values (table);
  if (isNotAssigned (values))
  {
    eclogger_msg (LL_ERROR, properties.name, "userinfo", "no result #1 values found");
    return FALSE;
  }
  
  item = ecudc_next (values, &cursor);
  // iterrate through all nodes
  if (isNotAssigned (item))
  {
    EcString v = ecjson_write (values);
    
    eclogger_fmt (LL_ERROR, properties.name, "userinfo", "no result #2 values found: %s", v);
    
    ecstr_delete(&v);
    
    return FALSE;
  }

  userInfo->name = ecstr_copy(ecudc_get_asString(item, "name", NULL));
  userInfo->uid = ecudc_get_asUInt64(item, "id", 0);
    
  ecstr_replace (crc, ecudc_get_asString(item, "crc", NULL));
  
  //eclogger_fmt (LL_TRACE, properties.name, "userinfo", "done with id:'%s' name:'%s'", ecudc_get_asString(item, "id", NULL), ecudc_get_asString(item, "name", NULL));
  
  return TRUE;  
}

//-----------------------------------------------------------------------------------------------------------

EcUdc module_check (Q5Module* self, const EcString username, const EcString password)
{
  EcUdc res;
  EcUdc values;
  EcUdc item;
  EcUdc table;
  void* cursor = NULL;
  const EcString origin_password;
  
  //eclogger_msg (LL_DEBUG, properties.name, "check", "start");
  
  if (isNotAssigned (self->root))
  {
    eclogger_msg (LL_ERROR, properties.name, "check", "root is not assigned");
    return NULL;    
  }

  table = adbo_get_table (self->root, "users");
  if (isNotAssigned (table))
  {
    eclogger_msg (LL_ERROR, properties.name, "check", "no table users");
    return NULL;    
  }
  
  {
    EcUdc params = ecudc_create (ENTC_UDC_NODE, "data");
    // username
    ecudc_add_asString(params, "username", username);
    
    adbo_item_fetch (table, params, self->adbo);
    
    ecudc_destroy(&params);
  }  
  
  values = adbo_item_values (table);
  if (isNotAssigned (values))
  {
    eclogger_msg (LL_ERROR, properties.name, "check", "no result #1 values found");
    return NULL;
  }
  
  item = ecudc_next (values, &cursor);
  // iterrate through all nodes
  if (isNotAssigned (item))
  {
    EcString v = ecjson_write (values);
    
    eclogger_fmt (LL_ERROR, properties.name, "check", "no result #2 values found: %s", v);

    ecstr_delete(&v);
    
    return NULL;
  }
  
  origin_password = ecudc_get_asString(item, "password", NULL);
  if (!ecstr_equal (origin_password, password))
  {
    eclogger_msg (LL_ERROR, properties.name, "check", "passwords dont match");
    return NULL;
  }
  
  res = ecudc_create (ENTC_UDC_NODE, "auth_credentials");
  
  ecudc_add_asString (res, "name", ecudc_get_asString(item, "name", NULL));
  ecudc_add_asString (res, "id", ecudc_get_asString(item, "id", NULL));
  
  //eclogger_fmt (LL_TRACE, properties.name, "check", "done with id:'%s' name:'%s'", ecudc_get_asString(item, "id", NULL), ecudc_get_asString(item, "name", NULL));
  
  return res;
}

//-----------------------------------------------------------------------------------------------------------

int module_check_basic_auth (Q5Module* self, EcUdc auth, EcUserInfo userInfo)
{
  const EcString username = ecudc_get_asString(auth, "username", NULL);
  const EcString response = ecudc_get_asString(auth, "response", NULL);
  const EcString realm = ecudc_get_asString(auth, "realm", NULL);

  EcString crc = ecstr_init ();
  int res;
  int ret = ENTC_RESCODE_NEEDS_PERMISSION;

  if (isNotAssigned (username) || isNotAssigned (response) || isNotAssigned (realm))
  {   
    eclogger_msg (LL_TRACE, properties.name, "basic auth", "no credentials given, send NEEDS AUTH response");
    return ENTC_RESCODE_NEEDS_AUTH;
  }
    
  ecmutex_lock (self->mutex);
  
  res = module_get_userinfo (self, username, realm, &crc, userInfo);
  
  ecmutex_unlock (self->mutex);
  
  if (ecstr_valid(crc) && (userInfo->uid > 0))
  {      
    // response and out calculated checksum must be identical
    if (ecstr_equal(crc, response))
    {
      ret = ENTC_RESCODE_OK;
    }
    else
    {
      ret = ENTC_RESCODE_NEEDS_PERMISSION;        
    }
  }
  
  return ret;
}

//-----------------------------------------------------------------------------------------------------------

int module_check_digest_auth (Q5Module* self, EcUdc auth, EcUserInfo userInfo)
{
  int ret = ENTC_RESCODE_NEEDS_AUTH;

  const EcString username = ecudc_get_asString(auth, "username", NULL);
  const EcString realm = ecudc_get_asString(auth, "realm", NULL);
  const EcString nonce = ecudc_get_asString(auth, "nonce", NULL);
  const EcString h2 = ecudc_get_asString(auth, "h2", NULL);
  const EcString response = ecudc_get_asString(auth, "response", NULL);
  const EcString qop = ecudc_get_asString(auth, "qop", NULL);
  const EcString nc = ecudc_get_asString(auth, "nc", NULL);
  const EcString cnonce = ecudc_get_asString(auth, "cnonce", NULL);
  
  if (isAssigned (username) && isAssigned (realm) && isAssigned (nonce) && isAssigned (h2) && isAssigned (response) && isAssigned (qop) && isAssigned (nc) && isAssigned (cnonce))
  {
    EcString crc = ecstr_init ();
    int res;
    
    ecmutex_lock (self->mutex);

    res = module_get_userinfo (self, username, realm, &crc, userInfo);
    
    ecmutex_unlock (self->mutex);

    if (ecstr_valid(crc) && (userInfo->uid > 0))
    {      
      
      EcBuffer re0;
      EcBuffer re1;
      
      EcStream s = ecstream_new ();
      
      ecstream_append (s, crc);
      ecstream_append (s, ":");
      ecstream_append (s, nonce);
      ecstream_append (s, ":");
      ecstream_append (s, nc);
      ecstream_append (s, ":");
      ecstream_append (s, cnonce);
      ecstream_append (s, ":");
      ecstream_append (s, qop);
      ecstream_append (s, ":");
      ecstream_append (s, h2);
      
      re0 = ecstream_trans (&s);
      re1 = ecbuf_md5 (re0);
      
      //eclogger_fmt (LL_TRACE, properties.name, "digest", "HA1: %s", crc);
      //eclogger_fmt (LL_TRACE, properties.name, "digest", "HA2: %s", h2);
      //eclogger_fmt (LL_TRACE, properties.name, "digest", "RE1: %s", ecbuf_const_str(re1));
      //eclogger_fmt (LL_TRACE, properties.name, "digest", "REO: %s", response);  
      
      // response and out calculated checksum must be identical
      if (ecstr_equal(ecbuf_const_str(re1), response))
      {
        ret = ENTC_RESCODE_OK;
      }
      else
      {
        ret = ENTC_RESCODE_NEEDS_PERMISSION;        
      }
      
      ecbuf_destroy (&re1);
      ecbuf_destroy (&re0);
    }
    else
    {
      ret = ENTC_RESCODE_NEEDS_PERMISSION;              
    }
    
    ecstr_delete (&crc);
  }
  
  return ret;  
}

//-----------------------------------------------------------------------------------------------------------

int _STDCALL module_callback_stat (void* ptr, EcMessageData* dIn, EcMessageData* dOut)
{
  if (isAssigned (dIn) && (dIn->rev == 1) && isAssigned (dOut))
  {
    EcUserInfo info;

    ecmessages_initDataN (dOut, Q5_MSGTYPE_USERINFO, 1, 0, ENTC_UDC_USERINFO, NULL);
    
    info = ecudc_asUserInfo (dOut->content);

    switch (dIn->type)
    {
      case Q5_MSGTYPE_AUTH_BASIC:
      {
        return module_check_basic_auth (ptr, dIn->content, info);
      }
      break;
      case Q5_MSGTYPE_AUTH_DIGEST:
      {
        return module_check_digest_auth (ptr, dIn->content, info);
      }
      break;
    }
  }
  
  return ENTC_RESCODE_IGNORE;
}

//-----------------------------------------------------------------------------------------------------------

int _STDCALL module_getPath (void* ptr, EcString* path, EcUserInfo userInfo, EcUdc* userData, EcUdc content, EcUdc object)
{
  const EcString type;

  EcUdc auth = ecudc_node (content, ECDATA_AUTH);
  if (isNotAssigned (auth))
  {
    return ENTC_RESCODE_NEEDS_AUTH;
  }
  
  type = ecudc_get_asString(auth, "type", NULL);
  if (ecstr_equal(type, "basic"))
  {
    return module_check_basic_auth (ptr, auth, userInfo);
  }
  else if (ecstr_equal(type, "digest"))
  {
    return module_check_digest_auth (ptr, auth, userInfo);
  }      
  
  return ENTC_RESCODE_OK;
}

//===========================================================================================================

int _STDCALL module_get (void* ptr, EcMessageData* dOut, EcUserInfo userInfo, EcUdc content)
{
  EcUserInfo info;

  ecmessages_initDataN (dOut, Q5_MSGTYPE_USERINFO, 1, 0, ENTC_UDC_USERINFO, NULL);

  info = ecudc_asUserInfo (dOut->content);

  info->name = ecstr_copy (userInfo->name);
  info->uid = userInfo->uid;
  
  return ENTC_RESCODE_OK;
}

//===========================================================================================================

int _STDCALL module_callback_get (void* ptr, EcMessageData* dIn, EcMessageData* dOut)
{
  if (isAssigned (dIn))
  {
    switch (dIn->type)
    {
      case Q5_MSGTYPE_STATE_INIT:
      {
        return auth_checkF0 (ptr, dIn, dOut, module_getPath, 0);
      }
      case Q5_MSGTYPE_STATE_PROCESS:
      {
        return auth_processF0 (ptr, dIn, dOut, module_get);        
      }
    }
  }
  return ENTC_RESCODE_IGNORE;
}

//-----------------------------------------------------------------------------------------------------------

int _STDCALL module_callback_opts (void* ptr, EcMessageData* dIn, EcMessageData* dOut)
{
  if (isAssigned (dOut))
  {
    EcUdc options = ecudc_create(ENTC_UDC_NODE, NULL);
    
    EcUdc methods = ecudc_create(ENTC_UDC_LIST, "methods");
    
    ecudc_add_asString (methods, "", "Digest");
    ecudc_add_asString (methods, "", "Basic");
    
    ecudc_add (options, &methods);
    
    dOut->content = options; dOut->rev = 1;
    
    dOut->ref = 0;
    dOut->type = Q5_MSGTYPE_HTTP_AUTH_OPTIONS;
    
    eclogger_msg (LL_DEBUG, properties.name, "options", "send auth options");
    
    return ENTC_RESCODE_OK;
  }
  
  return ENTC_RESCODE_IGNORE;
}

//-----------------------------------------------------------------------------------------------------------

int _STDCALL module_callback_del (void* ptr, EcMessageData* dIn, EcMessageData* dOut)
{
  return ENTC_RESCODE_CLEAR_AUTH;
}

//-----------------------------------------------------------------------------------------------------------

void module_config (Q5Module* self, const EcUdc result_item)
{
}

//-------------------------------------------------------------------------------------

int module_start (Q5Module* self)
{    
  self->mutex = ecmutex_new ();

  self->tokens = ecmap_new ();
  
  self->adbo = adbo_context_create (q5core_getEventFiles (self->core), q5core_getConfPath(self->core), q5core_getExecPath(self->core));
  
  self->root = NULL;
  
  {
    EcXMLStream xmlstream;
    
    xmlstream = ecxmlstream_openfile("adbo/auth.xml", q5core_getConfPath (self->core));
    /* parse the xml structure */
    while( ecxmlstream_nextNode( xmlstream ) )
    {
      if( ecxmlstream_isBegin( xmlstream, "nodes" ) )
      {
        self->root = adbo_structure_fromXml (self->adbo, xmlstream, "root", "nodes");
      }
    }
    
    ecxmlstream_close(xmlstream);    
  }
  
  eclogger_fmt (LL_TRACE, self->instance->name, "start", "register callbacks for %i", self->instance->msgid);
  
  // register the callback method
  ecmessages_add (self->instance->msgid, Q5_GEN_OPTS, module_callback_opts, self);
  ecmessages_add (self->instance->msgid, Q5_NODE_STAT, module_callback_stat, self);
  
  ecmessages_add (self->instance->msgid, Q5_DATA_GET, module_callback_get, self);
  ecmessages_add (self->instance->msgid, Q5_DATA_DEL, module_callback_del, self);
  
  return TRUE;
}

//-------------------------------------------------------------------------------------

int module_stop (Q5Module* self)
{
  // remove the callback method
  ecmessages_removeAll (self->instance->msgid);

  ecmutex_delete (&(self->mutex));

  adbo_context_destroy(&(self->adbo));
  
  if (isAssigned (self->root))
  {
    ecudc_destroy(&(self->root));
  }
  
  ecmap_delete (&(self->tokens));
  
  return TRUE;
}

//-------------------------------------------------------------------------------------

Q5Module* module_create (Q5Core core, Q5ModuleInstance* instance)
{
  Q5Module* self = ENTC_NEW (Q5Module);
  
  self->core = core;
  self->instance = instance;
  self->tokens = NULL;
  
  return self;
}
  
//-------------------------------------------------------------------------------------

void module_destroy (Q5Module** pself)
{    
  //Q5Module* self = *pself;
  
  ENTC_DEL (pself, Q5Module);
}

//-------------------------------------------------------------------------------------
