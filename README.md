# gr-grnet
GNURadio TCP and UDP source and sink blocks completely rewritten in C++ with Boost.  The UDP block has some additional header/crc options that can be optionally enabled as well:

Header/CRC

None (raw byte stream)

0xFFFFFFFF sync + leading 32-bit sequence number

sync + leading 32-bit sequence number + 32-bit data size

sync + leading 32-bit sequence number + 32-bit data size + trailing crc32



## Building
This module requires zlib for the crc32 calculations in the UDP module.  You can install it with 'apt-get install zlib1g-dev' if it's not already present.


Build is pretty standard:

mkdir build

cd build

cmake ..

make

make install

ldconfig

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

