
The Network Simulator, Version 3
================================

## Table of Contents:

- [The Network Simulator, Version 3](#the-network-simulator-version-3)
  - [Table of Contents:](#table-of-contents)
  - [An Open Source project](#an-open-source-project)
  - [Building ns-3](#building-ns-3)
  - [Running ns-3](#running-ns-3)
  - [Getting access to the ns-3 documentation](#getting-access-to-the-ns-3-documentation)
  - [Working with the development version of ns-3](#working-with-the-development-version-of-ns-3)
  - [Changes on Branch `dev-cyq` by Yanqing](#changes-on-branch-dev-cyq-by-yanqing)
    - [Add Compiler Flag for Modified Socket Tos](#add-compiler-flag-for-modified-socket-tos)
  - [Examples on Branch `dev-cyq` by Yanqing](#examples-on-branch-dev-cyq-by-yanqing)

Note:  Much more substantial information about ns-3 can be found at
http://www.nsnam.org

## An Open Source project

ns-3 is a free open source project aiming to build a discrete-event
network simulator targeted for simulation research and education.   
This is a collaborative project; we hope that
the missing pieces of the models we have not yet implemented
will be contributed by the community in an open collaboration
process.

The process of contributing to the ns-3 project varies with
the people involved, the amount of time they can invest
and the type of model they want to work on, but the current
process that the project tries to follow is described here:
http://www.nsnam.org/developers/contributing-code/

This README excerpts some details from a more extensive
tutorial that is maintained at:
http://www.nsnam.org/documentation/latest/

## Building ns-3

The code for the framework and the default models provided
by ns-3 is built as a set of libraries. User simulations
are expected to be written as simple programs that make
use of these ns-3 libraries.

To build the set of default libraries and the example
programs included in this package, you need to use the
tool 'waf'. Detailed information on how to use waf is
included in the file doc/build.txt

However, the real quick and dirty way to get started is to
type the command
```shell
./waf configure --enable-examples
```

followed by

```shell
./waf
```

in the directory which contains this README file. The files
built will be copied in the build/ directory.

The current codebase is expected to build and run on the
set of platforms listed in the [release notes](RELEASE_NOTES)
file.

Other platforms may or may not work: we welcome patches to
improve the portability of the code to these other platforms.

## Running ns-3

On recent Linux systems, once you have built ns-3 (with examples
enabled), it should be easy to run the sample programs with the
following command, such as:

```shell
./waf --run simple-global-routing
```

That program should generate a `simple-global-routing.tr` text
trace file and a set of `simple-global-routing-xx-xx.pcap` binary
pcap trace files, which can be read by `tcpdump -tt -r filename.pcap`
The program source can be found in the examples/routing directory.

## Getting access to the ns-3 documentation

Once you have verified that your build of ns-3 works by running
the simple-point-to-point example as outlined in 3) above, it is
quite likely that you will want to get started on reading
some ns-3 documentation.

All of that documentation should always be available from
the ns-3 website: http:://www.nsnam.org/documentation/.

This documentation includes:

  - a tutorial

  - a reference manual

  - models in the ns-3 model library

  - a wiki for user-contributed tips: http://www.nsnam.org/wiki/

  - API documentation generated using doxygen: this is
    a reference manual, most likely not very well suited
    as introductory text:
    http://www.nsnam.org/doxygen/index.html

## Working with the development version of ns-3

If you want to download and use the development version of ns-3, you
need to use the tool `git`. A quick and dirty cheat sheet is included
in the manual, but reading through the git
tutorials found in the Internet is usually a good idea if you are not
familiar with it.

If you have successfully installed git, you can get
a copy of the development version with the following command:
```shell
git clone https://gitlab.com/nsnam/ns-3-dev.git
```

However, we recommend to follow the Gitlab guidelines for starters,
that includes creating a Gitlab account, forking the ns-3-dev project
under the new account's name, and then cloning the forked repository.
You can find more information in the manual [link].

## Changes on Branch `dev-cyq` by Yanqing

Here I list some changes toke on NS3 itself due to my requirements.
These changes will take effect on this and its branches.

### Add Compiler Flag for Modified Socket Tos

From [sockets-api](https://www.nsnam.org/docs/release/3.30/models/html/sockets-api.html#priority)
we can see, the priority is forced setting to 4 different bands.
No matter what the `ns3::PrioQueueDisc::PrioMap` is,
it will only use 4 priority queues.
So I change it to be able to use all 16 priority queues as your expectation.

In the method `ns3::Socket::IpTos2Priority` of the file `socket.cc`,
if the compiler flag `CYQ_TOS` exists,
the priority of the socket is setting to high four bits.

## Examples on Branch `dev-cyq` by Yanqing

I add some examples to test the functionalities of NS3 which are not shown in the manual or document.

- ipv4-raw-socket
