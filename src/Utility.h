// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2002 Simon Goodall, University of Southampton

#ifndef _UTILITY_H_
#define _UTILITY_H_ 1

#include <wfmath/quaternion.h>
//class Quaternion;
#include <string>
#include "config.h"

#ifdef HAVE_SSTREAM
#include <sstream>
#define END_STREAM "" 
typedef std::stringstream SSTREAM;
#elif defined(HAVE_STRSTREAM)
#include <strstream>
typedef std::strstream SSTREAM;
// strstream requires a null character at the end of the stream
#define END_STREAM std::ends
#else
#error "sstream or strstream not found!"
#endif

namespace Sear {

template <class out_value, class in_value>
void cast_stream(const in_value &in, out_value &out) {
  SSTREAM ss;
  ss << in << END_STREAM;
#ifdef HAVE_SSTREAM
  SSTREAM sss(ss.str());
  sss >> out;
#else
  ss >> out;
#endif
}


template <class T>
std::string string_fmt(const T & t) {
  SSTREAM ss;
  ss << t << END_STREAM;
  return ss.str();
}

//std::string floatToString(float &);
//std::string intToString(int &);

void reduceToUnit(float vector[3]);

void calcNormal(float v[3][3], float out[3]);

WFMath::Quaternion MatToQuat(float m[4][4]);

//void QuatToMatrix(WFMath::Quaternion quat, float m[4][4]);
void QuatToMatrix(WFMath::Quaternion quat, float m[4][4]);

WFMath::Quaternion EulerToQuat(float roll, float pitch, float yaw);

WFMath::Quaternion QuatMul(const WFMath::Quaternion &q1, const WFMath::Quaternion &q2);

WFMath::Quaternion QuatSlerp(WFMath::Quaternion from, WFMath::Quaternion to, float t);

} /* namespace Sear */
#endif /* _UTILITY_H_ */

