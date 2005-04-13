// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2002 Simon Goodall

// $Id: Log.cpp,v 1.9 2005-04-13 12:16:03 simon Exp $

#include "Log.h"
#include <iostream>

/* LOG AIMS
 * Will provide  ability to log each type to a different file
 * FREQUENT LOG ACTIONS SHOULD BE IN A #if #endif BLOCK and
 * only included in debug mode
 */ 

#ifdef USE_MMGR
  #include "common/mmgr.h"
#endif

#ifdef DEBUG
  static const bool debug = true;
#else
  static const bool debug = false;
#endif
  
namespace Sear {
  
void Log::writeLog(const std::string &msg, LogLevel level) {
  std::string type;
  switch (level) {
    case LOG_DEFAULT: type = "Default"; break;
    case LOG_ERROR: type = "Error"; break;
    case LOG_WARNING: type = "Warning"; break;
    case LOG_ERIS: type = "Eris"; break;		
    case LOG_INFO: type = "Info"; break;
  }
  std::cout << type << ": " << msg << std::endl << std::flush;
}

} /* namespace Sear */

