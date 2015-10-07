#ifndef MYSQLPROT_STATES_H
#define MYSQLPROT_STATES_H 1

#include "realm.h"

#include <system/macros.h>
#include <system/ecsocket.h>
#include <system/ecasyncio.h>
#include <tools/ecasyncvc.h>

#include <core/q5.h>

__CPP_EXTERN______________________________________________________________________________START

__LIB_EXPORT EcAsyncUdpContext prot_context_create (GameServerPlayer player);

__CPP_EXTERN______________________________________________________________________________END

#endif
