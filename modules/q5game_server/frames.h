#ifndef GAMESERVER_FRAMES_H
#define GAMESERVER_FRAMES_H 1

#include <system/macros.h>
#include <tools/ecasyncvc.h>
#include <types/ecbuffer.h>

typedef struct 
{
  
  // reference
  EcBuffer orig;
  
  ulong_t pos;
  
} GameServerFrameCursor;

typedef struct
{
  
  ubyte_t ch1;
  
  ubyte_t ch2;
  
  ubyte_t flag1;
  
  ubyte_t flag2;
  
  EcUdc content;
  
} GameServerFrame;

__CPP_EXTERN______________________________________________________________________________START

__LIB_EXPORT void gs_frames_init (EcBuffer buf, GameServerFrameCursor*);

__LIB_EXPORT int gs_frames_next (GameServerFrameCursor*, GameServerFrame*);

__CPP_EXTERN______________________________________________________________________________END

#endif
