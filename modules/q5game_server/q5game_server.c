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
#include <types/ecbuffer.h>

// project includes
#include "prot_context.h"
#include "entities.h"

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
  
  GameServerEntities entities;
  
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

//-------------------------------------------------------------------------------------

static int _STDCALL module_thread_run (void* ptr)
{
  Q5Module* self = ptr;
  ENetEvent event;
  
  while (enet_host_service (self->server, &event, 100) > 0)
  {
    switch (event.type)
    {
      case ENET_EVENT_TYPE_NONE: break;
      case ENET_EVENT_TYPE_CONNECT:
      {
        // create a new player for this server
        gse_addPlayer (self->entities, event.peer);
      }
      break;
      case ENET_EVENT_TYPE_RECEIVE:
      {
        EcBuffer_s buf = { event.packet->data, event.packet->dataLength };
        
        gse_message (self->entities, event.peer, &buf, event.channelID);
        
        enet_packet_destroy (event.packet);
      }
      break;
      case ENET_EVENT_TYPE_DISCONNECT:
      {
        GameServerPlayer player = event.peer->data;
        
        gse_rmPlayer(self->entities, &player);
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

  uint32_t gameEngineSrvNo = q5core_getModuleId (self->core, "GAEGN");
  
  if (gameEngineSrvNo == 0)
  {
    eclogger_fmt (LL_WARN, properties.name, "server", "no game engine available");
  }
  
  self->entities = gse_create (self->server, gameEngineSrvNo, "Lobo's geiler server");
  
  gse_addRealm (self->entities, "the shire");

  return TRUE;
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
  
  gse_destroy(&(self->entities));

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
  
  self->entities = NULL;
  
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
