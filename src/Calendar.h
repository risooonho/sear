// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2004 Simon Goodall

// $Id: Calendar.h,v 1.7 2005-04-06 13:36:08 simon Exp $

#ifndef SEAR_CALENDAR_H
#define SEAR_CALENDAR_H 1

#include <string>

#include <sigc++/object_slot.h>
#include <sigc++/connection.h>
#include <varconf/Config.h>
#include "interfaces/ConsoleObject.h"
#include <wfmath/timestamp.h>

/**
 * Sear namespace
 */ 
namespace Sear {

// Forward Declarations
class Console;

/**
 * This class regulates the client-side game time.
 * \todo Link this with server time
 */ 
class Calendar : public SigC::Object, public ConsoleObject {
public:
  // List of time areas
  typedef enum {
    INVALID = 0,
    DAWN,
    DAY,
    DUSK,
    NIGHT
  } TimeArea;

  /**
   * Default constructor
   */   
  Calendar();

  /**
   * Destructor
   */ 
  ~Calendar();

  /**
   * Initialise Calendar
   */ 
  void init();

  /**
   *  Shutdown Calendar
   */  
  void shutdown();

  /**
   * Update calendar by time_elapsed seconds.
   * @param time_elapsed Time elapsed in seconds
   */
  void update(double time_elapsed);

  /**
   * Re-sync with server time
   */
  void serverUpdate(double time);

  void setWorldTime(const WFMath::TimeStamp &ts);

  /**
   * Read Calendar config data
   */  
  void readConfig(varconf::Config &config);
  
  /**
   * Write Calendar config data
   */  
  void writeConfig(varconf::Config &config);
  
  /**
   * Callback for config updates
   * @param section The config section the key is in
   * @param key The updated key
   * @param config The config object that was updated
   */
  void config_update(const std::string &section, const std::string &key, varconf::Config &config);
 
  /**
   * Registers Calendar console commands
   * @param console The console object to register with
   */ 
  void registerCommands(Console *console);

  /**
   * Run a console command
   * @param command Console command
   * @param args Console command arguments
   */  
  void runCommand(const std::string &command, const std::string &args);
 
  /**
   * Returns number of seconds per minute
   * @return Seconds per minute
   */ 
  unsigned int getSecondsPerMinute() const { return m_seconds_per_minute; }
  
  /**
   * Returns number of minutes per hour
   * @return Minutes per hour
   */
  unsigned int getMinutesPerHour() const { return m_minutes_per_hour; }
  
  /**
   * Returns number of hours per day
   * @return Hours per day
   */
  unsigned int getHoursPerDay() const { return m_hours_per_day; }
  
  /**
   * Returns number of days per week
   * @return Days per week
   */ 
  unsigned int getDaysPerWeek() const { return m_days_per_week; }
  
  /**
   * Returns number of weeks per month
   * @return Weeks per month
   */
  unsigned int getWeeksPerMonth() const { return m_weeks_per_month; }
  
  /**
   * Returns number of months per year
   * @param Months per year
   */
  unsigned int getMonthsPerYear() const { return m_months_per_year; }
  
  /**
   * Return current seconds
   * @return Current seconds
   */
  double getSeconds() const { return m_seconds; }
  
  /**
   * Return minutes
   * @return Current minutes
   */
  unsigned int getMinutes() const { return m_minutes; }
  
  /**
   * Return hours
   * Current hours
   */ 
  unsigned int getHours() const { return m_hours; }
  
  /**
   * Return days
   * @return Current days
   */ 
  unsigned int getDays() const { return m_days; }
  
  /**
   * Return weeks
   * @return Current weeks
   */ 
  unsigned int getWeeks() const { return m_weeks; }
  
  /**
   * Return months
   * @return Current months
   */ 
  unsigned int getMonths() const { return m_months; }
  
  /*
   * Return years
   * @return Current years
   */ 
  unsigned int getYears() const { return m_years; }

  /**
   * Returns current Time area
   * @return Current time area
   */ 
  TimeArea getTimeArea() const { return m_time_area; }
  
  /**
   * Return start hour for dawn
   * @return Dawn start hour
   */ 
  unsigned int getDawnStart() const { return m_dawn_start; }

  /**
   * Return start hour for day
   * @return Day start hour
   */ 
  unsigned int getDayStart() const { return m_day_start; }

  /**
   * Return start hour for dusk
   * @return Dusk start hour
   */ 
  unsigned int getDuskStart() const { return m_dusk_start; }

  /**
   * Return start hour for night
   * @return Night start hour
   */ 
  unsigned int getNightStart() const { return m_night_start; }
  
  /**
   * Return number of seconds in current time area
   * @return Seconds in time area
   */ 
  double getTimeInArea() const { return m_time_in_area; }
  
  /**
   * Return name of day
   * @return Day name
   */
  std::string getDayName() const { return m_current_day_name; }
  
  /**
   * Return name of month
   * @return Month name
   */
  std::string getMonthName() const { return m_current_month_name; }  

  
private:
  bool m_initialised; ///< Calendar initialisation state
  
  unsigned int m_seconds_per_minute; ///< Number of seconds in a minute
  unsigned int m_minutes_per_hour;   ///< Number of minutes in an hour
  unsigned int m_hours_per_day;      ///< Number of hours in a day
  unsigned int m_days_per_week;      ///< Number of days in a week
  unsigned int m_weeks_per_month;    ///< Number of weeks in a month
  unsigned int m_months_per_year;    //</ Number of months in a year

  // Current time and date values
  WFMath::TimeStamp m_ts;
  double m_seconds; ///< Current seconds
  double m_server_seconds; ///< Predicted server seconds
  double m_seconds_counter; ///< Number of seconds passed in current day
  unsigned int m_minutes; ///< Current minutes
  unsigned int m_hours; ///< Current hour
  unsigned int m_days; ///< Current day
  unsigned int m_weeks; ///< Current week
  unsigned int m_months; ///< Current month
  unsigned int m_years; ///< Current years
  
  TimeArea m_time_area; ///< The current time area
  unsigned int m_dawn_start; ///< Hour in which dawn starts
  unsigned int m_day_start; ///< Hour in which day starts
  unsigned int m_dusk_start; ///< Hour in which dusk starts
  unsigned int m_night_start; ///< Hour in which night starts

  double m_time_in_area; ///< Time duration in current time area
  
  /**
   * Mapping between number and name
   */  
  typedef std::map<unsigned int, std::string> NameMap;
  
  NameMap m_day_names;   ///< Mapping for day names
  NameMap m_month_names; ///< Mapping for month names

  std::string m_current_day_name;   ///< Name of current day
  std::string m_current_month_name; ///< Name of current month

  SigC::Connection m_config_connection; ///< Connection object for signal
};
	
} /* namespace Sear */

#endif /* SEAR_CALENDER_H */
