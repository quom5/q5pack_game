#include "entities.h"

#include "player.h"
#include "realm.h"

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

  eclogger_fmt (LL_TRACE, "GAME_S", "entities", "player added to server");      
}

//-------------------------------------------------------------------------------------

void gse_rmPlayer (GameServerEntities self, GameServerPlayer* ptrPlayer)
{
  ecmutex_lock (self->mutex);

  eclist_remove(self->players, *ptrPlayer);

  ecmutex_unlock (self->mutex);
  
  gs_player_destroy (ptrPlayer);

  eclogger_fmt (LL_TRACE, "GAME_S", "entities", "player removed from server");  
}

//-------------------------------------------------------------------------------------

GameServerRealm gse_addRealm (GameServerEntities self, const EcString name)
{
  GameServerRealm realm = gs_realm_create (self->host, self->gameEngineSrvNo, name);
  
  ecmutex_lock (self->mutex);

  eclist_append(self->realms, realm);
  
  ecmutex_unlock (self->mutex);

  eclogger_fmt (LL_TRACE, "GAME_S", "entities", "new realm '%s' added to server", name);      

  return realm;
}

//-------------------------------------------------------------------------------------

void gse_message (GameServerEntities self, ENetPeer* peer, GameServerFrame* frame, int channel)
{
  gs_player_message (peer->data, peer, frame, channel);
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

typedef struct 
{
  
  GameServerEntities entities;
  
  EcTable data;
  
  int status;
  
  EcListCursor cursor;
  
} GameServerEntitiesInfoCursor;

//----------------------------------------------------------------------------------------

int _STDCALL gse_cursor_fill (void* ptr, EcTable* table)
{
  GameServerEntitiesInfoCursor* self = ptr;
  
  eclogger_msg (LL_TRACE, "GAME_S", "cursor", "fill cursor");

  int cnt = 0;
  
  while (cnt < 10 && self->status)
  {
    self->status = eclist_cnext (&(self->cursor));
    
    if (self->status)
    {
      GameServerPlayer player = self->cursor.value;
      
      cnt++;
      
      gs_player_fillInfo (player, NULL, self->data, cnt);
    }    
  }
  
  *table = self->data;
  
  return cnt;
}

//----------------------------------------------------------------------------------------

int _STDCALL gse_cursor_destroy (void* ptr, EcTable table)
{
  //GameServerEntitiesInfoCursor* self = ptr;
  
  eclogger_msg (LL_TRACE, "GAME_S", "cursor", "destroy and clean cursor");

  //ectable_delete (&(self->data));
  
  return TRUE;
}

//-------------------------------------------------------------------------------------

EcUdc gse_cursor (GameServerEntities self, GameServerRealm realm)
{
  EcUdc udc = ecudc_create(ENTC_UDC_CURSOR, NULL);
  
  EcCursor cursor = ecudc_asCursor (udc);
    
  GameServerEntitiesInfoCursor* ci = ENTC_NEW (GameServerEntitiesInfoCursor);
  
  eclogger_msg (LL_TRACE, "GAME_S", "cursor", "create new cursor");

  ci->entities = self;
  ci->data = ectable_new (4, 10);
  ci->status = TRUE;
  
  ectable_set (ci->data, 0, 0, "id");
  ectable_set (ci->data, 0, 1, "name");
  ectable_set (ci->data, 0, 2, "realm");
  ectable_set (ci->data, 0, 3, "spawned");

  eclist_cursor (self->players, &(ci->cursor));
    
  eccursor_callbacks(cursor, ci, gse_cursor_fill, gse_cursor_destroy);
  
  return udc;
}

//-------------------------------------------------------------------------------------
