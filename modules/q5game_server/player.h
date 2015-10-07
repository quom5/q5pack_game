#ifndef GAMESERVER_PLAYER_H
#define GAMESERVER_PLAYER_H 1

#include "realm.h"

#include <system/macros.h>
#include <tools/ecasyncvc.h>

__CPP_EXTERN______________________________________________________________________________START

__LIB_EXPORT GameServerPlayer gs_player_create (GameServerRealm);

__LIB_EXPORT void gs_player_destroy (GameServerPlayer*);

__LIB_EXPORT void gs_player_ping (GameServerPlayer);

__LIB_EXPORT void gs_player_connect (GameServerPlayer, EcAsyncUdpContext ctx, const unsigned char* buffer, ulong_t len);

__LIB_EXPORT void gs_player_disconnect (GameServerPlayer, EcAsyncUdpContext ctx);

__LIB_EXPORT void gs_player_position (GameServerPlayer, EcAsyncUdpContext ctx, const unsigned char* buffer, ulong_t len);

__LIB_EXPORT void gs_player_reqPlayers (GameServerPlayer, EcDatagram dg);

__LIB_EXPORT void gs_player_sendInfo (GameServerPlayer, EcDatagram);

__CPP_EXTERN______________________________________________________________________________END

#endif
