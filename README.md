# a2sdn

CMPUT 379 Assignment 2

## Description

A program that implements the transactions performed by a simple linear SDN. 
The program can be invoked as a controller using `a2sdn cont nSwitch` where `cont`
is a reserved word, and `nSwitch` specifies the number of switches in the network 
(at `mostMAXNSW=7` switches). The program can also be invoked as a switch using 
`a2sdn swi trafficFile [null|swj] [null|swk] IPlow-IPhigh`. In this form, the 
program simulates switch `swi` by processing traffic read from file `trafficFile`.
Port 1 and port 2 of `swi` are connected to switches `swj` and `swk`, respectively. 
Either, or both, of these two switches may be `null`. Switch `swi` handles traffic 
fromhosts in the IP range `[IPlow-IPhigh]`. Each IP address is `≤MAXIP(= 1000)`.
Data transmissions among the switches and the controller use FIFOs. Each FIFO 
is named `fifo-x-y` where `x=/=y`, and `x=0` (or, `y=0`) for the controller, and 
`x,y∈[1,MAXNSW]` for a switch. Thus, e.g., `sw2` sends data to the controller
on `fifo-2-0`.
