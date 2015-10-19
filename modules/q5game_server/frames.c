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
      EcBuffer_s buf = { cursor->orig->buffer + 8, cursor->orig->size - 8 };
      
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

void gs_frames_send (ENetPeer* peer, ubyte_t ch1, ubyte_t ch2, EcUdc node, int reliable)
{
  unsigned char chh [8] = {ch1, ch2, 0, 0, 0, 0, 0, 0};
  EcBuffer_s h = {chh, 8};
  
  EcBuffer bins = ecbins_write (node, &h);
  
  {
    uint32_t* ptr = (uint32_t*)(bins->buffer + 4);
    *ptr = bins->size;
  }
  
  ENetPacket * packet = enet_packet_create (bins->buffer, bins->size, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
  
  enet_peer_send (peer, 0, packet);
  
  ecbuf_destroy(&bins);
}

//-------------------------------------------------------------------------------------

void gs_frames_broadcast (ENetHost* host, ubyte_t ch1, ubyte_t ch2, EcUdc node, int reliable)
{
  unsigned char chh [8] = {ch1, ch2, 0, 0, 0, 0, 0, 0};

  if (node)
  {
    EcBuffer_s h = {chh, 8};

    EcBuffer bins = ecbins_write(node, &h);
    
    {
      uint32_t* ptr = (uint32_t*)(bins->buffer + 4);
      *ptr = bins->size;
    }
    
    ENetPacket * packet = enet_packet_create (bins->buffer, bins->size, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    
    enet_host_broadcast	(host, 0, packet);
    
    ecbuf_destroy(&bins);
  }
  else
  {
    ENetPacket * packet = enet_packet_create (chh, 8, 0);
    
    enet_host_broadcast	(host, 0, packet);    
  }  
}

//-------------------------------------------------------------------------------------


