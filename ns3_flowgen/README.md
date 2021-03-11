# Flow Generator for Branch `dev-cyq`

The code is converted from the HPCC opensource repository.

## Usage

`python traffic_gen.py -h` for help.

Example:
`python traffic_gen.py -c WebSearch_distribution.txt -n 320 -l 0.3 -b 100G -t 0.1` generates traffic according to the web search flow size distribution, for 320 hosts, at 30% network load with 100Gbps host bandwidth for 0.1s.

The generate traffic can be directly used by the simulation.
