#include "entities.h"

#include "player.h"
#include "prot_context.h"

#include <utils/ecmessages.h>
#include <tools/ecjson.h>
#include <types/ecudc.h>
#include <system/ecmutex.h>

#define GAME_ENGINE_PLAYER_NEW 10001
#define GAME_ENGINE_PLAYER_DEL 10002
#define GAME_ENGINE_PLAYER_POS 10003

struct GameServerEntities_s
{
  
  ENetHost* host;
  
  EcList players;
  
  EcList realms;
  
  EcMutex mutex;
  
  uint32_t gameEngineSrvNo;
  
  EcString name;
  
};

//-------------------------------------------------------------------------------------

GameServerEntities gse_create (ENetHost* enetHost, uint32_t gameEngineSrvNo, const EcString name)
{
  GameServerEntities self = ENTC_NEW (struct GameServerEntities_s);
  
  self->host = enetHost;
  
  self->players = eclist_new ();
  self->realms = eclist_new ();
  
  self->mutex = ecmutex_new ();
  
  self->gameEngineSrvNo = gameEngineSrvNo;
  
  self->name = ecstr_copy(name);
  
  return self;
}

//-------------------------------------------------------------------------------------

void gse_destroy (GameServerEntities* pself)
{
  EcListCursor cursor;
  
  GameServerEntities self = *pself;
  
  eclist_cursor(self->players, &cursor);
  while (eclist_cnext(&cursor))
  {
    gs_player_destroy ((GameServerPlayer*)&(cursor.value));
  }
  
  eclist_delete(&(self->players));

  eclist_cursor(self->realms, &cursor);
  while (eclist_cnext(&cursor))
  {
    gs_realm_destroy ((GameServerRealm*)&(cursor.value));
  }
  
  eclist_delete(&(self->realms));
  
  ecmutex_delete(&(self->mutex));
  
  ecstr_delete(&(self->name));
  
  ENTC_DEL (pself, struct GameServerEntities_s);
}

//-------------------------------------------------------------------------------------

void gse_addPlayer (GameServerEntities self, ENetPeer* peer)
{
  GameServerPlayer player = gs_player_create (self, peer);
    
  ecmutex_lock (self->mutex);
  
  eclist_append (self->players, player);
  
  ecmutex_unlock (self->mutex);

  eclogger_fmt (LL_TRACE, "GCTX", "entities", "player added to server");      
}

//-------------------------------------------------------------------------------------

void gse_rmPlayer (GameServerEntities self, GameServerPlayer* ptrPlayer)
{
  ecmutex_lock (self->mutex);

  eclist_remove(self->players, *ptrPlayer);

  ecmutex_unlock (self->mutex);
  
  gs_player_destroy (ptrPlayer);

  eclogger_fmt (LL_TRACE, "GCTX", "entities", "player removed from server");  
}

//-------------------------------------------------------------------------------------

GameServerRealm gse_addRealm (GameServerEntities self, const EcString name)
{
  GameServerRealm realm = gs_realm_create (self->host, self->gameEngineSrvNo, name);
  
  ecmutex_lock (self->mutex);

  eclist_append(self->realms, realm);
  
  ecmutex_unlock (self->mutex);

  eclogger_fmt (LL_TRACE, "GCTX", "entities", "new realm '%s' added to server", name);      

  return realm;
}

//-------------------------------------------------------------------------------------

void gse_message (GameServerEntities self, ENetPeer* peer, EcBuffer buf, int channel)
{
  gs_player_message (peer->data, peer, buf, channel);
}

//-------------------------------------------------------------------------------------

GameServerRealm gse_realm (GameServerEntities self, const EcString name)
{
  EcListCursor cursor;

  GameServerRealm realm = NULL;
  
  ecmutex_lock (self->mutex);

  eclist_cursor(self->realms, &cursor);
  while (eclist_cnext(&cursor))
  {
    if (ecstr_equal(name, gs_realm_name(cursor.value)))
    {
      realm = cursor.value;
      break;
    }
  }  
  
  ecmutex_unlock (self->mutex);

  return realm;
}

//-------------------------------------------------------------------------------------

void gse_sendPlayers (GameServerEntities self, GameServerRealm realm, ENetPeer* peer)
{
  EcListCursor cursor;
  
  ecmutex_lock (self->mutex);
  
  eclist_cursor(self->players, &cursor);
  
  while (eclist_cnext(&cursor))
  {
    GameServerPlayer player = cursor.value;
    
    if (player != peer->data)
    {
      gs_player_sendInfo (player, realm, peer);
    }
  }
  
  ecmutex_unlock (self->mutex);
}

//-------------------------------------------------------------------------------------
