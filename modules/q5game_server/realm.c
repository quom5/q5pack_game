#include "realm.h"

#include "player.h"
#include "prot_context.h"

#include <utils/ecmessages.h>
#include <tools/ecjson.h>
#include <types/ecudc.h>
#include <system/ecmutex.h>

struct GameServerRealm_s
{
  
  EcAsynUdpDispatcher dispatcher;
  
  EcList players;
  
  EcMutex mutex;
  
};

//-------------------------------------------------------------------------------------

GameServerRealm gs_realm_create (EcAsynUdpDispatcher dispatcher)
{
  GameServerRealm self = ENTC_NEW (struct GameServerRealm_s);
  
  self->dispatcher = dispatcher;
  self->players = eclist_new ();
  
  self->mutex = ecmutex_new ();
  
  return self;
}

//-------------------------------------------------------------------------------------

void gs_realm_destroy (GameServerRealm* pself)
{
  GameServerRealm self = *pself;
  
  eclist_delete(&(self->players));
  
  ecmutex_delete(&(self->mutex));
  
  ENTC_DEL (pself, struct GameServerRealm_s);
}

//-------------------------------------------------------------------------------------

EcAsyncUdpContext gs_realm_newContext (GameServerRealm self)
{
  GameServerPlayer player = gs_player_create (self);
    
  ecmutex_lock (self->mutex);
  
  eclist_append (self->players, player);
  
  ecmutex_unlock (self->mutex);

  return prot_context_create (player);
}

//-------------------------------------------------------------------------------------

void gs_realm_removePlayer (GameServerRealm self, GameServerPlayer player)
{
  ecmutex_lock (self->mutex);

  eclist_remove(self->players, player);

  ecmutex_unlock (self->mutex);
}

//-------------------------------------------------------------------------------------

void gs_realm_broadcast (GameServerRealm self, EcAsyncUdpContext ctx, const EcString command, EcUdc node)
{
  EcString jsonText = ecjson_write(node);
  
  EcString commandText = ecstr_cat2(command, jsonText);
  
  EcBuffer buf = ecbuf_create_str (&commandText);
  
  ecasync_udpdisp_broadcast (self->dispatcher, buf, buf->size, ctx);
  
  ecbuf_destroy(&buf);
  
  ecstr_delete(&jsonText);  
}

//-------------------------------------------------------------------------------------

void gs_realm_sendPlayers (GameServerRealm self, EcDatagram dg, GameServerPlayer extPlayer)
{
  EcListCursor cursor;
  
  ecmutex_lock (self->mutex);

  eclist_cursor(self->players, &cursor);
  
  while (eclist_cnext(&cursor))
  {
    GameServerPlayer player = cursor.value;
    
    if (player != extPlayer)
    {
      gs_player_sendInfo (player, dg);
    }
  }
  
  ecmutex_unlock (self->mutex);
}

//-------------------------------------------------------------------------------------
