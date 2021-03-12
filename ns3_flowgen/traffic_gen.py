#!/usr/bin/python3

import sys
import random
import math
import heapq
from optparse import OptionParser
from ns3_rand import CdfRand, PossionRand


def get_bps_from_str(b):
    if b == None:
        return None
    if type(b) != str:
        return None
    if b[-1] == "G":
        return float(b[:-1]) * 1e9
    if b[-1] == "M":
        return float(b[:-1]) * 1e6
    if b[-1] == "K":
        return float(b[:-1]) * 1e3
    return float(b)


if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option(
        "-c",
        "--cdf",
        dest="cdf_file",
        help="the file of the traffic size cdf",
        default="uniform_distribution.txt",
    )
    parser.add_option("-n", "--nhost", dest="nhost", help="number of hosts")
    parser.add_option(
        "-l",
        "--load",
        dest="load",
        help="the percentage of the traffic load to the network capacity, by default 0.3",
        default="0.3",
    )
    parser.add_option(
        "-b",
        "--bandwidth",
        dest="bandwidth",
        help="the bandwidth of host link (G/M/K), by default 10G",
        default="10G",
    )
    parser.add_option(
        "-t",
        "--time",
        dest="time",
        help="the total run time (s), by default 10",
        default="10",
    )
    parser.add_option(
        "-o",
        "--output",
        dest="output",
        help="the output file",
        default="tmp_traffic.csv",
    )
    options, args = parser.parse_args()

    # global members
    base_time = 1e9  # ns
    cdf_rand = CdfRand(0)
    possion_rand = PossionRand(0)
    host_rand = random.Random(0)

    # read options
    if not options.nhost:
        print("Error: please use -n to enter number of hosts")
        sys.exit(0)
    n_host = int(options.nhost)  # host number
    load = float(options.load)  # traffic load
    bandwidth = get_bps_from_str(options.bandwidth)
    time = float(options.time) * 1e9  # convert to ns
    output = options.output
    if bandwidth == None:
        print("Error: bandwidth format incorrect")
        sys.exit(0)

    # read the cdf, save in cdf as [[x_i, cdf_i] ...]
    cdf_filepath = options.cdf_file
    cdf_file = open(cdf_filepath, "r")
    lines = cdf_file.readlines()
    cdf = []
    for line in lines:
        x, y = map(float, line.strip().split(" "))
        cdf.append([x, y])

    # create a custom random generator, which takes a cdf, and generate number according to the cdf
    if not cdf_rand.set_cdf(cdf):
        print("Error: not valid cdf")
        sys.exit(0)

    # generate flows
    output_file = open(output, "w")
    output_file.write(
        "StartTime,FromNode,ToNode,SourcePort,DestinationPort,Size,Priority\n"
    )
    avg = cdf_rand.get_avg()
    avg_interval_arrival = 1 / (bandwidth * load / 8.0 / avg) * 1e9  # ns
    host_list = [
        (base_time + int(possion_rand.get_rand(avg_interval_arrival)), i)
        for i in range(1,n_host)
    ]  # init host with (first sending time, host id)
    heapq.heapify(host_list)
    flow_dict = dict()
    while len(host_list) > 0:
        start_time, src_host_id = host_list[0]
        next_time = start_time + int(possion_rand.get_rand(avg_interval_arrival))
        dst_host_id = host_rand.randint(1, n_host)
        while dst_host_id == src_host_id:
            dst_host_id = host_rand.randint(1, n_host)
        if next_time > time + base_time:
            heapq.heappop(host_list)
        else:
            flow_size = int(cdf_rand.get_rand())
            if flow_size <= 0:
                flow_size = 1
            if (src_host_id, dst_host_id) in flow_dict:
                flow_dict[(src_host_id, dst_host_id)] += 1
            else:
                flow_dict[(src_host_id, dst_host_id)] = 1
            # Notice that time is (s) and size is (byte) by default
            src_host = "h" + str(src_host_id)
            dst_host = "h" + str(dst_host_id)
            output_file.write(
                "%dns,%s,%s,%d,%d,%dB,%d\n"
                % (
                    start_time,
                    src_host,
                    dst_host,
                    1,
                    flow_dict[(src_host_id, dst_host_id)],
                    flow_size,
                    0,
                )
            )
            heapq.heapreplace(host_list, (next_time, src_host_id))
    output_file.close()