#include <system/macros.h>

// quom includes
#include <core/q5.h>
#include <core/q5.h>
#include <core/q5modules.h>
#include <core/q5core.h>

// entc includes
#include <tools/ecasyncvc.h>
#include <utils/ecmessages.h>
#include <tools/ecdata.h>
#include <tools/ecjson.h>

// project includes
#include "prot_context.h"
#include "realm.h"

//================================================================================================

static Q5ModuleProperties properties = {"GASRV", "Game server", 10001, 0x4001};

typedef struct {
  
  Q5ModuleInstance* instance;

  // reference
  Q5Core core;
  
  EcAsync async;

  uint_t port;
  
  EcString host;
  
  // reference
  EcAsynUdpDispatcher dispatcher;
  
  GameServerRealm grealm;
  
} Q5Module;

typedef struct {
  
  uint_t api;
  
  uint_t data;
  
  EcMessageData msdata;
  
} Q5ModuleServiceNums;

// add exported methods of this shared library
#include <core/q5module.inc>

//===========================================================================================================

void module_config (Q5Module* self, const EcUdc item)
{ 
}

//-----------------------------------------------------------------------------------------------------------

int _STDCALL module_callback_maintenance (void* ptr, EcMessageData* dIn, EcMessageData* dOut)
{
  // casts
  Q5Module* self = ptr;

  EcBuffer buf = ecbuf_create (10);

  ecbuf_format(buf, 10, "00");
  
  ecasync_udpdisp_broadcast (self->dispatcher, buf, 2, NULL);
  
  ecbuf_destroy(&buf);
  
  return ENTC_RESCODE_OK;
}

//-------------------------------------------------------------------------------------------

EcAsyncUdpContext _STDCALL ecasync_onDispatch (void* ptr)
{
  Q5Module* self = ptr;
  return gs_realm_newContext (self->grealm);
}

//-------------------------------------------------------------------------------------

int module_start (Q5Module* self)
{
  EcAsynUdpDispatcher dispatcher;
  
  // create asyncronous server with 4 threads
  self->async = ecasync_create (4);
  
  // try to create an UDP dispatcher
  dispatcher = ecasync_udpdisp_create (self->host, self->port, q5core_getTerm (self->core), ecasync_onDispatch, self);
  if (dispatcher)
  {
    EcAsyncContext context;

    // keep a reference for later
    self->dispatcher = dispatcher;
    
    // convert dispatcher to async context, dispatcher will be freed by the context
    context = ecasync_udpdisp_context (&dispatcher);
    
    // add the context to all threads
    ecasync_addToAll (self->async, &context);
    
    eclogger_fmt (LL_DEBUG, properties.name, "server", "listen '%s' on port '%u'", self->host, self->port);

    ecmessages_add (self->instance->msgid, Q5_SERVICE_MAINTENANCE, module_callback_maintenance, self);

    self->grealm = gs_realm_create (self->dispatcher);
    
    return TRUE;
  }
  else 
  {
    return FALSE;
  }
}

//-------------------------------------------------------------------------------------

int module_stop (Q5Module* self)
{
  gs_realm_destroy(&(self->grealm));
  
  ecasync_destroy (&(self->async));
  
  eclogger_fmt (LL_DEBUG, properties.name, "server", "stopped");
  
  return TRUE;
}

//-------------------------------------------------------------------------------------

Q5Module* module_create (Q5Core core, Q5ModuleInstance* instance)
{
  Q5Module* self = ENTC_NEW (Q5Module);
  
  self->instance = instance;
  self->core = core;
  
  self->port = 44000;
  self->host = ecstr_copy ("0.0.0.0");
  
  self->async = NULL;
  self->dispatcher = NULL;
  
  self->grealm = NULL;
  
  return self;
}
  
//-------------------------------------------------------------------------------------

void module_destroy (Q5Module** pself)
{    
  Q5Module* self = *pself;
  
  ecstr_delete (&(self->host));
  
  ENTC_DEL (pself, Q5Module);
}

//-------------------------------------------------------------------------------------
