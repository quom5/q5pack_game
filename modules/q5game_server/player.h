#ifndef GAMESERVER_PLAYER_H
#define GAMESERVER_PLAYER_H 1

#include "entities.h"

#include <system/macros.h>
#include <tools/ecasyncvc.h>

#include "enet/enet.h"

__CPP_EXTERN______________________________________________________________________________START

__LIB_EXPORT GameServerPlayer gs_player_create (GameServerEntities entities, ENetPeer* peer);

__LIB_EXPORT void gs_player_destroy (GameServerPlayer*);

__LIB_EXPORT void gs_player_message (GameServerPlayer, ENetPeer*, EcBuffer, int channel);

__LIB_EXPORT void gs_player_sendInfo (GameServerPlayer, GameServerRealm, ENetPeer* peer);

__CPP_EXTERN______________________________________________________________________________END

#endif
