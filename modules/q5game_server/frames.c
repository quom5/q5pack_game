#include "frames.h"

#include <tools/ecbins.h>
#include <types/ecudc.h>
#include <tools/ecbins.h>

//-------------------------------------------------------------------------------------

void gs_frames_init (EcBuffer buf, GameServerFrameCursor* cursor)
{
  cursor->orig = buf;
  cursor->pos = 0;
}

//-------------------------------------------------------------------------------------

int gs_frames_next (GameServerFrameCursor* cursor, GameServerFrame* frame)
{
  if (cursor->pos == 0)
  {
    const unsigned char* posbuf = cursor->orig->buffer + cursor->pos;
    
    frame->ch1 = posbuf [0];
    frame->ch2 = posbuf [1];

    {
      EcBuffer_s buf = { cursor->orig->buffer + 2, cursor->orig->size - 2 };
      
      frame->content = ecbins_read(&buf, NULL);
    }
      
    cursor->pos = 1;
    
    return TRUE;
  }
  else
  {
    if (frame->content)
    {
      ecudc_destroy(&(frame->content));
    }
    
    return FALSE;
  }
}

//-------------------------------------------------------------------------------------
