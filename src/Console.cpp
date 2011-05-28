// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2008 Simon Goodall, University of Southampton

#include "common/Utility.h"
#include "common/Log.h"

#include "Bindings.h"
#include "Console.h"
#include "System.h"
#include "renderers/Graphics.h"
#include "renderers/Render.h"
#include "FileHandler.h"

#include "renderers/RenderSystem.h"

#ifdef DEBUG
  static const bool debug = true;
#else
  static const bool debug = false;
#endif

#include <fstream>
#include <cassert>

namespace Sear {
	
static const std::string TOGGLE_CONSOLE = "toggle_console";
static const std::string LIST_CONSOLE_COMMANDS = "list_commands";

static const std::string PANEL = "panel";
static const std::string FONT = "font";

static const std::string SAY = "say";
static const std::string CMD_SAY = "/say ";

std::ostream &operator<<(std::ostream &OStream, const std::list< std::string > &List) {
  std::list< std::string >::const_iterator iItem = List.begin();
  
  while(iItem != List.end()) {
    std::string sCopy = *iItem;
    std::string::size_type Offset = 0;
    
    while((Offset = sCopy.find('\\', Offset)) != std::string::npos) {
      sCopy.replace(Offset, 1, "\\\\");
      Offset += 2;
    }
    Offset = 0;
    while((Offset = sCopy.find('\n', Offset)) != std::string::npos) {
      sCopy.replace(Offset, 1, "\\n");
      Offset += 2;
    }
    OStream << sCopy << std::endl;
    ++iItem;
  }
  
  return OStream;
}

std::istream &operator>>(std::istream &IStream, std::list< std::string > &List) {
  std::string sString = "";
  
  while(std::getline(IStream, sString)) {
    std::string::size_type Offset = 0;
    
    while((Offset = sString.find("\\\\", Offset)) != std::string::npos) {
      sString.replace(Offset, 1, "\\");
      Offset += 2;
    }
    Offset = 0;
    while((Offset = sString.find("\\n", Offset)) != std::string::npos) {
      sString.replace(Offset, 1, "\n");
      Offset += 2;
    }
    List.push_back(sString);
  }
  
  return IStream;
}

Console::Console(System *system) :
  m_animateConsole(false),
  m_showConsole(false),
  m_consoleHeight(0),
  m_console_messages(std::list<std::string>()),
  m_screen_messages(std::list<screenMessage>()),
  m_panel_id(0),
  m_panel_state(-1),
  m_font_state(-1),
  m_system(system),
  m_CaretPosition(0),
  m_bTabOnce(false),
  m_initialised(false)
{
  assert((system != NULL) && "System is NULL");
}

Console::~Console() {
  if (m_initialised) shutdown();
}

bool Console::init() {
  assert(m_initialised == false);

  // Register console commands
  registerCommand(TOGGLE_CONSOLE, this);
  registerCommand(LIST_CONSOLE_COMMANDS, this);
  //Makes sure at least one key is bound to the console
  Bindings::bind("backquote", "/" + std::string(TOGGLE_CONSOLE));
  
  std::ifstream HistoryFile((m_system->getFileHandler()->getUserDataPath() + "history").c_str());
  
  HistoryFile >> m_CommandHistory;
  m_CommandHistoryIterator = m_CommandHistory.end();
  m_initialised = true;
  return true;
}

void Console::shutdown() {
  assert(m_initialised == true);
  if (debug) Log::writeLog("Shutting down console.", Log::LOG_DEFAULT);
  
  std::ofstream HistoryFile((m_system->getFileHandler()->getUserDataPath() + "history").c_str());
  
  if (m_panel_id > 0) {
    RenderSystem::getInstance().releaseTexture(m_panel_id);
    m_panel_id = 0;
  }

  // TODO: Sometimes this can stall for a long time
  HistoryFile << m_CommandHistory;
  m_CommandHistory.clear();
  m_CommandHistoryIterator = m_CommandHistory.end();
  m_registered_commands.clear();
  m_CommandStarts.clear();
  m_console_messages.clear();
  m_screen_messages.clear();
  m_initialised = false;
}

void Console::pushMessage(const std::string &message, int type, int duration) {
  assert ((m_initialised == true) && "Console not initialised");
  // Is this a screen message
  if (type & SCREEN_MESSAGE) {	
    //If we have reached our message limit, remove the oldest message regardless of duration
    if (m_screen_messages.size() >= MAX_MESSAGES) m_screen_messages.erase(m_screen_messages.begin());
    m_screen_messages.push_back(screenMessage(message, m_system->getTime() + duration));
  }
  // Is this a console message?
  if (type & CONSOLE_MESSAGE) {
    //If we have reached our message limit, remove the oldest message regardless of duration
    if (m_console_messages.size() >= MAX_MESSAGES) m_console_messages.erase(m_console_messages.begin());
    m_console_messages.push_back(message);
  }
}

void Console::draw(void) {
  assert ((m_initialised == true) && "Console not initialised");
  // If we are animating and putting console into visible state,
  //  the raise height a tad
  if (m_animateConsole && m_showConsole) {
    m_consoleHeight += CONSOLE_SPEED;
    // Have we reached full height?
    if (m_consoleHeight >= CONSOLE_HEIGHT) {
      // Disable animation
      m_consoleHeight = CONSOLE_HEIGHT;
      m_animateConsole = 0;
    }
    renderConsoleMessages();
  // Ife we are animating and putting console into hidden state,
  //  the lower height a tad
  } else if (m_animateConsole && !m_showConsole) {
    m_consoleHeight -= CONSOLE_SPEED;
    // Have we become visible?
    if (m_consoleHeight <= 0) {
      //Disable animation
      m_consoleHeight = 0;
      m_animateConsole = 0;
    }
    renderConsoleMessages();
  // Else are we just plain visible?    
  } else if (m_showConsole) {
    renderConsoleMessages();
  }
  //Screen messages are always visible
  renderScreenMessages();
}

void Console::renderConsoleMessages(void) {
  assert ((m_initialised == true) && "Console not initialised");
  Render *renderer = RenderSystem::getInstance().getRenderer();
  if (!renderer) {
    Log::writeLog("Console: Error - Renderer object not created", Log::LOG_ERROR);
    return;
  }
  std::list<std::string>::const_iterator I;
  int i;
  //Render console panel
  int consoleOffset = CONSOLE_HEIGHT - m_consoleHeight;
  //Make panel slightly transparent
  renderer->setColour(0.0f, 0.0f, 1.0f, 0.85f);
  if (m_panel_id < 1) {
    m_panel_id = RenderSystem::getInstance().requestTexture(PANEL);
    m_panel_state = RenderSystem::getInstance().requestState(PANEL);
  }
  RenderSystem::getInstance().switchState(m_panel_state);

  renderer->drawTextRect(0, 0, renderer->getWindowWidth(), m_consoleHeight, m_panel_id);
  
  if (m_font_state == -1) {
    m_font_state = RenderSystem::getInstance().requestState(FONT);
  }
  RenderSystem::getInstance().switchState(m_font_state);
  renderer->setColour(1.0f, 1.0f, 0.0f, 1.0f);
  //Render console messges
  for (I = m_console_messages.begin(), i = 0; I != m_console_messages.end(); ++I, ++i) {
    int j = m_console_messages.size() - i;
    renderer->print(CONSOLE_TEXT_OFFSET_X, CONSOLE_TEXT_OFFSET_Y + j * FONT_HEIGHT - consoleOffset, (char*)(*I).c_str(), 0);
  }
  //Render current command string
  std::string str = CONSOLE_PROMPT_STRING + m_Command;
  renderer->print(CONSOLE_TEXT_OFFSET_X, CONSOLE_TEXT_OFFSET_Y - consoleOffset, const_cast<char *>(str.c_str()), 0);
  
  // Render the caret
  unsigned int Spaces = m_CaretPosition + CONSOLE_PROMPT_LENGTH;
  
  str.reserve(Spaces + 2);
  str.assign(Spaces, ' ');
  str += CONSOLE_CURSOR_STRING;
  renderer->print(CONSOLE_TEXT_OFFSET_X, CONSOLE_TEXT_OFFSET_Y - consoleOffset, const_cast<char *>(str.c_str()), 0);
}

void Console::renderScreenMessages() {
  assert ((m_initialised == true) && "Console not initialised");
  if (m_screen_messages.empty()) return;	
  int i;

  if (m_font_state == -1) {
    m_font_state = RenderSystem::getInstance().requestState(FONT);
  }
  RenderSystem::getInstance().switchState(m_font_state);
  Render *renderer = RenderSystem::getInstance().getRenderer();
  renderer->setColour(1.0f, 1.0f, 0.0f, 1.0f);
  // Get screen height so we can calculate offset correctly
  int height = renderer->getWindowHeight();
  //Get time so we can remove expired messages
  unsigned int current_time = m_system->getTime();
  //Render messges
  std::list<screenMessage>::const_iterator I = m_screen_messages.begin();
  std::list<screenMessage>::const_iterator Iend = m_screen_messages.end();
  for (i = 0; I != Iend; ++I, ++i) {
    const std::string &str = I->first;
    renderer->print(CONSOLE_TEXT_OFFSET_X, height - ((i + 1) * FONT_HEIGHT ), const_cast<char*>(str.c_str()), 0);
  }
  //Remove expired messages
  //TODO this currently only removes the messges from  the top of the list
  //     should a messages at the top have a longer expiry time than one
  //     later in the list, the later one will stay in the list until the 
  //     higher entries have been removed
  bool loop = true;
  while (loop) {
  if (m_screen_messages.empty()) break;	
    loop = false; // break unless it gets set to true again
    screenMessage sm = *m_screen_messages.begin(); // Get first messge
    unsigned int message_time = sm.second;
    if (current_time > message_time) { // Check expiry time
      m_screen_messages.erase(m_screen_messages.begin());
      loop = true; // Go again
    }
  }
}

void Console::toggleConsole() {
  assert ((m_initialised == true) && "Console not initialised");
  // Start the animation	
  m_animateConsole = 1;
  // Toggle state
  m_showConsole = !m_showConsole;
  m_system->vEnableKeyRepeat(m_showConsole);
}

void Console::vHandleInput(const SDLKey &KeySym, const Uint16 &UnicodeCharacter)
{
  if(KeySym == SDLK_UP) {
    // get the previous command from the command history
    if(m_CommandHistoryIterator != m_CommandHistory.begin()) {
      --m_CommandHistoryIterator;
      m_Command = *m_CommandHistoryIterator;
      m_CaretPosition = m_Command.length();
    }
  } else if (KeySym == SDLK_DOWN) {
    // get the next command from the command history
    if(m_CommandHistoryIterator != m_CommandHistory.end()) {
      ++m_CommandHistoryIterator;
      if(m_CommandHistoryIterator != m_CommandHistory.end()) {
        m_Command = *m_CommandHistoryIterator;
        m_CaretPosition = m_Command.length();
      } else {
        m_Command = "";
        m_CaretPosition = 0;
      }
    }
  } else if(KeySym == SDLK_PAGEUP) {
    // get the previous command from the command history starting with the same content as command
    if(m_CommandHistoryIterator != m_CommandHistory.begin()) {
      std::list< std::string >::iterator iIterator = m_CommandHistoryIterator;
      std::string sPartialCommand = m_Command.substr(0, m_CaretPosition);
      
      do {
        --iIterator;
        if(iIterator->substr(0, sPartialCommand.length()) == sPartialCommand) {
          m_CommandHistoryIterator = iIterator;
          m_Command = *iIterator;
          break;
        }
      } while(iIterator != m_CommandHistory.begin());
    }
  } else if(KeySym == SDLK_PAGEDOWN) {
    // get the previous command from the command history starting with the same content as command
    if(m_CommandHistoryIterator != m_CommandHistory.end()) {
      std::list< std::string >::iterator iIterator = m_CommandHistoryIterator;
      std::string sPartialCommand = m_Command.substr(0, m_CaretPosition);
      
      while(++iIterator != m_CommandHistory.end()) {
        if(iIterator->substr(0, sPartialCommand.length()) == sPartialCommand) {
          m_CommandHistoryIterator = iIterator;
          m_Command = *iIterator;
          break;
        }
      }
    }
  } else if(KeySym == SDLK_RETURN) {
    // accept the current command and execute it; reset the command
    if(m_Command.empty() == false) {
      m_system->runCommand(m_Command);
      m_CommandHistory.push_back(m_Command);
      m_CommandHistoryIterator = m_CommandHistory.end();
      m_Command = "";
      m_CaretPosition = 0;
    }
  } else if(KeySym == SDLK_BACKSPACE) {
    if(m_CaretPosition > 0) {
      m_Command = m_Command.erase(m_CaretPosition - 1, 1);
      m_CaretPosition--;
    }
  } else if(KeySym == SDLK_DELETE) {
    if(m_CaretPosition < m_Command.length()) {
      m_Command = m_Command.erase(m_CaretPosition, 1);
    }
  } else if(KeySym == SDLK_LEFT) {
    if(m_CaretPosition > 0) {
      m_CaretPosition--;
    }
  } else if(KeySym == SDLK_RIGHT) {
    if(m_CaretPosition < m_Command.length()) {
      m_CaretPosition++;
    }
  } else if(KeySym == SDLK_HOME) {
    m_CaretPosition = 0;
  } else if(KeySym == SDLK_END) {
    m_CaretPosition = m_Command.length();
  } else if(KeySym == SDLK_TAB) {
    if((m_Command.length() > 1) && (m_Command[0] == '/') && (m_Command.find(' ') == std::string::npos)) {
      std::string sCommandStart(m_Command.substr(1));
      std::multimap< std::string, std::string >::size_type Count = m_CommandStarts.count(sCommandStart);
      
      if(Count == 0) {
        pushMessage("No command match.", CONSOLE_MESSAGE, 0);
      } else if(Count == 1) {
        m_Command = '/' + m_CommandStarts.find(sCommandStart)->second + ' ';
        m_CaretPosition = m_Command.length();
      } else {
        typedef std::multimap< std::string, std::string >::iterator MI;
        std::pair< MI, MI > Range(m_CommandStarts.equal_range(sCommandStart));
        MI iCommand(Range.first);
        std::string sCommand(iCommand->second);
        
        if(m_bTabOnce == true) {
          std::string sPossibilies(sCommand);
          
          while(++iCommand != Range.second) {
            sPossibilies += ", ";
            sPossibilies += iCommand->second;
          }
          pushMessage(sPossibilies, CONSOLE_MESSAGE, 0);
          m_bTabOnce = false;
        } else {
          m_bTabOnce = true;
        }
        
        std::string::size_type i = sCommandStart.length();
        while((i <= sCommand.length()) && (Count == m_CommandStarts.count(sCommand.substr(0, i)))) {
          ++i;
        }
        m_Command = '/' + sCommand.substr(0, i - 1);
        m_CaretPosition = m_Command.length();
      }
    }
  } else if(UnicodeCharacter < 0x80 && UnicodeCharacter > 0) {
    m_Command.insert(m_CaretPosition, 1, static_cast< char >(UnicodeCharacter));
    m_CaretPosition++;
  }
  if(KeySym != SDLK_TAB) {
    m_bTabOnce = false;
  }
}

void Console::registerCommand(const std::string &command, ConsoleObject *object) {
//  if (debug) Log::writeLog(std::string("Registering: ") + command, Log::LOG_INFO);
  // Assign the ConsoleObject to the command
  assert(m_registered_commands.find(command) == m_registered_commands.end());
  m_registered_commands[command] = object;
  // Prepare the command starts = a multimap that assigns the command start to the full commands.
  for(std::string::size_type i = 1; i <= command.length(); ++i) {
    m_CommandStarts.insert(std::make_pair(command.substr(0, i), command));
  }
}

void Console::runCommand(const std::string &comd) {
  assert ((m_initialised == true) && "Console not initialised");
  if (comd.empty()) return; // Ignore empty string
  std::string command = comd;
  // Grab first character of command string
  char c = command.c_str()[0];
  // Check to see if command is a command, or a speech string
  if ((c != '/' && c != '+' && c != '-')) {
    // Its a speech string, so SAY it
    // FIXME /say is not always available!
    if (m_registered_commands[SAY]) {
      runCommand(std::string(CMD_SAY) + command);
    } else {
      if (debug) Log::writeLog(std::string("Cannot SAY, not in game yet: ") + command, Log::LOG_ERROR);
      pushMessage("Cannot SAY, not it game yet" , CONSOLE_MESSAGE, 0);
    }
    return; 
  }

  // If command has a leading /, remove it
  std::string command_string = (c == '/')? command.substr(1) : command;
  // Split string into command / arguments pair
  Tokeniser tokeniser = Tokeniser();
  tokeniser.initTokens(command_string);
  std::string cmd = tokeniser.nextToken();
  const std::string &args = tokeniser.remainingTokens();
  
  // This allows command abbreviation
  std::multimap< std::string, std::string >::size_type Count = m_CommandStarts.count(cmd);
  
  if(Count == 0) {
    if (debug) Log::writeLog(std::string("Unknown command: ") + command, Log::LOG_ERROR);
    pushMessage("Unknown command: " + cmd, CONSOLE_MESSAGE, 0);
  } else if(Count == 1) {
    cmd = m_CommandStarts.find(cmd)->second;
  } else if (m_registered_commands.find(cmd) == m_registered_commands.end()) {
    // the above checks for commands that are abbrevs of others 'get' <=> 'get_time'
    pushMessage("Ambigious command: " + cmd, CONSOLE_MESSAGE, 0);
  }


  // Print all commands apart form toggle console to the console
  if (cmd != TOGGLE_CONSOLE) pushMessage(command_string, CONSOLE_MESSAGE, 0);

  // Find the registered handler
  std::map<std::string, ConsoleObject*>::iterator I = m_registered_commands.find(cmd);

  if (I != m_registered_commands.end()) {
    ConsoleObject* con_obj = I->second;
    // If object exists, run the command
    if (con_obj) con_obj->runCommand(cmd, args);
  }
}

void Console::runCommand(const std::string &command, const std::string &args) {
  assert ((m_initialised == true) && "Console not initialised");
  // This command toggles the console
  if (command == TOGGLE_CONSOLE) {
    toggleConsole();
  }
  // This commands prints all currently registers commands to the Log File
  else if (command == LIST_CONSOLE_COMMANDS) {
    for (std::map<std::string, ConsoleObject*>::const_iterator I = m_registered_commands.begin(); I != m_registered_commands.end(); ++I) {
      // TODO - should we check to see if I->second is valid?
//      if (debug)
	      Log::writeLog(I->first, Log::LOG_INFO);
    }
  }
}

} /* namespace Sear */
