/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Yanqing Chen  <shellqiqi@outlook.com>
 */

#ifndef CYQ_UTILS_H
#define CYQ_UTILS_H

#include <string>
#include <sstream>
#include <exception>
#include <ctime>
#include <iomanip>

namespace cyq {

class DataSize
{
public:
  static uint64_t
  GetBytes (const std::string &s)
  {
    uint64_t v;
    bool ok = DoParse (s, v);
    if (!ok)
      {
        throw ("Could not parse rate: " + s);
      }
    return v;
  }

private:
  static bool
  DoParse (const std::string &s, uint64_t &v)
  {
    const std::string::size_type n = s.find_first_not_of ("0123456789");
    if (n != std::string::npos)
      { // Found non-numeric
        std::istringstream iss;
        iss.str (s.substr (0, n));
        uint64_t r;
        iss >> r;
        std::string trailer = s.substr (n, std::string::npos);
        if (trailer == "B")
          {
            // Byte
            v = r;
          }
        else if (trailer == "KB")
          {
            // KiloByte
            v = r * 1000;
          }
        else if (trailer == "KiB")
          {
            // KibiByte
            v = r * 1024;
          }
        else if (trailer == "MB")
          {
            // MegaByte
            v = r * 1000 * 1000;
          }
        else if (trailer == "MiB")
          {
            // MebiByte
            v = r * 1024 * 1024;
          }
        else if (trailer == "GB")
          {
            // GigaByte
            v = r * 1000 * 1000 * 1000;
          }
        else if (trailer == "GiB")
          {
            // GibiByte
            v = r * 1024 * 1024 * 1024;
          }
        else
          {
            return false;
          }
        return true;
      }
    std::istringstream iss;
    iss.str (s);
    iss >> v;
    return true;
  }
};

class Time
{
public:
  static std::string
  GetCurrTimeStr (const char *fmt)
  {
    std::stringstream ss;
    auto currTime = time (NULL);
    ss << std::put_time (std::localtime (&currTime), fmt);
    return ss.str ();
  }
};

} // namespace cyq

#endif /* CYQ_UTILS_H */
