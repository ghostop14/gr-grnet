# gr-grnet
GNURadio TCP and UDP source and sink blocks completely rewritten in C++ with Boost.  The UDP block has some additional header options that can be optionally enabled as well:

Header/CRC

None (raw byte stream)

64-bit sequence numbers

64-bit sequence numbers + 16-bit data size

CHDR (64-bit)



## Building
This module requires zlib for the crc32 calculations in the UDP module and libpcap for the PCAPUDPSource block.  You can install them with 'apt-get install zlib1g-dev libpcap-dev' if it's not already present.


Build is pretty standard:

mkdir build

cd build

cmake ..

make

make install

ldconfig

Also, if you plan on using the UDP source block, it is strongly recommended that you increase the kernel buffer sizes for UDP packets.  You can do this by adding the following lines to /etc/sysctl.conf and rebooting:

net.core.rmem_default=26214400

net.core.rmem_max = 104857600


They can also be changed within the current running session using the following commands (however, without adding them to sysctl.conf they will not survive a reboot):

sudo sysctl -w net.core.rmem_default=26214400

sudo sysctl -w net.core.rmem_max = 104857600


## Performance
Throughput is always important.  Each was measured against a local 127.0.0.1 listener on port 2000.  The following metrics were generated (the module installs a test-netsink command-line tool that can be used to test your own performance.  
(Commas were added to the Bps output for clarity).  The results indicate that the blocks can sustain network traffic at full gigabit network throughput if necessary.

TCP Sink

Testing netsink.  Sending test data to 127.0.0.1:2000

Code Run Time:      0.000002 s  (4,093,850,624.000000 Bps)


TCP Source

TCP Source waiting for connection on 0.0.0.0:2000

TCP Source 0.0.0.0:2000 connected.

Connected.  Waiting for data...

Data received.  Processing...

Code Run Time:      0.000002 s  (3,957,239,296.000000 Bps)



UDP Sink

Testing netsink.  Sending test data to 127.0.0.1:2000

Code Run Time:      0.000002 s  (3,734,619,648.000000 Bps)


UDP Source

Waiting for connection...

Connected.  Waiting for data...

Data received.  Processing 8192 bytes

Code Run Time:      0.000002 s  (3,734,619,648.000000 Bps)


## Notes
If with the TCP Source you run into an error like this: "ICE default IO error handler doing an exit()" delete your ~/.ICEauthority file, logoff and log back in again then try again.

