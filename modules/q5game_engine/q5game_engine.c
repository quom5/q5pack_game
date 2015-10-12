#include <system/macros.h>

// quom includes
#include <core/q5.h>
#include <core/q5modules.h>
#include <core/q5core.h>
#include <core/q5auth.h>

// entc includes
#include <adbo.h>
#include <adbo_context.h>

#include <tools/ecjson.h>
#include <types/ecmap.h>
#include <tools/ecdata.h>
#include <utils/ecmessages.h>


//================================================================================================

static Q5ModuleProperties properties = {"GAME_E", "game engine", 10001, 0x4001};

typedef struct {
  
  // reference
  Q5Core core;
  
  Q5ModuleInstance* instance;
  
    
} Q5Module;

// add exported methods of this shared library
#include <core/q5module.inc>

#define GAME_ENGINE_PLAYER_NEW 10001
#define GAME_ENGINE_PLAYER_DEL 10002
#define GAME_ENGINE_PLAYER_POS 10003

//================================================================================================

int _STDCALL module_callback_player_new (void* ptr, EcMessageData* dIn, EcMessageData* dOut)
{
  eclogger_fmt (LL_DEBUG, properties.name, "player", "new player registered");

  return ENTC_RESCODE_OK;
}

//-------------------------------------------------------------------------------------

int _STDCALL module_callback_player_del (void* ptr, EcMessageData* dIn, EcMessageData* dOut)
{
  eclogger_fmt (LL_DEBUG, properties.name, "player", "player removed");

  return ENTC_RESCODE_OK;
}

//-------------------------------------------------------------------------------------

int _STDCALL module_callback_player_pos (void* ptr, EcMessageData* dIn, EcMessageData* dOut)
{
  eclogger_fmt (LL_DEBUG, properties.name, "player", "set position");

  return ENTC_RESCODE_OK;
}

//================================================================================================

void module_config (Q5Module* self, const EcUdc result_item)
{
}

//-------------------------------------------------------------------------------------

int module_start (Q5Module* self)
{  
  eclogger_fmt (LL_TRACE, self->instance->name, "start", "register callbacks for %i", self->instance->msgid);
  
  // register the callback method
  ecmessages_add (self->instance->msgid, GAME_ENGINE_PLAYER_NEW, module_callback_player_new, self);
  ecmessages_add (self->instance->msgid, GAME_ENGINE_PLAYER_DEL, module_callback_player_del, self);
  ecmessages_add (self->instance->msgid, GAME_ENGINE_PLAYER_POS, module_callback_player_pos, self);
  
  return TRUE;
}

//-------------------------------------------------------------------------------------

int module_stop (Q5Module* self)
{
  // remove the callback method
  ecmessages_removeAll (self->instance->msgid);

  return TRUE;
}

//-------------------------------------------------------------------------------------

Q5Module* module_create (Q5Core core, Q5ModuleInstance* instance)
{
  Q5Module* self = ENTC_NEW (Q5Module);
  
  self->core = core;
  self->instance = instance;
  
  return self;
}
  
//-------------------------------------------------------------------------------------

void module_destroy (Q5Module** pself)
{    
  //Q5Module* self = *pself;
  
  ENTC_DEL (pself, Q5Module);
}

//-------------------------------------------------------------------------------------
