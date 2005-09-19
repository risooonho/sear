// This file may be redistributed and modified only under the terms of
// the GNU Lesser General Public License (See COPYING for details).
// Copyright (C) 2004 Alistair Riddoch

#ifndef COMMON_OPERATIONS_H
#define COMMON_OPERATIONS_H

#include <Atlas/Objects/Operation.h>

#include <Atlas/Objects/Generic.h>

namespace Atlas { namespace Objects { namespace Operation {

extern int ATTACK_NO;

class Attack : public Generic
{
  public:
    Attack() {
        (*this)->setType("attack", ATTACK_NO);
    }
};

} } }

#endif
