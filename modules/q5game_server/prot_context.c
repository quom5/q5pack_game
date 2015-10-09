#include "prot_context.h"

#include <utils/ecmessages.h>

#include "player.h"

//-------------------------------------------------------------------------------------

int prot_context_basic_commands (GameServerPlayer self, EcAsyncUdpContext ctx, EcBuffer buf, ulong_t len)
{
  switch (buf->buffer[1])
  {
    case '1':  // player disconnect
    {
      return FALSE;  // this will destroy the context
    }
    case '2':  // player connected
    {
      gs_player_connect (self, buf->buffer + 2, len - 2);
    }
    break;
    case '3':  // new player position
    {
      gs_player_position (self, buf->buffer + 2, len - 2);              
    }
    break;
  }
  
  return TRUE;
}

//-------------------------------------------------------------------------------------

int prot_context_request_commands (GameServerPlayer self, EcDatagram dg, EcBuffer buf, ulong_t len)
{
  switch (buf->buffer[1])
  {
    case '2':  // request list of all players 
    {
      gs_player_reqPlayers (self, dg);
    }
  }
  
  return TRUE;
}

//-------------------------------------------------------------------------------------

int _STDCALL prot_context_onRecv (void* ptr, EcAsyncUdpContext ctx, EcDatagram dg, ulong_t len)
{  
  GameServerPlayer player = ptr;
  
  if (len < 2)
  {
    eclogger_fmt (LL_ERROR, "GCTX", "recv", "datagram too short");
    return FALSE;
  }

  EcBuffer buf = ecdatagram_buffer(dg);
  
  eclogger_fmt (LL_TRACE, "GCTX", "recv", "message: '%s'", buf->buffer);

  switch (buf->buffer[0])
  {
    case '0':  // basic commands
    {
      return prot_context_basic_commands (player, ctx, buf, len);
    }
    case '2':  // request commands
    {
      return prot_context_request_commands (player, dg, buf, len);
    }
  }

  eclogger_fmt (LL_ERROR, "GCTX", "recv", "unknown command '%s'", buf->buffer);
  return FALSE;
}

//-------------------------------------------------------------------------------------

void _STDCALL prot_context_onDestroy (void** ptr)
{
  GameServerPlayer player = *ptr;
  
  // broadcast to other player this one is now gone away
  gs_player_disconnect (player, NULL);
  
  gs_player_destroy (&player);
  
  eclogger_fmt (LL_TRACE, "GCTX", "create", "client context destroyed");
}

//-------------------------------------------------------------------------------------------

EcAsyncUdpContext prot_context_create (GameServerPlayer player)
{
  eclogger_fmt (LL_TRACE, "GCTX", "create", "new client context created");

  return ecasync_udpcontext_create (60000, prot_context_onRecv, prot_context_onDestroy, player);
}

//-------------------------------------------------------------------------------------------

