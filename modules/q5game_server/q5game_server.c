#include <system/macros.h>

// quom includes
#include <core/q5.h>
#include <core/q5.h>
#include <core/q5modules.h>
#include <core/q5core.h>
#include <core/q5auth.h>

// entc includes
#include <tools/ecasyncvc.h>
#include <utils/ecmessages.h>
#include <system/ectime.h>
#include <system/ecthread.h>
#include <types/ecbuffer.h>

// project includes
#include "entities.h"
#include "frames.h"

// enet includes
#include <enet/enet.h>

//================================================================================================

static Q5ModuleProperties properties = {"GAME_S", "Game server", 10001, 0x4001};

typedef struct {
  
  Q5ModuleInstance* instance;

  // reference
  Q5Core core;
  
  uint_t port;
  
  EcString host;
    
  GameServerEntities entities;
  
  ENetHost* server;
  
  EcList threads;
  
  int done;
  
  EcTable data;
  
} Q5Module;

typedef struct {
  
  uint_t api;
  
  uint_t data;
  
  EcMessageData msdata;
  
} Q5ModuleServiceNums;

// add exported methods of this shared library
#include <core/q5module.inc>

//===========================================================================================================

int _STDCALL module_getPath (void* ptr, EcString* path, EcUserInfo userInfo, EcUdc* userData, EcUdc content, EcUdc object)
{  
  ecstr_replace (path, "");

  return ENTC_RESCODE_OK;
}

//-------------------------------------------------------------------------------------

int _STDCALL module_get (void* ptr, EcMessageData* dOut, EcUserInfo userInfo, EcUdc content, const EcString object)
{
  Q5Module* self = ptr;

  dOut->content = gse_cursor (self->entities, NULL);
  
  dOut->type = Q5_MSGTYPE_TABLE; dOut->rev = 1;
  dOut->ref = 0;
  
  return ENTC_RESCODE_OK;
}

//-------------------------------------------------------------------------------------

int _STDCALL module_callback_get (void* ptr, EcMessageData* dIn, EcMessageData* dOut)
{
  if (isAssigned (dIn))
  {
    switch (dIn->type)
    {
      case Q5_MSGTYPE_STATE_INIT:
      {
        return auth_checkF0 (ptr, dIn, dOut, module_getPath, ENTC_UDC_TABLEINFO);
      }
      case Q5_MSGTYPE_STATE_PROCESS:
      {
        return auth_processF1 (ptr, dIn, dOut, module_get, FALSE);
      }
    }
  }
  return ENTC_RESCODE_IGNORE;
}

//===========================================================================================================

void module_frames (Q5Module* self, ENetEvent* event, EcBuffer buf)
{
  GameServerFrameCursor cursor;
  GameServerFrame frame;
  
  gs_frames_init (buf, &cursor);
  
  while (gs_frames_next (&cursor, &frame))
  {
    gse_message (self->entities, event->peer, &frame, event->channelID);
  }
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
        
        module_frames (self, &event, &buf);
        
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

//===========================================================================================================

void module_config (Q5Module* self, const EcUdc item)
{ 
}

//-------------------------------------------------------------------------------------

int module_start (Q5Module* self)
{
  int i;  
  uint32_t gameEngineSrvNo;
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
  
  for (i = 0; i < 1; i++)
  {
    EcThread thread = ecthread_new ();
    
    eclist_append(self->threads, thread);
    
    ecthread_start(thread, module_thread_run, self);
  }

  gameEngineSrvNo = q5core_getModuleId (self->core, "GAME_E");
  
  if (gameEngineSrvNo == 0)
  {
    eclogger_fmt (LL_WARN, properties.name, "server", "no game engine available");
  }
  
  self->entities = gse_create (self->server, gameEngineSrvNo, "Lobo's geiler server");
  
  gse_addRealm (self->entities, "the shire");
  
  ecmessages_add (self->instance->msgid, Q5_DATA_GET, module_callback_get, self);

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
  
  while (eclist_cnext (&cursor))
  {
    ecthread_delete((EcThread*)(&cursor.value));
  }
  
  eclist_delete (&(self->threads));
  
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
