#include "player.h"

#include "realm.h"

#include <utils/ecmessages.h>
#include <tools/ecjson.h>
#include <tools/ecbins.h>
#include <types/ecudc.h>

struct GameServerPlayer_s
{
  
  GameServerEntities entities;
  
  ENetPeer* peer;
  
  GameServerRealm realm;
  
  EcString name;
  
  int playerNo;
  
  int posX;
  
  int posY;
  
  int posZ;
  
  int spawned;
  
};

//-------------------------------------------------------------------------------------

GameServerPlayer gs_player_create (GameServerEntities entities, ENetPeer* peer)
{
  GameServerPlayer self = ENTC_NEW (struct GameServerPlayer_s);
  
  self->entities = entities;
  self->peer = peer;
  
  // link back
  peer->data = self;
  
  self->realm = NULL;
  self->name = NULL;
  
  self->posX = 0;
  self->posY = 0;
  self->posZ = 0;
  
  self->spawned = FALSE;
  
  return self;
}

//-------------------------------------------------------------------------------------

void gs_player_send (ENetPeer* peer, const EcString command, EcUdc node, int reliable)
{
  EcBuffer bins = ecbins_write (node, command);
  
  ENetPacket * packet = enet_packet_create (bins->buffer, bins->size, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
  
  enet_peer_send (peer, 0, packet);
  
  ecbuf_destroy(&bins);
}

//-------------------------------------------------------------------------------------------

void gs_player_leaveRealm (GameServerPlayer self)
{
  if (self->realm == NULL)
  {
    return;
  }
  
  EcUdc node = ecudc_create(ENTC_UDC_NODE, NULL);
  
  eclogger_fmt (LL_TRACE, "GAME_S", "player", "player '%s' left '%s'", self->name, gs_realm_name (self->realm));  
  
  // add user id to node
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  gs_realm_broadcast (self->realm, "14", node, TRUE);  // inform other players
  
  self->realm = NULL;
  self->spawned = FALSE;
}

//-------------------------------------------------------------------------------------------

void gs_player_disconnect (GameServerPlayer self, ENetPeer* peer)
{
  if (self->realm)
  {
    gs_player_leaveRealm (self);
  }
  
  EcUdc node = ecudc_create(ENTC_UDC_NODE, NULL);
  
  eclogger_fmt (LL_TRACE, "GAME_S", "recv", "player disconnected '%s'", self->name);  
  
  gs_player_send (self->peer, "01", node, TRUE);
  
  ecudc_destroy(&node);  
}

//-------------------------------------------------------------------------------------------

void gs_player_destroy (GameServerPlayer* pself)
{
  GameServerPlayer self = *pself;
  
  gs_player_disconnect (self, NULL);
  
  ecstr_delete(&(self->name));
  
  ENTC_DEL (pself, struct GameServerPlayer_s);
}

//-------------------------------------------------------------------------------------------

void gs_player_authenticate (GameServerPlayer self, ENetPeer* peer, const unsigned char* buffer, ulong_t len)
{
  static int pno = 0;
  
  pno++;
  
  EcBuffer_s posbuf = { (unsigned char*)buffer, len };

  EcUdc node = ecbins_read(&posbuf, NULL);
  
  self->name = ecstr_copy(ecudc_get_asString(node, "Name", "[unknown]"));
  self->playerNo = pno;
  
  eclogger_fmt (LL_TRACE, "GAME_S", "recv", "player authenticated as '%s'", self->name);      
  
  ecudc_add_asUInt32(node, "Id", self->playerNo);

  gs_player_send (self->peer, "02", node, TRUE);

  ecudc_destroy(&node);  
}

//-------------------------------------------------------------------------------------------

void gs_player_spawn (GameServerPlayer self)
{
  if (self->realm == NULL)
  {
    return;
  }
  
  if (self->spawned)
  {
    return;
  }
  
  eclogger_fmt (LL_TRACE, "GAME_S", "recv", "player was spawned '%s' [%i|%i|%i]", self->name, self->posX, self->posY, self->posZ);
  
  EcUdc node = ecudc_create(ENTC_UDC_NODE, NULL);
  
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  ecudc_add_asUInt32(node, "PosX", self->posX);
  ecudc_add_asUInt32(node, "PosY", self->posY);
  ecudc_add_asUInt32(node, "PosZ", self->posZ);
  
  gs_realm_broadcast (self->realm, "15", node, TRUE);
  
  ecudc_destroy(&node);  
  
  self->spawned = TRUE;
}

//-------------------------------------------------------------------------------------------

void gs_player_joinRealm (GameServerPlayer self, ENetPeer* peer, const unsigned char* buffer, ulong_t len)
{
  if (self->realm)
  {
    gs_player_leaveRealm (self);
  }
  
  EcBuffer_s posbuf = { (unsigned char*)buffer, len };

  EcUdc node = ecbins_read(&posbuf, NULL);

  self->realm = gse_realm (self->entities, ecudc_get_asString(node, "Realm", NULL));
  
  if (self->realm == NULL)
  {
    return;
  }
  
  eclogger_fmt (LL_TRACE, "GAME_S", "player", "player '%s' enters '%s'", self->name, gs_realm_name (self->realm));  
  
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  ecudc_add_asString(node, "Name", self->name);

  gs_realm_broadcast (self->realm, "13", node, TRUE);  // inform all
  
  ecudc_destroy(&node); 
  
  gs_player_spawn (self);
}

//-------------------------------------------------------------------------------------------

void gs_player_unspawn (GameServerPlayer self)
{
  if (self->realm == NULL)
  {
    return;
  }
  
  if (self->spawned == FALSE)
  {
    return;
  }
  
  eclogger_fmt (LL_TRACE, "GAME_S", "recv", "player was paused '%s' [%i|%i|%i]", self->name, self->posX, self->posY, self->posZ);
  
  EcUdc node = ecudc_create(ENTC_UDC_NODE, NULL);
  
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  gs_realm_broadcast (self->realm, "16", node, TRUE);
  
  ecudc_destroy(&node);
  
  self->spawned = FALSE;
}

//-------------------------------------------------------------------------------------------

void gs_player_position (GameServerPlayer self, const unsigned char* buffer, ulong_t len)
{
  if (self->realm == NULL)
  {
    return;
  }
  
  if (self->spawned == FALSE)
  {
    return;
  }
  
  EcBuffer_s posbuf = { (unsigned char*)buffer, len };
  
  EcUdc node = ecbins_read(&posbuf, NULL);
  
  self->posX = ecudc_get_asUInt32(node, "PosX", self->posX);
  self->posY = ecudc_get_asUInt32(node, "PosY", self->posY);
  self->posZ = ecudc_get_asUInt32(node, "PosZ", self->posZ);
  
  eclogger_fmt (LL_TRACE, "GAME_S", "recv", "set player position '%s' [%i|%i|%i]", self->name, self->posX, self->posY, self->posZ);
  
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  gs_realm_broadcast (self->realm, "17", node, FALSE);
  
  ecudc_destroy(&node);
}

//-------------------------------------------------------------------------------------

int gs_player_basic_commands (GameServerPlayer player, ENetPeer* peer, EcBuffer buf)
{
  switch (buf->buffer[1])
  {
    case '1':  // leave server / disconnect
    {
      gs_player_disconnect (player, peer);
    }
    break;
    case '2':  // authenticate
    {
      gs_player_authenticate (player, peer, buf->buffer + 2, buf->size - 2);
    }
    break;
    case '3':  // join realm
    {
      gs_player_joinRealm (player, peer, buf->buffer + 2, buf->size - 2);
    }
    break;
    case '4':  // leave realm
    {
      gs_player_leaveRealm (player);
    }
    break;
    case '5':  // spawn 
    {
      gs_player_spawn (player);
    }
    break;
    case '6':  // unspawn
    {
      gs_player_unspawn (player);      
    }
    break;
    case '7':  // new player position
    {
      gs_player_position (player, buf->buffer + 2, buf->size - 2);              
    }
    break;
  }
  
  return TRUE;
}

//-------------------------------------------------------------------------------------------

void gs_player_reqPlayers (GameServerPlayer self, ENetPeer* peer)
{
  if (self->realm == NULL)
  {
    return;
  }
  
  eclogger_fmt (LL_TRACE, "GAME_S", "request", "send all players");
  
  gse_sendPlayers (self->entities, self->realm, peer);
}

//-------------------------------------------------------------------------------------------

void gs_player_request_commands (GameServerPlayer player, ENetPeer* peer, EcBuffer buf)
{
  switch (buf->buffer[1])
  {
    case '1':  // request list of all realms 
    {
      gs_player_reqPlayers (player, peer);
    }
    break;
    case '2':  // request list of all players withina realm 
    {
      gs_player_reqPlayers (player, peer);
    }
    break;
  }
}

//-------------------------------------------------------------------------------------------

void gs_player_message (GameServerPlayer self, ENetPeer* peer, EcBuffer buf, int channel)
{
  if (channel == 0)  // game channel
  {
    switch (buf->buffer[0])
    {
      case '0':  // basic commands
      {
        gs_player_basic_commands (self, peer, buf);
      }
      case '2':  // request commands
      {
        gs_player_request_commands (self, peer, buf);
      }
    }
  }
}

//-------------------------------------------------------------------------------------------

void gs_player_sendInfo (GameServerPlayer self, GameServerRealm realm, ENetPeer* peer)
{
  if (self->realm == realm)
  {
    EcUdc node = ecudc_create(ENTC_UDC_NODE, NULL);
    
    ecudc_add_asString(node, "Name", self->name);
    ecudc_add_asUInt32(node, "Id", self->playerNo);
    ecudc_add_asUInt32(node, "Spawned", self->spawned ? 1 : 0);
    
    ecudc_add_asUInt32(node, "PosX", self->posX);
    ecudc_add_asUInt32(node, "PosY", self->posY);
    ecudc_add_asUInt32(node, "PosZ", self->posZ);
    
    eclogger_fmt (LL_TRACE, "GAME_S", "request", "send player '%s'", self->name);
    
    gs_player_send (peer, "13", node, TRUE);  // send new player
    
    ecudc_destroy(&node);      
  }
}

//-------------------------------------------------------------------------------------------

void gs_player_fillInfo (GameServerPlayer self, GameServerRealm realm, EcTable table, int row)
{
  ectable_set (table, row, 0, "0");
  ectable_set (table, row, 1, self->name);
  
  if (self->realm)
  {
    ectable_set (table, row, 2, gs_realm_name (self->realm));    
  }
  else
  {
    ectable_set (table, row, 2, "");    
  }

  ectable_set (table, row, 3, self->spawned ? "true" : "false");    
}

//-------------------------------------------------------------------------------------------
