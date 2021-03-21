#!/usr/bin/python3

from subprocess import Popen

Popen(
    './waf --run "scratch/pfc-cyq ns3_config/irn/interDC/inter-dc-config.json"',
    shell=True,
)
