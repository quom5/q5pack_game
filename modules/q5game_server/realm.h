#ifndef GAMESERVER_PLAYERS_H
#define GAMESERVER_PLAYERS_H 1

#include <system/macros.h>
#include <tools/ecasyncvc.h>

struct GameServerPlayer_s; typedef struct GameServerPlayer_s* GameServerPlayer;

struct GameServerRealm_s; typedef struct GameServerRealm_s* GameServerRealm;

__CPP_EXTERN______________________________________________________________________________START

__LIB_EXPORT GameServerRealm gs_realm_create (EcAsynUdpDispatcher dispatcher);

__LIB_EXPORT void gs_realm_destroy (GameServerRealm*);

__LIB_EXPORT EcAsyncUdpContext gs_realm_newContext (GameServerRealm);

__LIB_EXPORT void gs_realm_removePlayer (GameServerRealm, GameServerPlayer);

__LIB_EXPORT void gs_realm_broadcast (GameServerRealm, EcAsyncUdpContext ctx, const EcString command, EcUdc node);

__LIB_EXPORT void gs_realm_sendPlayers (GameServerRealm, EcDatagram, GameServerPlayer);

__CPP_EXTERN______________________________________________________________________________END

#endif
