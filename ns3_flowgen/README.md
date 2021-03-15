# Flow Generator for Branch `dev-cyq`

The code is converted from the HPCC opensource repository.

## Usage

### All to all generator

`python traffic_gen.py -h` for help.

This script will generate flows from any nodes to any other nodes.

Example:
`python traffic_gen.py -c cdf/WebSearch_distribution.txt -n 320 -l 0.3 -b 100G -t 0.1` generates traffic according to the web search flow size distribution, for 320 hosts, at 30% network load with 100Gbps host bandwidth for 0.1s.

The generate traffic can be directly used by the simulation.

### Bipartite generator

`python traffic_gen_bipartite.py -h` for help.

This script will generate flows only from node part 1 to node part 2 or backward.

`./traffic_gen_bipartite.py -c cdf/WebSearch_distribution.txt -l 0.3 -t 0.1 --bipartite 0 10 11 20 -b 100G` generates traffic between part 1 (node 0 to node 10) and part 2 (node 11 to node 20).

## Flow size distributions
We provide 4 distributions. `WebSearch_distribution.txt` and `FbHdp_distribution.txt` are the ones used in the HPCC paper. `AliStorage2019.txt` are collected from Alibaba's production distributed storage system in 2019. `GoogleRPC2008.txt` are Google's RPC size distribution before 2008.
