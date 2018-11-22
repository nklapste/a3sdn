# a3sdn

[![Build Status](https://travis-ci.com/nklapste/a3sdn.svg?branch=master)](https://travis-ci.com/nklapste/a3sdn)

## Description

a3sdn is an expansion to the developments of [a2sdn](https://github.com/nklapste/a2sdn)
a linear software-defined network (SDN) that is composed of one 
main controller and multiple switches (sw1, sw2, … sw7). 

Some extensions a3sdn introduces is that communication between switches was 
done via named pipes (FIFOs) and a common traffic file while communication
between switches and controllers was done via TCP sockets. 

Additionally, the requirements of non-blocking I/O still apply for a3sdn.

Also the requirement for switches to disconnect and have the SDN still 
operate “effectively” was added.

