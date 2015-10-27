#include "frames.h"

#include <tools/ecbins.h>
#include <types/ecudc.h>
#include <tools/ecbins.h>

#define C_BUFFER_SIZE_SMALL    0x00
#define C_BUFFER_SIZE_NORMAL   0x01
#define C_BUFFER_SIZE_BIG      0x02

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

    eclogger_fmt (LL_TRACE, "GAME_S", "frame", "received %i bytes [%i|%i]", cursor->orig->size, frame->ch1, frame->ch2);  
    
    
    /*
    if (posbuf [2] & C_BUFFER_SIZE_BIG)
    {
      EcBuffer_s buf = { cursor->orig->buffer + 8, cursor->orig->size - 8 };
      
      frame->content = ecbins_read(&buf, NULL);      
    }
    else if (posbuf [2] & C_BUFFER_SIZE_NORMAL)
    {
      EcBuffer_s buf = { cursor->orig->buffer + 6, cursor->orig->size - 6 };      

      frame->content = ecbins_read(&buf, NULL);      
    }
    else
    {
      EcBuffer_s buf = { cursor->orig->buffer + 5, cursor->orig->size - 5 };      
      
      frame->content = ecbins_read(&buf, NULL);      
    }
     */

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

ENetPacket* gs_frame_createPacket (ubyte_t ch1, ubyte_t ch2, EcUdc node, int reliable)
{
  ENetPacket* packet;
  
  unsigned char chh [2] = {ch1, ch2};
  EcBuffer_s h = {chh, 2};

  if (isAssigned (node))
  {
    EcBuffer bins = ecbins_write (node, &h);

    packet = enet_packet_create (bins->buffer, bins->size, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);    
    
    ecbuf_destroy(&bins);
  }
  else
  {
    packet = enet_packet_create (chh, 2, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);    
  }
  
  /*
  
  unsigned char chh [8] = {ch1, ch2, 0, 0, 0, 0, 0, 0};
  EcBuffer_s h = {chh, 8};
  
  if (isAssigned (node))
  {
    EcBuffer bins = ecbins_write (node, &h);
    
    if (bins->size < 255) // use 1 byte for len
    { 
      bins->buffer [7] = bins->size;
      
      // shift the first 4 bytes by 2 bytes to right
      bins->buffer [6] = bins->buffer [3];
      bins->buffer [5] = bins->buffer [2];
      bins->buffer [4] = bins->buffer [1];
      bins->buffer [3] = bins->buffer [0];
      
      packet = enet_packet_create (bins->buffer + 3, bins->size - 3, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);    
    }
    else if (bins->size < 65536) // use 2 bytes for len
    {
      bins->buffer [2] |= C_BUFFER_SIZE_NORMAL;

      uint16_t* ptr = (uint16_t*)(bins->buffer + 6);
      *ptr = bins->size;
      
      // shift the first 4 bytes by 2 bytes to right
      bins->buffer [5] = bins->buffer [3];
      bins->buffer [4] = bins->buffer [2];
      bins->buffer [3] = bins->buffer [1];
      bins->buffer [2] = bins->buffer [0];
      
      packet = enet_packet_create (bins->buffer + 2, bins->size - 2, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);    
    }
    else // use 4 bytes for len
    {
      bins->buffer [2] |= C_BUFFER_SIZE_BIG;

      uint32_t* ptr = (uint32_t*)(bins->buffer + 4);
      *ptr = bins->size;    
      
      packet = enet_packet_create (bins->buffer, bins->size, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    }  

    ecbuf_destroy(&bins);
  }
  else
  {
    chh [7] = 0;
    
    // shift the first 4 bytes by 2 bytes to right
    chh [6] = chh [3];
    chh [5] = chh [2];
    chh [4] = chh [1];
    chh [3] = chh [0];
    
    packet = enet_packet_create (chh, 5, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
  }
   
   */
  
  return packet;
}

//-------------------------------------------------------------------------------------

void gs_frames_send (ENetPeer* peer, ubyte_t ch1, ubyte_t ch2, EcUdc node, int reliable)
{
  ENetPacket* packet = gs_frame_createPacket (ch1, ch2, node, reliable);
  
  enet_peer_send (peer, 0, packet);
}

//-------------------------------------------------------------------------------------

void gs_frames_broadcast (ENetHost* host, ubyte_t ch1, ubyte_t ch2, EcUdc node, int reliable)
{
  ENetPacket* packet = gs_frame_createPacket (ch1, ch2, node, reliable);
  
  enet_host_broadcast	(host, 0, packet);
}

//-------------------------------------------------------------------------------------


