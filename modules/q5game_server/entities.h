#ifndef GAMESERVER_ENTITIES_H
#define GAMESERVER_ENTITIES_H 1

#include <system/macros.h>
#include <tools/ecasyncvc.h>

#include "enet/enet.h"

struct GameServerPlayer_s; typedef struct GameServerPlayer_s* GameServerPlayer;
struct GameServerRealm_s; typedef struct GameServerRealm_s* GameServerRealm;

struct GameServerEntities_s; typedef struct GameServerEntities_s* GameServerEntities;

__CPP_EXTERN______________________________________________________________________________START

__LIB_EXPORT GameServerEntities gse_create (ENetHost* enetHost, uint32_t gameEngineSrvNo, const EcString name);

__LIB_EXPORT void gse_destroy (GameServerEntities*);

__LIB_EXPORT void gse_addPlayer (GameServerEntities, ENetPeer* peer);

__LIB_EXPORT void gse_rmPlayer (GameServerEntities, GameServerPlayer*);

__LIB_EXPORT GameServerRealm gse_addRealm (GameServerEntities, const EcString name);

// misc

__LIB_EXPORT void gse_message (GameServerEntities, ENetPeer*, EcBuffer, int channel);

__LIB_EXPORT GameServerRealm gse_realm (GameServerEntities, const EcString name);

__LIB_EXPORT void gse_sendPlayers (GameServerEntities, GameServerRealm, ENetPeer* peer);

__CPP_EXTERN______________________________________________________________________________END

#endif
