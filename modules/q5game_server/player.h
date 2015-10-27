#ifndef GAMESERVER_PLAYER_H
#define GAMESERVER_PLAYER_H 1

#include "entities.h"
#include "frames.h"

#include <system/macros.h>
#include <tools/ecasyncvc.h>

#include "enet/enet.h"

//-------------------------------------------------------------------------------------

#define C_COMMAND_PLAYER      '0'
#define C_COMMAND_BROADCAST   '1'
#define C_COMMAND_REQUEST     '2'

#define C_MSG_TEST            '0'
#define C_MSG_DISCONNECT      '1'
#define C_MSG_AUTHENTICATE    '2'
#define C_MSG_JOIN_REALM      '3'
#define C_MSG_LEAVE_REALM     '4'
#define C_MSG_SPAWN           '5'
#define C_MSG_UNSPAWN         '6'
#define C_MSG_POSITION        '7'

#define C_MSG_LISTREALMS      '1'
#define C_MSG_PLAYERS         '2'

//-------------------------------------------------------------------------------------

__CPP_EXTERN______________________________________________________________________________START

__LIB_EXPORT GameServerPlayer gs_player_create (GameServerEntities entities, ENetPeer* peer);

__LIB_EXPORT void gs_player_destroy (GameServerPlayer*);

__LIB_EXPORT void gs_player_message (GameServerPlayer, ENetPeer*, GameServerFrame*, int channel);

__LIB_EXPORT void gs_player_sendInfo (GameServerPlayer, GameServerRealm, ENetPeer* peer);

__LIB_EXPORT void gs_player_fillInfo (GameServerPlayer, GameServerRealm, EcTable, int row);

__CPP_EXTERN______________________________________________________________________________END

#endif
