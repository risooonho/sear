// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2002 Simon Goodall, University of Southampton
#include "System.h"
#include "Character.h"
#include "client.h"

#include <SDL/SDL.h>


#include <Atlas/Objects/Operation/Move.h>
#include <Atlas/Objects/Operation/Touch.h>
#include <Atlas/Objects/Operation/Talk.h>

#include <Eris/Connection.h>
#include "WorldEntity.h"
#include <math.h>
#include <wfmath/atlasconv.h>

#include "System.h"
#include "EventHandler.h"
#include "Event.h"

#include "cmd.h"
#include "Config.h"
#include "Utility.h"
#include <string>

#include "Log.h"


namespace Sear {

Character::Character(WorldEntity *self, System *system) :
  _self(self),
  _system(system),
  _walk_speed(0.0f),
  _run_speed(0.0f),
  _rotate_speed(0.0f),
  _angle(0.0f),
  _rate(0.0f),
  _speed(0.0f),
  _strafe_speed(0.0f),
  _orient(WFMath::Quaternion(1.0f, 0.0f, 0.0f, 0.0f)),
  _time(0),
  _run_modifier(false)
	
{
}

Character::~Character() {
}

bool Character::init() {
  float divisor, zaxis;
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return false;
  }
  readConfig();
  _orient = _self->getOrientation();
  divisor = sqrt(square(_orient.vector().x()) + square(_orient.vector().y()) + square(_orient.vector().z()));
  zaxis = (divisor != 0.0f) ? (_orient.vector().z() / divisor) : 0.0f; // Check for possible divide by zero error;
  _angle = -2.0f * acos(_self->getOrientation().scalar()) * zaxis;
  return true;
}

void Character::shutdown() {
  writeConfig();
}

void Character::moveForward(float speed) {
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  _speed += speed;
  updateLocals(true);
}

void Character::strafe(float speed) {
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  _strafe_speed += speed;
  updateLocals(true);
}

void Character::rotate(float rate) {
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  if (rate != CMD_modifier) _rate += rate * _rotate_speed;
  updateLocals(true);
  if (_rate != 0.0f) _system->getEventHandler()->addEvent(Event(EF_UPDATE_CHAR_ROTATE, NULL, EC_TIME, server_update_interval + System::instance()->getTime()));
}

void Character::updateLocals(bool send_to_server) {
  float x, y, z;
  float mod_speed;
  float angle;
  unsigned int ticks;
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  ticks = SDL_GetTicks();
  angle = deg_to_rad(_rate * ((ticks - _time) / 1000.0f));
  _angle += angle;
//  if (_angle == WFMath::Pi) _angle += 0.01; // Stops entity points due west which causes cyphesis to flip it upside down;
  mod_speed = (_run_modifier) ? (_speed * _run_speed) : (_speed * _walk_speed);
  z = 0.0f;
  y = mod_speed * -sin(_angle);
  x = mod_speed * cos(_angle);  
  mod_speed = (_run_modifier) ? (_strafe_speed * _run_speed) : (_strafe_speed * _walk_speed);
  z += 0.0f;
  y += mod_speed * -sin(_angle + (WFMath::Pi / 2.0f));
  x += mod_speed * cos(_angle + (WFMath::Pi / 2.0f));
  _time = ticks;
  _orient *= WFMath::Quaternion(WFMath::Vector<3>(0.0f, 0.0f, 1.0f), -angle);
  if (send_to_server) updateMove(x, y, z, _orient);
}

void Character::updateMove(float x, float y, float z, WFMath::Quaternion orient) {
  Atlas::Objects::Operation::Move move;
  Atlas::Message::Object::MapType args;
  Atlas::Message::Object::ListType vel;
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  move = Atlas::Objects::Operation::Move::Instantiate();
  vel.push_back(x);
  vel.push_back(y);
  vel.push_back(z);
  args["velocity"] = vel;
  args["orientation"] = orient.toAtlas();
  args["loc"] = _self->getContainer()->getID();
  args["id"] = _self->getID();
  move.SetFrom(_self->getID());
  move.SetArgs(Atlas::Message::Object::ListType(1, args));
  Eris::Connection::Instance()->send(move);
}

void Character::getEntity(const std::string &id) {
  Atlas::Objects::Operation::Move move;
  Atlas::Message::Object::MapType args;
  Atlas::Message::Object::ListType pos;
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  move = Atlas::Objects::Operation::Move::Instantiate();
  pos.push_back(0.0);
  pos.push_back(0.0);
  pos.push_back(0.0);
  args["pos"] = pos;
  args["loc"] = _self->getID();
  args["id"] = id;
  move.SetFrom(_self->getID());
  move.SetArgs(Atlas::Message::Object::ListType(1, args));
  Eris::Connection::Instance()->send(move);
}

void Character::dropEntity(const std::string &name, int quantity) {
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  if (quantity == 0) {
    Log::writeLog( "Quantity is 0! Dropping nothing.", Log::DEFAULT);
    return;
  }
  Log::writeLog(std::string("Dropping ") + string_fmt(quantity) + std::string(" items of ") + name, Log::DEFAULT);
  std::map<std::string, int> inventory;
  for (unsigned int i = 0; (quantity) && (i < _self->getNumMembers()); i++) {
    WorldEntity *we = (WorldEntity*)_self->getMember(i);
    if (we->getName() == name) {
      Atlas::Objects::Operation::Move move;
      Atlas::Message::Object::MapType args;
      Atlas::Message::Object::ListType pos;
      move = Atlas::Objects::Operation::Move::Instantiate();
      pos.push_back(_self->GetPos().x());
      pos.push_back(_self->GetPos().y());
      pos.push_back(_self->GetPos().z());
      args["pos"] = pos;
      args["loc"] = _self->getContainer()->getID();
      args["id"] = we->getID();
      move.SetFrom(_self->getID());
      move.SetArgs(Atlas::Message::Object::ListType(1, args));
      Eris::Connection::Instance()->send(move);
      quantity--;
    }
  }
}

void Character::touchEntity(const std::string &id) {
  Atlas::Objects::Operation::Touch touch;
  Atlas::Message::Object::MapType args;
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  touch = Atlas::Objects::Operation::Touch::Instantiate();
  args["id"] = id;
  touch.SetFrom(_self->getID());
  touch.SetTo(id);
  touch.SetArgs(Atlas::Message::Object::ListType(1, args));
  Eris::Connection::Instance()->send(touch);
}

void Character::displayInventory() {
  if (!_self) return;
  std::map<std::string, int> inventory;
  for (unsigned int i = 0; i < _self->getNumMembers(); i++)
    inventory[_self->getMember(i)->getName()]++;
  for (std::map<std::string, int>::const_iterator I = inventory.begin(); I != inventory.end(); I++) {
//    std::cout << I->second << " - " << I->first << std::endl;
  std::string quantity = string_fmt(I->second);
  std::string name = I->first;
    _system->pushMessage(std::string(name + std::string(" - ") + std::string(quantity)), 3);
  }
 
}

void Character::say(const std::string &msg) {
  Atlas::Objects::Operation::Talk talk;
  Atlas::Message::Object::MapType args;
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  talk =  Atlas::Objects::Operation::Talk::Instantiate();
  args["say"] = msg;
  talk.SetArgs(Atlas::Message::Object::ListType(1, args));
  talk.SetFrom(_self->getID());
  Eris::Connection::Instance()->send(talk);
}

void Character::toggleRunModifier() {
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  _run_modifier = !_run_modifier;
  updateLocals(true);
  if (_run_modifier) _system->runCommand(std::string(CMD_CALLY_SET_MOTION) + std::string(" ") + std::string(CMD_character_run_args));
  else _system->runCommand(std::string(CMD_CALLY_SET_MOTION) + std::string(" ") + std::string(CMD_character_walk_args));
}

void Character::readConfig() {
  std::string temp;
  Config *general = _system->getGeneral();
  if (!general) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  temp = general->getAttribute(KEY_character_walk_speed);
  _walk_speed = (temp.empty()) ? (DEFAULT_character_walk_speed) : atof(temp.c_str());
  temp = general->getAttribute(KEY_character_run_speed);
  _run_speed = (temp.empty()) ? (DEFAULT_character_run_speed) : atof(temp.c_str());
  temp = general->getAttribute(KEY_character_rotate_speed);
  _rotate_speed = (temp.empty()) ? (DEFAULT_character_rotate_speed) : atof(temp.c_str());
}

void Character::writeConfig() {
  Config *general = _system->getGeneral();
  if (!general) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  general->setAttribute(KEY_character_walk_speed, string_fmt(_walk_speed));
  general->setAttribute(KEY_character_run_speed, string_fmt(_run_speed));
  general->setAttribute(KEY_character_rotate_speed, string_fmt(_rotate_speed));
}

void Character::giveEntity(const std::string &name, int quantity, const std::string &target) {
  if (!_self) {
    Log::writeLog("Character: Error - Character object not created", Log::ERROR);
    return;
  }
  if (quantity == 0) {
    Log::writeLog( "Quantity is 0! Dropping nothing.", Log::DEFAULT);
    return;
  }
  Log::writeLog(std::string("Giving ") + string_fmt(quantity) + std::string(" items of ") + name + std::string(" to ") + target, Log::DEFAULT);
  std::map<std::string, int> inventory;
  for (unsigned int i = 0; (quantity) && (i < _self->getNumMembers()); i++) {
    WorldEntity *we = (WorldEntity*)_self->getMember(i);
    if (we->getName() == name) {
      Atlas::Objects::Operation::Move move;
      Atlas::Message::Object::MapType args;
      Atlas::Message::Object::ListType pos;
      move = Atlas::Objects::Operation::Move::Instantiate();
      pos.push_back(_self->GetPos().x());
      pos.push_back(_self->GetPos().y());
      pos.push_back(_self->GetPos().z());
      args["pos"] = pos;
      args["loc"] = target;
      args["id"] = we->getID();
      move.SetFrom(_self->getID());
      move.SetArgs(Atlas::Message::Object::ListType(1, args));
      Eris::Connection::Instance()->send(move);
      quantity--;
    }
  }
}

}
