#!/usr/bin/python3

from subprocess import Popen

dirs = ["acrossDC", "interDC"]
conf = ["inter-dc-config", "across-dc-config"]
suffix = ["20", "40", "60", "80", "100"]
ps = []

for d in dirs:
    for c in conf:
        for i in range(20, 120, 20):
            cmd = f'./waf --run "scratch/pfc-cyq ns3_config/irn/{d}/{c}-{i}.json {i}"'
            print(cmd)
            ps.append(
                Popen(
                    cmd,
                    shell=True,
                )
            )

for p in ps:
    p.wait()

print("Done!")
