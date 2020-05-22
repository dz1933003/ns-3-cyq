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
 */

#include "ns3/core-module.h"
#include "ns3/dpsk-module.h"
#include "ns3/pfc-module.h"
#include "ns3/rdma-module.h"
#include "ns3/internet-module.h"
#include <fstream>
#include <nlohmann/json.hpp>

using namespace ns3;
using json = nlohmann::json;

NS_LOG_COMPONENT_DEFINE ("PFC HW");

int
main (int argc, char *argv[])
{
  NS_LOG_UNCOND ("PFC HW");

  if (argc != 2)
    {
      NS_LOG_UNCOND ("ERROR: No config file");
      return 1;
    }

  std::ifstream file (argv[1]);
  json conf = json::parse (file);

  const uint32_t nQueue = conf["Global"]["QueueNumber"];
  NS_LOG_UNCOND(nQueue);

  // TODO cyq: complete read configuration

  Simulator::Run ();
  Simulator::Destroy ();
}
