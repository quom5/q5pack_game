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
#include <system/ectime.h>
#include <system/ecthread.h>

// project includes
#include "prot_context.h"
#include "realm.h"

// enet includes
#include <enet/enet.h>

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
  
  ENetHost* server;
  
  EcList threads;
  
  int done;
  
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
  /*
  // casts
  Q5Module* self = ptr;

  EcBuffer buf = ecbuf_create (10);

  ecbuf_format(buf, 10, "00");
  
  ecasync_udpdisp_broadcast (self->dispatcher, buf, 2, NULL);
  
  ecbuf_destroy(&buf);
  */
  return ENTC_RESCODE_OK;
   
}

//-------------------------------------------------------------------------------------------

/*
EcAsyncUdpContext _STDCALL ecasync_onDispatch (void* ptr)
{
  Q5Module* self = ptr;
  return gs_realm_newContext (self->grealm);
}
 */

//-----------------------------------------------------------------------------------------------------------

static void _STDCALL module_context_destroy (void** ptr)
{
  Q5Module* self = *ptr;

}

//-----------------------------------------------------------------------------------------------------------

static EcHandle _STDCALL module_context_handle (void* ptr)
{
  Q5Module* self = ptr;
  
  // this is the current socket
  return self->server->socket;
}

//-----------------------------------------------------------------------------------------------------------

static int _STDCALL module_context_run (void* ptr)
{
  Q5Module* self = ptr;
  ENetEvent event;
  
  eclogger_fmt (LL_TRACE, properties.name, "server", "got message");
  
  if (enet_host_service (self->server, &event, 10) == 0)
  {
    eclogger_fmt (LL_WARN, properties.name, "server", "enet did not catch any event");
    return TRUE;
  }
  
  switch (event.type)
  {
    case ENET_EVENT_TYPE_NONE:
    {
      
    }
    break;
    case ENET_EVENT_TYPE_CONNECT:
    {
      eclogger_fmt (LL_ERROR, properties.name, "server", "A new client connected from %x:%u.\n", event.peer->address.host, event.peer -> address.port);

      /* Store any relevant client information here. */
      event.peer->data = "Client information";
    }
    break;
    case ENET_EVENT_TYPE_RECEIVE:
    {
      eclogger_fmt (LL_ERROR, properties.name, "server", "A packet of length %u containing %s was received from %s on channel %u.\n",
              event.packet -> dataLength,
              event.packet -> data,
              event.peer -> data,
              event.channelID);
      /* Clean up the packet now that we're done using it. */
      enet_packet_destroy (event.packet);
      
    }
    break;
    case ENET_EVENT_TYPE_DISCONNECT:
    {
      eclogger_fmt (LL_ERROR, properties.name, "server", "%s disconnected.\n", event.peer -> data);
      /* Reset the peer's client information. */
      event.peer -> data = NULL;
    }
    break;
  }

  return TRUE;
}

//-----------------------------------------------------------------------------------------------------------

static int _STDCALL module_context_hasTimedOut (void* obj, void* ptr)
{
  Q5Module* self = obj;
  EcStopWatch refWatch = ptr;

  
  return FALSE;
}

//-------------------------------------------------------------------------------------

static int _STDCALL module_thread_run (void* ptr)
{
  Q5Module* self = ptr;
  ENetEvent event;
  
  eclogger_fmt (LL_TRACE, properties.name, "server", "check for packets");

  while ((enet_host_service (self->server, &event, 200) > 0) && (self->done == TRUE))
  {
    eclogger_fmt (LL_TRACE, properties.name, "server", "event type %i", event.type);
    
    switch (event.type)
    {
      case ENET_EVENT_TYPE_NONE:
      {
        eclogger_fmt (LL_TRACE, properties.name, "server", "no conn received");
        
      }
      break;
      case ENET_EVENT_TYPE_CONNECT:
      {
        eclogger_fmt (LL_TRACE, properties.name, "server", "A new client connected from %x:%u", event.peer->address.host, event.peer -> address.port);
        
        /* Store any relevant client information here. */
        event.peer->data = "Client information";
      }
        break;
      case ENET_EVENT_TYPE_RECEIVE:
      {
        eclogger_fmt (LL_TRACE, properties.name, "server", "A packet of length %u containing %s was received from %s on channel %u",
                      event.packet -> dataLength,
                      event.packet -> data,
                      event.peer -> data,
                      event.channelID);
        /* Clean up the packet now that we're done using it. */
        enet_packet_destroy (event.packet);
        
      }
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
      {
        eclogger_fmt (LL_TRACE, properties.name, "server", "%s disconnected", event.peer -> data);
        /* Reset the peer's client information. */
        event.peer -> data = NULL;
      }
        break;
    }    
  }
  
  return !self->done;
}

//-------------------------------------------------------------------------------------

int module_start (Q5Module* self)
{
  ENetAddress address;

  if (enet_initialize () != 0)
  {
    eclogger_msg (LL_ERROR, properties.name, "server", "could not initialize enet");
    return FALSE;
  }
  
  enet_address_set_host (&address, self->host);
  address.port = self->port;
  
  self->server = enet_host_create (&address, 32, 2, 0, 0);      
  if (!self->server)
  {
    enet_deinitialize();

    eclogger_msg (LL_ERROR, properties.name, "server", "can't start enet server");
    return FALSE;
  }

  eclogger_fmt (LL_DEBUG, properties.name, "server", "listen '%s' on port '%u'", self->host, self->port);

  self->threads = eclist_new ();
  
  self->done = FALSE;
  
  int i;  
  for (i = 0; i < 1; i++)
  {
    EcThread thread = ecthread_new ();
    
    eclist_append(self->threads, thread);
    
    ecthread_start(thread, module_thread_run, self);
  }
  
  /*
  
  // create asyncronous server with 4 threads
  self->async = ecasync_create (4);
  
  static const EcAsyncContextCallbacks callbacks = {module_context_destroy, module_context_handle, module_context_run, module_context_hasTimedOut};
  
  EcAsyncContext context = ecasync_context_create (&callbacks, self);
  
  // add the context to all threads
  ecasync_addToAll (self->async, &context);
  */
   
  /*
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

    uint32_t gameEngineSrvNo = q5core_getModuleId (self->core, "GAEGN");

    if (gameEngineSrvNo == 0)
    {
      eclogger_fmt (LL_WARN, properties.name, "server", "no game engine available");
    }
    
    self->grealm = gs_realm_create (self->dispatcher, gameEngineSrvNo);
    
    return TRUE;
  }
  else 
  {
    return FALSE;
  }
   
   */
}

//-------------------------------------------------------------------------------------

int module_stop (Q5Module* self)
{
  EcListCursor cursor;
  
  self->done = TRUE;
  
  eclist_cursor (self->threads, &cursor);
  
  while (eclist_cnext (&cursor))
  {
    ecthread_join (cursor.value);
  }
  
  eclist_clear (self->threads);
  
  
  
  /*
  gs_realm_destroy(&(self->grealm));
*/  
  //ecasync_destroy (&(self->async));

  enet_host_destroy (self->server);

  enet_deinitialize();
   
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
