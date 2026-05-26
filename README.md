# Open Audio Network
Open Audio Network is a L2 protocol designed to transport audio, control, clock sync and discovery. It is designed to be 
used with standard IT equipment and technically allows N*N bidirectional audio streams over Ethernet while keeping real-time
properties with low latency and low jitter in mind.

## Features
 - Node auto-discovery and network mapping
 - Shared compute resources allocation over the network
 - Clock syncing using PTP based protocol
 - Control data transport between the nodes
 - No IP stack needed for operation. Any raw Ethernet frame capable device can integrate the protocol