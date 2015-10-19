#ifndef GAMESERVER_PLAYERS_H
#define GAMESERVER_PLAYERS_H 1

#include <system/macros.h>
#include <tools/ecasyncvc.h>

#include "enet/enet.h"

#include "entities.h"

__CPP_EXTERN______________________________________________________________________________START

__LIB_EXPORT GameServerRealm gs_realm_create (ENetHost* enetHost, uint32_t gameEngineSrvNo, const EcString name);

__LIB_EXPORT void gs_realm_destroy (GameServerRealm*);

__LIB_EXPORT void gs_realm_broadcast (GameServerRealm, ubyte_t ch1, ubyte_t ch2, EcUdc node, int reliable);

__LIB_EXPORT const EcString gs_realm_name (GameServerRealm);

__CPP_EXTERN______________________________________________________________________________END

#endif
