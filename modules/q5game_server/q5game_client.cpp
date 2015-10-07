#include <system/macros.h>

// entc includes
#include <system/ecsocket.h>
#include <system/ecsignal.h>
#include <tools/ecjson.h>
#include <types/ecudc.h>

#include <curses.h>

#include <map>
#include <string>

namespace {
  
  class GamePlayer
  {
    
  public:
    
    GamePlayer (const char* name, uint_t x, uint_t y) : m_name (name), m_x (x), m_y (y)
    {
      setPosition (x, y);
    }
    
    void setPosition (uint_t x, uint_t y)
    {
      wipe ();
      
      m_x = x;
      m_y = y;
      
      mvaddstr (m_y, m_x, "X");
      mvaddstr (m_y + 1, m_x - 2, m_name.c_str());
      mvaddstr (0, 0, "");
      
      refresh ();        
    }
    
    void wipe ()
    {
      mvaddstr (m_y, m_x, " ");
      mvaddstr (m_y + 1, m_x - 2, "                ");
      mvaddstr (0, 0, "");      
    }
    
  private:
    
    std::string m_name;
    
    uint_t m_x;
    
    uint_t m_y;
    
  };
  
  class GameClient
  {
  
  public:
    
    //------------------------------------------------------------------------------------------------

    GameClient (EcSocket socket)
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
      
      m_socket = socket;      
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
      
      requestAllPlayers ();
    }
    
    //------------------------------------------------------------------------------------------------

    void requestAllPlayers ()
    {
      ecsocket_write (m_socket, "22", 2);      
    }
    
    //------------------------------------------------------------------------------------------------
    
    void handleMessage ()
    {
      static char buffer [1000];
      
      int count = ecsocket_readBunch (m_socket, buffer, 1000);
      
      mvaddstr (0, 40, buffer);
      mvaddstr (0, 0, "");
      
      refresh ();  
      
      if (count > 1)
      {
        switch (buffer [0])
        {
          case '0':
          {
            switch (buffer [1])
            {
              case '0': // ping
              {
                
              }
                break;
              case '3': // set own position
              {
                
              }
                break;
            }
          }
            break;
          case '1':
          {
            switch (buffer [1])
            {
              case '1': // player left
              {
                clearPlayer (buffer + 2);
              }
                break;
              case '2': // new player showed off
              {
                newPlayer (buffer + 2);
              }
                break;
              case '3': // set player position
              {
                setPlayerPosition (buffer + 2);
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
      EcString jsonText = ecjson_write(node);
      
      EcString commandText = ecstr_cat2(command, jsonText);
      
      EcBuffer buf = ecbuf_create_str (&commandText);
      
      ecsocket_write (m_socket, buf->buffer, buf->size);  
      
      ecbuf_destroy(&buf);
      
      ecstr_delete(&jsonText);
    }
    
    //------------------------------------------------------------------------------------------------
    
    void setPosition ()
    {
      ecudc_setUInt32 (m_posXNode, m_posX);
      ecudc_setUInt32 (m_posYNode, m_posY);
      
      sendNode ("03", m_player);
    }
    
    //------------------------------------------------------------------------------------------------

    void newPlayer (const char* buffer)
    {
      EcUdc node = ecjson_read(buffer, NULL);

      int id = ecudc_get_asUInt32(node, "Id", 0);
      if (id > 0)
      {
        uint_t posX = ecudc_get_asUInt32(node, "PosX", 0);
        uint_t posY = ecudc_get_asUInt32(node, "PosY", 0);        

        m_players [id] = new GamePlayer (ecudc_get_asString(node, "Name", "unknown"), posX, posY);
      }
    }
    
    //------------------------------------------------------------------------------------------------
    
    void clearPlayer (const char* buffer)
    {
      EcUdc node = ecjson_read(buffer, NULL);
      
      int id = ecudc_get_asUInt32(node, "Id", 0);
      if (id > 0)
      {
        std::map<int, GamePlayer*>::const_iterator i = m_players.find (id);
        if (i != m_players.end())
        {
          i->second->wipe ();
        }
      }
    }

    //------------------------------------------------------------------------------------------------
    
    void setPlayerPosition (const char* buffer)
    {
      EcUdc node = ecjson_read(buffer, NULL);
      
      int id = ecudc_get_asUInt32(node, "Id", 0);
      if (id > 0)
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
    
    //------------------------------------------------------------------------------------------------

  private:
    
    EcUdc m_player;
    
    uint_t m_posX;
    
    uint_t m_posY;
    
    EcUdc m_posXNode;
    
    EcUdc m_posYNode;
    
    EcUdc m_posZNode;
    
    EcSocket m_socket;
    
    std::map<int, GamePlayer*> m_players;
    
  };
  
}

//================================================================================================

int main (int argc, char *argv[])
{
  WINDOW * mainwin = initscr();
  
  //noecho();
  //curs_set(FALSE);
    
  EcEventContext ec = ece_context_new ();
  
  ecsignal_init (ec);
  
  EcSocket socket = ecsocket_new (ec, ENTC_SOCKET_PROTOCOL_UDP);
  
  if (ecsocket_connect(socket, "127.0.0.1", 44000))
  {
    mvaddstr(0, 1, "OK");
    refresh();
  }
  
  GameClient gc (socket);
  
  const char* playerName;
  
  if (argc > 1)
  {
    playerName = argv[1];
  }
  else
  {
    playerName = "[unknown]";    
  }
  
  gc.setPlayer (playerName);

  mvaddstr(0, 10, playerName);
  refresh();
  
  EcEventQueue queue = ece_list_create (ec, NULL);
  
  ece_list_add (queue, ecsocket_getReadHandle (socket), ENTC_EVENTTYPE_READ, NULL);
  ece_list_add (queue, STDIN_FILENO, ENTC_EVENTTYPE_READ, NULL);
  
  int res = ENTC_EVENT_TIMEOUT;
  
  while (res != ENTC_EVENT_ABORT)
  {
    switch (res)
    {
      case STDIN_FILENO:
      {
        char ch [2];        
        read(STDIN_FILENO, ch, 2);
        
        switch (ch[0])
        {
          case 'a':
          {
            mvaddstr (gc.posY (), gc.posX (), " ");
                
            gc.decreasePosX ();
            
            mvaddstr (gc.posY (), gc.posX (), "O");
            mvaddstr (0, 0, "");

            refresh ();
          }
          break;
          case 'd':
          {
            mvaddstr (gc.posY (), gc.posX (), " ");
            
            gc.increasePosX ();
            
            mvaddstr (gc.posY (), gc.posX (), "O");
            mvaddstr (0, 0, "");
            
            refresh ();
          }
          break;
          case 'w':
          {
            mvaddstr (gc.posY (), gc.posX (), " ");
            
            gc.decreasePosY ();
            
            mvaddstr (gc.posY (), gc.posX (), "O");
            mvaddstr (0, 0, "");
            
            refresh ();
          }
          break;
          case 's':
          {
            mvaddstr (gc.posY (), gc.posX (), " ");
            
            gc.increasePosY ();
            
            mvaddstr (gc.posY (), gc.posX (), "O");
            mvaddstr (0, 0, "");
            
            refresh ();
          }
          break;
        }
      }
      break;
      case ENTC_EVENT_TIMEOUT:
      {
        ecsocket_write (socket, "00", 2);
      } 
      break;
      default:
      {
        gc.handleMessage ();
      }
      break;
    }
    
    res = ece_list_wait (queue, 10000, NULL);
  }

  ecsocket_write (socket, "01", 2);
  
  ecsignal_done ();

  ece_context_delete(&ec);
  
  delwin(mainwin);
  endwin();
  refresh();
  
  return 0;
}

//================================================================================================

