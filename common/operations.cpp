// This file may be redistributed and modified only under the terms of
// the GNU Lesser General Public License (See COPYING for details).
// Copyright (C) 2004 Alistair Riddoch

#include "operations.h"

UseData::~UseData()
{
}

UseData * UseData::copy() const
{
    UseData * copied = UseData::alloc();
    *copied = *this;
    return copied;
}

bool UseData::instanceOf(int classNo) const
{
    if(USE_NO == classNo) return true;
    return SetData::instanceOf(classNo);
}

//freelist related methods specific to this class
UseData *UseData::defaults_UseData = 0;
UseData *UseData::begin_UseData = 0;

UseData *UseData::alloc()
{
    if(begin_UseData) {
        UseData *res = begin_UseData;
        assert( res->m_refCount == 0 );
        res->m_attrFlags = 0;
        res->m_attributes.clear();
        begin_UseData = (UseData *)begin_UseData->m_next;
        return res;
    }
    return new UseData(UseData::getDefaultObjectInstance());
}

void UseData::free()
{
    m_next = begin_UseData;
    begin_UseData = this;
}


UseData *UseData::getDefaultObjectInstance()
{
    if (defaults_UseData == 0) {
        defaults_UseData = new UseData;
        defaults_UseData->attr_objtype = "op";
        defaults_UseData->attr_serialno = 0;
        defaults_UseData->attr_refno = 0;
        defaults_UseData->attr_seconds = 0.0;
        defaults_UseData->attr_future_seconds = 0.0;
        defaults_UseData->attr_stamp = 0.0;
        defaults_UseData->attr_parents = std::list<std::string>(1, "wield");
    }
    return defaults_UseData;
}

UseData *UseData::getDefaultObject()
{
    return UseData::getDefaultObjectInstance();
}



WieldData::~WieldData()
{
}

WieldData * WieldData::copy() const
{
    WieldData * copied = WieldData::alloc();
    *copied = *this;
    return copied;
}

bool WieldData::instanceOf(int classNo) const
{
    if(WIELD_NO == classNo) return true;
    return SetData::instanceOf(classNo);
}

//freelist related methods specific to this class
WieldData *WieldData::defaults_WieldData = 0;
WieldData *WieldData::begin_WieldData = 0;

WieldData *WieldData::alloc()
{
    if(begin_WieldData) {
        WieldData *res = begin_WieldData;
        assert( res->m_refCount == 0 );
        res->m_attrFlags = 0;
        res->m_attributes.clear();
        begin_WieldData = (WieldData *)begin_WieldData->m_next;
        return res;
    }
    return new WieldData(WieldData::getDefaultObjectInstance());
}

void WieldData::free()
{
    m_next = begin_WieldData;
    begin_WieldData = this;
}


WieldData *WieldData::getDefaultObjectInstance()
{
    if (defaults_WieldData == 0) {
        defaults_WieldData = new WieldData;
        defaults_WieldData->attr_objtype = "op";
        defaults_WieldData->attr_serialno = 0;
        defaults_WieldData->attr_refno = 0;
        defaults_WieldData->attr_seconds = 0.0;
        defaults_WieldData->attr_future_seconds = 0.0;
        defaults_WieldData->attr_stamp = 0.0;
        defaults_WieldData->attr_parents = std::list<std::string>(1, "wield");
    }
    return defaults_WieldData;
}

WieldData *WieldData::getDefaultObject()
{
    return WieldData::getDefaultObjectInstance();
}
