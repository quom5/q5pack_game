#include <system/macros.h>

// entc includes
#include <system/ecsocket.h>
#include <system/ecsignal.h>
#include <tools/ecjson.h>
#include <tools/ecbins.h>
#include <types/ecudc.h>
#include <system/ecthread.h>

#include <curses.h>

#include <map>
#include <string>

// enet includes
#include <enet/enet.h>

static int _STDCALL module_thread_run (void* ptr);

namespace {
  
  class GamePlayer
  {
    
  public:
    
    GamePlayer (const char* name, int x, int y, bool spawned)
    : m_name (name)
    , m_x (x)
    , m_y (y)
    , m_spawned (spawned)
    {
      setPosition (x, y);
    }
    
    void setPosition (uint_t x, uint_t y)
    {
      if (m_spawned)
      {
        wipe ();
        
        m_x = x;
        m_y = y;
        
        mvaddstr (m_y, m_x, "X");
        mvaddstr (m_y + 1, m_x - 2, m_name.c_str());
        mvaddstr (0, 0, "");
        
        refresh ();        
      }
      else
      {
        
      }
    }
    
    void wipe ()
    {
      mvaddstr (m_y, m_x, " ");
      mvaddstr (m_y + 1, m_x - 2, "                ");
      mvaddstr (0, 0, "");      
    }
    
    void unspawn ()
    {
      m_spawned = false;
      
      wipe ();
      
      mvaddstr (m_y, m_x, "-");
      mvaddstr (m_y + 1, m_x - 2, m_name.c_str());
      mvaddstr (0, 0, "");
      
      refresh ();              
    }
    
    void spawn ()
    {
      m_spawned = true;
      setPosition (m_x, m_y);
    }
    
  private:
    
    std::string m_name;
    
    uint_t m_x;
    
    uint_t m_y;
    
    bool m_spawned;
    
  };
  
  class GameClient
  {
  
  public:
    
    //------------------------------------------------------------------------------------------------

    GameClient (ENetPeer* peer)
    : m_peer (peer)
    , m_win (NULL)
    , m_id (0)
    , m_spawned (false)
    {
      m_posX = 0;
      m_posY = 0;
      
      m_player = ecudc_create(ENTC_UDC_NODE, NULL);
      
      m_posXNode = ecudc_create (ENTC_UDC_UINT32, "PosX");
      m_posYNode = ecudc_create (ENTC_UDC_UINT32, "PosY");
      m_posZNode = ecudc_create (ENTC_UDC_UINT32, "PosZ");
      
      {
        EcUdc h = m_posXNode;
        ecudc_add (m_player, &h);
      }
      {
        EcUdc h = m_posYNode;
        ecudc_add (m_player, &h);
      }
      {
        EcUdc h = m_posZNode;
        ecudc_add (m_player, &h);
      }         
    }
    
    //------------------------------------------------------------------------------------------------

    ~GameClient ()
    {
      leave ();
      
      delwin (m_win);
    }
    
    //------------------------------------------------------------------------------------------------

    void increasePosX ()
    {
      if (m_posX < 80)
      {
        m_posX++;  
        setPosition ();
      }
    }
    
    //------------------------------------------------------------------------------------------------
    
    void decreasePosX ()
    {
      if (m_posX > 0)
      {
        m_posX--; 
        setPosition ();
      }  
    }
    
    //------------------------------------------------------------------------------------------------
    
    void increasePosY ()
    {
      if (m_posY < 80)
      {
        m_posY++;
        setPosition ();
      }  
    }
    
    //------------------------------------------------------------------------------------------------
    
    void decreasePosY ()
    {
      if (m_posY > 0)
      {
        m_posY--;
        setPosition ();
      }  
    }
    
    //------------------------------------------------------------------------------------------------
    
    uint_t posX ()
    {
      return m_posX;
    }
    
    //------------------------------------------------------------------------------------------------
    
    uint_t posY ()
    {
      return m_posY;
    }
    
    //------------------------------------------------------------------------------------------------
    
    void setPlayer (const char* name)
    {
      EcUdc player = ecudc_create(ENTC_UDC_NODE, NULL);
      
      ecudc_add_asString (player, "Name", name);
      
      sendNode ("02", player);
      
      ecudc_destroy(&player);
    }
    
    //--------------------------------------------------------------------------
    
    void join ()
    {      
      EcUdc player = ecudc_create(ENTC_UDC_NODE, NULL);

      ecudc_add_asString (player, "Realm", "the shire");

      sendNode ("03", player);
      
      ecudc_destroy(&player);      
    }
    
    //--------------------------------------------------------------------------
    
    void leave ()
    {      
      EcUdc player = ecudc_create(ENTC_UDC_NODE, NULL);
      
      sendNode ("04", player);
      
      ecudc_destroy(&player); 
      
      unsetRealm ();
    }
    
    //------------------------------------------------------------------------------------------------

    void tooglePause ()
    {
      if (m_spawned)
      {
        ENetPacket * packet = enet_packet_create ("06", 2, 0);
        
        enet_peer_send	(m_peer, 0, packet);
        
        m_spawned = false;
      }
      else
      {
        ENetPacket * packet = enet_packet_create ("05", 2, 0);
        
        enet_peer_send	(m_peer, 0, packet);
        
        m_spawned = true;
      }
    }

    //------------------------------------------------------------------------------------------------

    void requestAllPlayers ()
    {
      ENetPacket * packet = enet_packet_create ("22", 2, 0);
      
      enet_peer_send	(m_peer, 0, packet);
    }
    
    //------------------------------------------------------------------------------------------------
    
    void handleMessage (EcBuffer buf)
    {
      if (buf->size > 1)
      {
        switch (buf->buffer [0])
        {
          case '0':
          {
            switch (buf->buffer [1])
            {
              case '1': // disconnect
              {
                unsetRealm ();
              }
              break;
              case '2': // authenticated
              {
                setAuthenticated ((const char*)buf->buffer + 2, buf->size -2);
              }
              break;
            }
          }
          break;
          case '1':
          {
            switch (buf->buffer [1])
            {
              case '3': // new player showed off
              {
                newPlayer ((const char*)buf->buffer + 2, buf->size - 2);
              }
              break;
              case '4': // player left
              {
                clearPlayer ((const char*)buf->buffer + 2, buf->size - 2);
              }
              break;
              case '5': // player left
              {
                spawnPlayer ((const char*)buf->buffer + 2, buf->size - 2);
              }
              break;
              case '6': // player left
              {
                unspawnPlayer ((const char*)buf->buffer + 2, buf->size - 2);
              }
              break;
              case '7': // set player position
              {
                setPlayerPosition ((const char*)buf->buffer + 2, buf->size - 2);
              }
              break;
            }
          }
          break;
        }
      }
    }
    
    //------------------------------------------------------------------------------------------------

  private:
    
    //------------------------------------------------------------------------------------------------
    
    void sendNode (const EcString command, EcUdc node)
    {
      EcBuffer bins = ecbins_write(node, command);

      ENetPacket * packet = enet_packet_create (bins->buffer, bins->size, 0);
      
      enet_peer_send	(m_peer, 0, packet);
      
      ecbuf_destroy(&bins);
    }
    
    //------------------------------------------------------------------------------------------------
    
    void setPosition ()
    {
      ecudc_setUInt32 (m_posXNode, m_posX);
      ecudc_setUInt32 (m_posYNode, m_posY);
      
      sendNode ("07", m_player);
    }
    
    //------------------------------------------------------------------------------------------------

    void setRealm ()
    {
      m_win = initscr();

      if (m_win)
      {
        printf("open the arena\n");
        
        mvaddstr(0, 10, "A R E N A");
        refresh();
        
        requestAllPlayers ();      
      }
    }
    
    //------------------------------------------------------------------------------------------------
    
    void setAuthenticated (const char* buffer, ulong_t len)
    {
      EcBuffer_s posbuf = { (unsigned char*)buffer, len };

      EcUdc node = ecbins_read (&posbuf, NULL);
      
      // get the own id
      m_id = ecudc_get_asUInt32(node, "Id", 0);

      printf("your a worthy hero with id %i\n", m_id);
    }
    
    //------------------------------------------------------------------------------------------------

    void unsetRealm ()
    {
      if (m_win)
      {
        endwin();

        m_win = NULL;
      }
      
      m_players.clear ();
    }
    
    //------------------------------------------------------------------------------------------------

    void newPlayer (const char* buffer, ulong_t len)
    {
      EcBuffer_s posbuf = { (unsigned char*)buffer, len };

      EcUdc node = ecbins_read (&posbuf, NULL);
      
      int id = ecudc_get_asUInt32(node, "Id", 0);
      if (id == m_id)
      {
        setRealm ();
      }
      else if (id > 0)
      {          
        if (m_win)
        {
          bool isSpawned = ecudc_get_asUInt32(node, "Spawned", 0) == 1;
          
          int x = ecudc_get_asUInt32(node, "PosX", 0);
          int y = ecudc_get_asUInt32(node, "PosY", 0);
          
          m_players [id] = new GamePlayer (ecudc_get_asString(node, "Name", "unknown"), x, y, isSpawned);
        }
      }
    }
    
    //------------------------------------------------------------------------------------------------
    
    void clearPlayer (const char* buffer, ulong_t len)
    {
      if (m_win)
      {
        EcBuffer_s posbuf = { (unsigned char*)buffer, len };

        EcUdc node = ecbins_read(&posbuf, NULL);
        
        int id = ecudc_get_asUInt32(node, "Id", 0);
        if (id > 0 && id != m_id)
        {
          std::map<int, GamePlayer*>::const_iterator i = m_players.find (id);
          if (i != m_players.end())
          {
            i->second->wipe ();
            refresh (); 
          }
        }
      }        
    }

    //------------------------------------------------------------------------------------------------
    
    void spawnPlayer (const char* buffer, ulong_t len)
    {
      if (m_win)
      {
        EcBuffer_s posbuf = { (unsigned char*)buffer, len };

        EcUdc node = ecbins_read(&posbuf, NULL);
        
        int id = ecudc_get_asUInt32(node, "Id", 0);
        if (id == m_id)
        {
          mvaddstr (m_posY, m_posX, " ");
          
          m_posX = ecudc_get_asUInt32(node, "PosX", 0);
          m_posY = ecudc_get_asUInt32(node, "PosY", 0);        

          mvaddstr (m_posY, m_posX, "O");
          mvaddstr (0, 0, "");
          
          refresh ();
          
          m_spawned = true;
        }
        else if (id > 0)
        {
          std::map<int, GamePlayer*>::const_iterator i = m_players.find (id);
          if (i != m_players.end())
          {
            uint_t posX = ecudc_get_asUInt32(node, "PosX", 0);
            uint_t posY = ecudc_get_asUInt32(node, "PosY", 0);        

            i->second->spawn (); 
            i->second->setPosition (posX, posY);
          }
        }
      }
    }
    
    //------------------------------------------------------------------------------------------------
    
    void unspawnPlayer (const char* buffer, ulong_t len)
    {
      if (m_win)
      {
        EcBuffer_s posbuf = { (unsigned char*)buffer, len };

        EcUdc node = ecbins_read(&posbuf, NULL);
        
        int id = ecudc_get_asUInt32(node, "Id", 0);
        if (id == m_id)
        {
          m_spawned = false;
        }
        else if (id > 0)
        {
          std::map<int, GamePlayer*>::const_iterator i = m_players.find (id);
          if (i != m_players.end())
          {
            i->second->unspawn ();
          }
        }
      }
    }

    //------------------------------------------------------------------------------------------------
    
    void setPlayerPosition (const char* buffer, ulong_t len)
    {
      if (m_win)
      {
        EcBuffer_s posbuf = { (unsigned char*)buffer, len };

        EcUdc node = ecbins_read(&posbuf, NULL);
        
        int id = ecudc_get_asUInt32(node, "Id", 0);
        if (id > 0 && id != m_id)
        {
          std::map<int, GamePlayer*>::const_iterator i = m_players.find (id);
          if (i != m_players.end())
          {
            uint_t posX = ecudc_get_asUInt32(node, "PosX", 0);
            uint_t posY = ecudc_get_asUInt32(node, "PosY", 0);
            
            i->second->setPosition (posX, posY);
          }
        }
      }
    }
    
    //------------------------------------------------------------------------------------------------

  private:
    
    EcUdc m_player;
    
    uint_t m_posX;
    
    uint_t m_posY;
    
    EcUdc m_posXNode;
    
    EcUdc m_posYNode;
    
    EcUdc m_posZNode;
    
    ENetPeer* m_peer;
    
    std::map<int, GamePlayer*> m_players;
    
    WINDOW* m_win;
    
    int m_id;
    
    bool m_spawned;
    
  };
  
  //===================================================================================================
  
  
  class ClientConnection
  {
    
  public:
    
    ClientConnection ()
    : m_host (enet_host_create (NULL, 1, 2, 0, 0))
    , m_run (true)
    , m_gc (NULL)
    {
    }
    
    ~ClientConnection ()
    {
      m_run = false;
      enet_peer_disconnect (m_peer, 0);
      
      ecthread_join(m_thread);
      
      if (m_gc)
      {
        delete m_gc;
        m_gc = NULL;        
      }
      
      if (m_peer)
      {
        enet_peer_reset (m_peer);
      }

      if (m_host)
      {
        enet_host_destroy(m_host);
      }
    }
  
    //--------------------------------------------------------------------------

    bool connect (const char* host, int port, const char* name)
    {
      if (m_host == NULL)
      {
        return false;
      }
      
      ENetAddress address;

      enet_address_set_host (&address, host);
      address.port = port;

      m_peer = enet_host_connect (m_host, &address, 2, 0);
      if (m_peer == NULL)
      {
        return false;
      }
      
      m_name = name;
      
      m_thread = ecthread_new ();
      
      ecthread_start(m_thread, module_thread_run, this);      
      
      return true;
    }
    
    //--------------------------------------------------------------------------
    
    bool run ()
    {
      ENetEvent event;
      
      while (enet_host_service (m_host, &event, 100) > 0)
      {
        switch (event.type)
        {
          case ENET_EVENT_TYPE_NONE: break;
          case ENET_EVENT_TYPE_CONNECT:
          {
            m_gc = new GameClient (m_peer);
            
            m_gc->setPlayer (m_name.c_str());
          }
          break;
          case ENET_EVENT_TYPE_RECEIVE:
          {
            EcBuffer_s buf = { event.packet->data, event.packet->dataLength };
           
            m_gc->handleMessage(&buf);
            
            enet_packet_destroy (event.packet);
          }
          break;
          case ENET_EVENT_TYPE_DISCONNECT:
          {
            delete m_gc;
            m_gc = NULL;
            
            m_run = false;
          }
          break;
        }    
      }
      return m_run;
    }
    
    //--------------------------------------------------------------------------

    void handleKeys ()
    {
      if (m_gc == NULL)
      {
        return;
      }
      
      char ch [2];        
      read(STDIN_FILENO, ch, 2);
      
      switch (ch[0])
      {
        case 'j':
        {
          m_gc->join ();
        }
        break;
        case 'l':
        {
          m_gc->leave ();
        }
        break;
        case 'p':
        {
          m_gc->tooglePause ();
        }
        break;
        case 'a':
        {
          mvaddstr (m_gc->posY (), m_gc->posX (), " ");
          
          m_gc->decreasePosX ();
          
          mvaddstr (m_gc->posY (), m_gc->posX (), "O");
          mvaddstr (0, 0, "");
          
          refresh ();
        }
        break;
        case 'd':
        {
          mvaddstr (m_gc->posY (), m_gc->posX (), " ");
          
          m_gc->increasePosX ();
          
          mvaddstr (m_gc->posY (), m_gc->posX (), "O");
          mvaddstr (0, 0, "");
          
          refresh ();
        }
        break;
        case 'w':
        {
          mvaddstr (m_gc->posY (), m_gc->posX (), " ");
          
          m_gc->decreasePosY ();
          
          mvaddstr (m_gc->posY (), m_gc->posX (), "O");
          mvaddstr (0, 0, "");
          
          refresh ();
        }
        break;
        case 's':
        {
          mvaddstr (m_gc->posY (), m_gc->posX (), " ");
          
          m_gc->increasePosY ();
          
          mvaddstr (m_gc->posY (), m_gc->posX (), "O");
          mvaddstr (0, 0, "");
          
          refresh ();
        }
        break;
      }      
    }
    
    //--------------------------------------------------------------------------
  
  private:
    
    ENetHost* m_host; 
    
    ENetPeer* m_peer;
    
    bool m_run;
    
    GameClient* m_gc;
    
    std::string m_name;
    
    EcThread m_thread;
    
  };
  
}

//================================================================================================

static int _STDCALL module_thread_run (void* ptr)
{
  return static_cast<ClientConnection*>(ptr)->run();
}

//-------------------------------------------------------------------------------------


int main (int argc, char *argv[])
{
  if (enet_initialize () != 0)
  {
    fprintf (stderr, "An error occurred while initializing ENet.\n");
    return EXIT_FAILURE;
  }

  if (argc < 4)
  {
    return EXIT_FAILURE;
  }
  
  ClientConnection cc;

  if (!cc.connect(argv[1], atoi(argv[2]), argv[3]))
  {
    return 0;
  }
    
  //noecho();
  //curs_set(FALSE);
    
  EcEventContext ec = ece_context_new ();
  ecsignal_init (ec);
  
  EcEventQueue queue = ece_list_create (ec, NULL);
  
  ece_list_add (queue, STDIN_FILENO, ENTC_EVENTTYPE_READ, NULL);
  
  int res = ENTC_EVENT_TIMEOUT;
  
  while (res != ENTC_EVENT_ABORT)
  {
    switch (res)
    {
      case STDIN_FILENO:
      {
        cc.handleKeys();
      }
      break;
    }
    
    res = ece_list_wait (queue, 10000, NULL);
  }
  
  ecsignal_done ();

  ece_context_delete(&ec);
    
  return 0;
}

//================================================================================================

