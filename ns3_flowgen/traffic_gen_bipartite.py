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
        type="string",
    )
    parser.add_option(
        "--seed",
        dest="seed",
        help="random seed",
        default=0,
        type="int",
    )
    parser.add_option(
        "-l",
        "--load",
        dest="load",
        help="the percentage of the traffic load to the network capacity, by default 0.3",
        default=0.3,
        type="float",
    )
    parser.add_option(
        "-b",
        "--bandwidth",
        dest="bandwidth",
        help="the bandwidth of host link (G/M/K), by default 10G",
        default="10G",
        type="string",
    )
    parser.add_option(
        "-t",
        "--time",
        dest="time",
        help="the total run time (s), by default 10",
        default=10,
        type="float",
    )
    parser.add_option(
        "--sport",
        dest="s_port",
        help="source port of all flows, by default 1",
        default=1,
        type="int",
    )
    parser.add_option(
        "-o",
        "--output",
        dest="output",
        help="the output file",
        default="tmp_traffic.csv",
        type="string",
    )
    parser.add_option(
        "--bipartite",
        dest="bipartite",
        help="bipartite configuration with "
        "1. start index, 2. end index of part one; "
        "3. start index, 4. end index of part two",
        nargs=4,
        type="int",
    )
    options, args = parser.parse_args()

    # global members
    base_time = 1e9  # ns
    seed = options.seed
    cdf_rand = CdfRand(seed)
    possion_rand = PossionRand(seed)
    host_rand = random.Random(seed)

    # read options
    if not options.bipartite:
        print("Error: please use --bipartite to configure bipartite topology")
        sys.exit(0)
    start_index_1 = options.bipartite[0]
    end_index_1 = options.bipartite[1]
    start_index_2 = options.bipartite[2]
    end_index_2 = options.bipartite[3]
    # check bipartite overlap
    if not (
        start_index_1 <= end_index_1
        and end_index_1 < start_index_2
        and start_index_2 <= end_index_2
    ):
        print("Error: Bipartite overlap")
        sys.exit(0)
    load = options.load  # traffic load
    bandwidth = get_bps_from_str(options.bandwidth)
    time = options.time * 1e9  # convert to ns
    s_port = options.s_port
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
    host_list_1 = {
        i for i in range(start_index_1, end_index_1 + 1)
    }  # host id of part 1
    host_list_2 = {
        i for i in range(start_index_2, end_index_2 + 1)
    }  # host id of part 1
    all_host = [
        (base_time + int(possion_rand.get_rand(avg_interval_arrival)), i)
        for i in range(start_index_1, end_index_1 + 1)
    ] 
    # + [
    #     (base_time + int(possion_rand.get_rand(avg_interval_arrival)), i)
    #     for i in range(start_index_2, end_index_2 + 1)
    # ]  # init host with (first sending time, host id)
    heapq.heapify(all_host)
    flow_dict = dict()
    while len(all_host) > 0:
        start_time, src_host_id = all_host[0]
        next_time = start_time + int(possion_rand.get_rand(avg_interval_arrival))
        dst_host_id = (
            host_rand.randint(start_index_2, end_index_2)
            # if src_host_id in host_list_1
            # else host_rand.randint(start_index_1, end_index_1)
        )
        if next_time > time + base_time:
            heapq.heappop(all_host)
        else:
            flow_size = int(cdf_rand.get_rand())
            if flow_size <= 0:
                flow_size = 1
            if (src_host_id, dst_host_id) in flow_dict:
                flow_dict[(src_host_id, dst_host_id)] += 1
            else:
                flow_dict[(src_host_id, dst_host_id)] = 1
            # Notice that time is (s) and size is (byte) by default
            output_file.write(
                "%dns,h%d,h%d,%d,%d,%dB,%d\n"
                % (
                    start_time,
                    src_host_id,
                    dst_host_id,
                    s_port,
                    flow_dict[(src_host_id, dst_host_id)],
                    flow_size,
                    0,
                )
            )
            heapq.heapreplace(all_host, (next_time, src_host_id))
    output_file.close()
