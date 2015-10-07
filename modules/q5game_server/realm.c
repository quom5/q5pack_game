#include "realm.h"

#include "player.h"
#include "prot_context.h"

#include <utils/ecmessages.h>
#include <tools/ecjson.h>
#include <types/ecudc.h>

struct GameServerRealm_s
{
  
  EcAsynUdpDispatcher dispatcher;
  
  EcList players;
  
};

//-------------------------------------------------------------------------------------

GameServerRealm gs_realm_create (EcAsynUdpDispatcher dispatcher)
{
  GameServerRealm self = ENTC_NEW (struct GameServerRealm_s);
  
  self->dispatcher = dispatcher;
  self->players = eclist_new ();
  
  return self;
}

//-------------------------------------------------------------------------------------

void gs_realm_destroy (GameServerRealm* pself)
{
  GameServerRealm self = *pself;
  
  eclist_delete(&(self->players));
  
  ENTC_DEL (pself, struct GameServerRealm_s);
}

//-------------------------------------------------------------------------------------

EcAsyncUdpContext gs_realm_newContext (GameServerRealm self)
{
  GameServerPlayer player = gs_player_create (self);
    
  eclist_append (self->players, player);
  
  return prot_context_create (player);
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
  
  eclist_cursor(self->players, &cursor);
  
  while (eclist_cnext(&cursor))
  {
    GameServerPlayer player = cursor.value;
    
    if (player != extPlayer)
    {
      gs_player_sendInfo (player, dg);
    }
  }
}

//-------------------------------------------------------------------------------------
