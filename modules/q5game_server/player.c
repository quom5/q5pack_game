#include "player.h"

#include "realm.h"

#include <utils/ecmessages.h>
#include <tools/ecjson.h>
#include <tools/ecbins.h>
#include <types/ecudc.h>

//-------------------------------------------------------------------------------------

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

void gs_player_leaveRealm (GameServerPlayer self)
{
  EcUdc node;

  if (self->realm == NULL)
  {
    return;
  }
  
  node = ecudc_create(ENTC_UDC_NODE, NULL);
  
  eclogger_fmt (LL_TRACE, "GAME_S", "player", "player '%s' left '%s'", self->name, gs_realm_name (self->realm));  
  
  // add user id to node
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  gs_realm_broadcast (self->realm, C_COMMAND_BROADCAST, C_MSG_LEAVE_REALM, node, TRUE);  
  // inform other players
  
  self->realm = NULL;
  self->spawned = FALSE;
  
  ecudc_destroy(&node);
}

//-------------------------------------------------------------------------------------------

void gs_player_disconnect (GameServerPlayer self, ENetPeer* peer)
{
  EcUdc node;

  if (self->realm)
  {
    gs_player_leaveRealm (self);
  }
  
  node = ecudc_create(ENTC_UDC_NODE, NULL);
  
  eclogger_fmt (LL_TRACE, "GAME_S", "recv", "player disconnected '%s'", self->name);  
  
  gs_frames_send (self->peer, C_COMMAND_PLAYER, C_MSG_DISCONNECT, node, TRUE);
  
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

void gs_player_authenticate (GameServerPlayer self, ENetPeer* peer, GameServerFrame* frame)
{
  static int pno = 0;
  
  pno++;
  
  self->name = ecstr_copy(ecudc_get_asString(frame->content, "Name", "[unknown]"));
  self->playerNo = pno;
  
  eclogger_fmt (LL_TRACE, "GAME_S", "recv", "player authenticated as '%s'", self->name);      
  
  ecudc_add_asUInt32(frame->content, "Id", self->playerNo);

  gs_frames_send (self->peer, C_COMMAND_PLAYER, C_MSG_AUTHENTICATE, frame->content, TRUE);
}

//-------------------------------------------------------------------------------------------

void gs_player_spawn (GameServerPlayer self)
{
  EcUdc node;

  if (self->realm == NULL)
  {
    return;
  }
  
  if (self->spawned)
  {
    return;
  }
  
  eclogger_fmt (LL_TRACE, "GAME_S", "recv", "player was spawned '%s' [%i|%i|%i]", self->name, self->posX, self->posY, self->posZ);
  
  node = ecudc_create(ENTC_UDC_NODE, NULL);
  
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  ecudc_add_asUInt32(node, "PosX", self->posX);
  ecudc_add_asUInt32(node, "PosY", self->posY);
  ecudc_add_asUInt32(node, "PosZ", self->posZ);
  
  gs_realm_broadcast (self->realm, C_COMMAND_BROADCAST, C_MSG_SPAWN, node, TRUE);
  
  ecudc_destroy(&node);  
  
  self->spawned = TRUE;
}

//-------------------------------------------------------------------------------------------

void gs_player_joinRealm (GameServerPlayer self, ENetPeer* peer, GameServerFrame* frame)
{
  if (self->realm)
  {
    gs_player_leaveRealm (self);
  }
  
  self->realm = gse_realm (self->entities, ecudc_get_asString(frame->content, "Realm", NULL));
  
  if (self->realm == NULL)
  {
    return;
  }
  
  eclogger_fmt (LL_TRACE, "GAME_S", "player", "player '%s' enters '%s'", self->name, gs_realm_name (self->realm));  
  
  ecudc_add_asUInt32(frame->content, "Id", self->playerNo);
  ecudc_add_asString(frame->content, "Name", self->name);

  gs_realm_broadcast (self->realm, C_COMMAND_BROADCAST, C_MSG_JOIN_REALM, frame->content, TRUE);
  // inform all
  
  gs_player_spawn (self);
}

//-------------------------------------------------------------------------------------------

void gs_player_unspawn (GameServerPlayer self)
{
  EcUdc node;

  if (self->realm == NULL)
  {
    return;
  }
  
  if (self->spawned == FALSE)
  {
    return;
  }
  
  eclogger_fmt (LL_TRACE, "GAME_S", "recv", "player was paused '%s' [%i|%i|%i]", self->name, self->posX, self->posY, self->posZ);
  
  node = ecudc_create(ENTC_UDC_NODE, NULL);
  
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  gs_realm_broadcast (self->realm, C_COMMAND_BROADCAST, C_MSG_UNSPAWN, node, TRUE);
  
  ecudc_destroy(&node);
  
  self->spawned = FALSE;
}

//-------------------------------------------------------------------------------------------

void gs_player_position (GameServerPlayer self, GameServerFrame* frame)
{
  if (self->realm == NULL)
  {
    return;
  }
  
  if (self->spawned == FALSE)
  {
    return;
  }
  
  self->posX = ecudc_get_asUInt32(frame->content, "PosX", self->posX);
  self->posY = ecudc_get_asUInt32(frame->content, "PosY", self->posY);
  self->posZ = ecudc_get_asUInt32(frame->content, "PosZ", self->posZ);
  
  eclogger_fmt (LL_TRACE, "GAME_S", "recv", "set player position '%s' [%i|%i|%i]", self->name, self->posX, self->posY, self->posZ);
  
  ecudc_add_asUInt32(frame->content, "Id", self->playerNo);
  
  gs_realm_broadcast (self->realm, C_COMMAND_BROADCAST, C_MSG_POSITION, frame->content, FALSE);
}

//-------------------------------------------------------------------------------------

int gs_player_basic_commands (GameServerPlayer player, ENetPeer* peer, GameServerFrame* frame)
{
  switch (frame->ch2)
  {
    case C_MSG_DISCONNECT:  // leave server / disconnect
    {
      gs_player_disconnect (player, peer);
    }
    break;
    case C_MSG_AUTHENTICATE:  // authenticate
    {
      gs_player_authenticate (player, peer, frame);
    }
    break;
    case C_MSG_JOIN_REALM:  // join realm
    {
      gs_player_joinRealm (player, peer, frame);
    }
    break;
    case C_MSG_LEAVE_REALM: // leave realm
    {
      gs_player_leaveRealm (player);
    }
    break;
    case C_MSG_SPAWN:       // spawn 
    {
      gs_player_spawn (player);
    }
    break;
    case C_MSG_UNSPAWN:     // unspawn
    {
      gs_player_unspawn (player);      
    }
    break;
    case C_MSG_POSITION:    // new player position
    {
      gs_player_position (player, frame);              
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

void gs_player_request_commands (GameServerPlayer player, ENetPeer* peer, GameServerFrame* frame)
{
  switch (frame->ch2)
  {
    case C_MSG_LISTREALMS:  // request list of all realms 
    {
      gs_player_reqPlayers (player, peer);
    }
    break;
    case C_MSG_PLAYERS:  // request list of all players within a realm 
    {
      gs_player_reqPlayers (player, peer);
    }
    break;
  }
}

//-------------------------------------------------------------------------------------------

void gs_player_message (GameServerPlayer self, ENetPeer* peer, GameServerFrame* frame, int channel)
{
  if (channel == 0)  // game channel
  {
    switch (frame->ch1)
    {
      case C_COMMAND_PLAYER:  // basic commands
      {
        gs_player_basic_commands (self, peer, frame);
      }
      case C_COMMAND_REQUEST:  // request commands
      {
        gs_player_request_commands (self, peer, frame);
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

    // send new player, simulate broadcast message
    gs_frames_send (peer, C_COMMAND_BROADCAST, C_MSG_JOIN_REALM, node, TRUE);
    
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
    ectable_set (table, row, 2, (char*)gs_realm_name (self->realm));    
  }
  else
  {
    ectable_set (table, row, 2, "");    
  }

  ectable_set (table, row, 3, self->spawned ? "true" : "false");    
}

//-------------------------------------------------------------------------------------------
