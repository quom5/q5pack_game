#include "realm.h"

#include "player.h"

#include <utils/ecmessages.h>
#include <tools/ecjson.h>
#include <types/ecudc.h>
#include <tools/ecbins.h>
#include <system/ecmutex.h>

#define GAME_ENGINE_PLAYER_NEW 10001
#define GAME_ENGINE_PLAYER_DEL 10002
#define GAME_ENGINE_PLAYER_POS 10003

struct GameServerRealm_s
{
  
  ENetHost* host;
  
  EcMutex mutex;
  
  uint32_t gameEngineSrvNo;
  
  EcString name;
  
};

//-------------------------------------------------------------------------------------

GameServerRealm gs_realm_create (ENetHost* enetHost, uint32_t gameEngineSrvNo, const EcString name)
{
  GameServerRealm self = ENTC_NEW (struct GameServerRealm_s);
  
  self->host = enetHost;
  
  self->mutex = ecmutex_new ();
  
  self->gameEngineSrvNo = gameEngineSrvNo;
  
  self->name = ecstr_copy(name);
  
  return self;
}

//-------------------------------------------------------------------------------------

void gs_realm_destroy (GameServerRealm* pself)
{
  GameServerRealm self = *pself;
  
  ecmutex_delete(&(self->mutex));
  
  ecstr_delete(&(self->name));
  
  ENTC_DEL (pself, struct GameServerRealm_s);
}

//-------------------------------------------------------------------------------------

void gs_realm_broadcast (GameServerRealm self, ubyte_t ch1, ubyte_t ch2, EcUdc node, int reliable)
{
  gs_frames_broadcast (self->host, ch1, ch2, node, reliable);
}

//-------------------------------------------------------------------------------------

const EcString gs_realm_name (GameServerRealm self)
{
  return self->name;
}

//-------------------------------------------------------------------------------------

