#include "player.h"

#include <utils/ecmessages.h>
#include <tools/ecjson.h>
#include <types/ecudc.h>

struct GameServerPlayer_s
{
  
  GameServerRealm realm;
  
  EcString playerName;
  
  int playerNo;
  
  int posX;
  
  int posY;
  
  int posZ;
  
};

//-------------------------------------------------------------------------------------

GameServerPlayer gs_player_create (GameServerRealm realm)
{
  GameServerPlayer self = ENTC_NEW (struct GameServerPlayer_s);
  
  self->realm = realm;
  self->playerName = NULL;
  
  self->posX = 0;
  self->posY = 0;
  self->posZ = 0;
  
  return self;
}

//-------------------------------------------------------------------------------------------

void gs_player_ping (GameServerPlayer self)
{
  eclogger_fmt (LL_TRACE, "GCTX", "recv", "ping [%s]", self->playerName);
}

//-------------------------------------------------------------------------------------------

void gs_player_connect (GameServerPlayer self, EcAsyncUdpContext ctx, const unsigned char* buffer, ulong_t len)
{
  static int pno = 0;
  
  pno++;
  
  EcUdc node = ecjson_read((const char*)buffer, NULL);
  
  self->playerName = ecstr_copy(ecudc_get_asString(node, "Name", "[unknown]"));
  self->playerNo = pno;
  
  eclogger_fmt (LL_TRACE, "GCTX", "recv", "player connected '%s'", self->playerName);      

  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  ecudc_add_asUInt32(node, "PosX", self->posX);
  ecudc_add_asUInt32(node, "PosY", self->posY);
  ecudc_add_asUInt32(node, "PosZ", self->posZ);
  
  gs_realm_broadcast (self->realm, ctx, "12", node);  // send new player

  ecudc_destroy(&node);  
}

//-------------------------------------------------------------------------------------------

void gs_player_disconnect (GameServerPlayer self, EcAsyncUdpContext ctx)
{
  EcUdc node = ecudc_create(ENTC_UDC_NODE, NULL);
  
  eclogger_fmt (LL_TRACE, "GCTX", "recv", "player disconnected '%s'", self->playerName);  
  
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  gs_realm_broadcast (self->realm, ctx, "11", node);
  
  ecudc_destroy(&node);  
}

//-------------------------------------------------------------------------------------------

void gs_player_position (GameServerPlayer self, EcAsyncUdpContext ctx, const unsigned char* buffer, ulong_t len)
{
  EcUdc node = ecjson_read((const char*)buffer, NULL);

  self->posX = ecudc_get_asUInt32(node, "PosX", self->posX);
  self->posY = ecudc_get_asUInt32(node, "PosY", self->posY);
  self->posZ = ecudc_get_asUInt32(node, "PosZ", self->posZ);
  
  eclogger_fmt (LL_TRACE, "GCTX", "recv", "set player position '%s' [%i|%i|%i]", self->playerName, self->posX, self->posY, self->posZ);
  
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  gs_realm_broadcast (self->realm, ctx, "13", node);
  
  ecudc_destroy(&node);
}

//-------------------------------------------------------------------------------------------

void gs_player_reqPlayers (GameServerPlayer self, EcDatagram dg)
{
  eclogger_fmt (LL_TRACE, "GCTX", "recv", "send all players");

  gs_realm_sendPlayers (self->realm, dg, self);
}

//-------------------------------------------------------------------------------------

void gs_player_send (EcDatagram dg, const EcString command, EcUdc node)
{
  EcString jsonText = ecjson_write(node);
  
  EcString commandText = ecstr_cat2(command, jsonText);
  
  EcBuffer buf = ecbuf_create_str (&commandText);
  
  ecdatagram_writeBuf(dg, buf, buf->size);

  ecbuf_destroy(&buf);
  
  ecstr_delete(&jsonText);  
}

//-------------------------------------------------------------------------------------------

void gs_player_sendInfo (GameServerPlayer self, EcDatagram dg)
{
  EcUdc node = ecudc_create(ENTC_UDC_NODE, NULL);

  ecudc_add_asString(node, "Name", self->playerName);
  ecudc_add_asUInt32(node, "Id", self->playerNo);
  
  ecudc_add_asUInt32(node, "PosX", self->posX);
  ecudc_add_asUInt32(node, "PosY", self->posY);
  ecudc_add_asUInt32(node, "PosZ", self->posZ);
  
  gs_player_send (dg, "12", node);  // send new player
  
  ecudc_destroy(&node);  
}

//-------------------------------------------------------------------------------------------

