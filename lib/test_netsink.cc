/*
 * test_netsink.cc
 *
 *  Created on: Apr 7, 2017
 *      Author: ghostop14
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include <gnuradio/unittests.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <chrono>
#include <boost/algorithm/string/replace.hpp>

#include "tcp_sink_impl.h"
#include "udp_sink_impl.h"

using namespace gr::grnet;

// For comma-separated output (#include <iomanip> and the local along with this class takes care of it

class comma_numpunct : public std::numpunct<char>
{
  protected:
    virtual char do_thousands_sep() const
    {
        return ',';
    }

    virtual std::string do_grouping() const
    {
        return "\03";
    }
};


int
main (int argc, char **argv)
{
	std::string host = "127.0.0.1";
	int port=2000;
	int headerType=HEADERTYPE_NONE;
	bool useTCP=true;

	if (argc > 1) {
		// 1 is the file name
		if (strcmp(argv[1],"--help")==0) {
			std::cout << std::endl;
			std::cout << "Usage: [--udp] [--host=<ip>] [--port=<port>] [--header | --header-crc]" << std::endl;
			std::cout << "Writes 819200 bytes to specified host and port (default is localhost:2000).  Default is TCP unless --udp is provided." << std::endl;
			std::cout << "The default data is the pattern string 0123456789 repeating.  The resulting throughput is then timed and shown." << std::endl;
			std::cout << "For udp, if --header is supplied, a 32-bit sequence number and 32-bit data size field are sent as a packet header." << std::endl;
			std::cout << "For udp, if --header-crc is supplied, the header is sent along with a trailing 32-bit CRC." << std::endl;
			std::cout << std::endl;
			exit(0);
		}


		for (int i=1;i<argc;i++) {
			std::string param = argv[i];

			if (strcmp(argv[i],"--header")==0) {
				headerType=HEADERTYPE_SEQPLUSSIZE;
			}
			else if (strcmp(argv[i],"--header-crc")==0) {
				headerType=HEADERTYPE_SEQSIZECRC;
			}

			if (strcmp(argv[i],"--udp")==0) {
				useTCP=false;
			}

			if (param.find("--host") != std::string::npos) {
				host=param;
				boost::replace_all(host,"--host=","");
			}

			if (param.find("--port") != std::string::npos) {
				boost::replace_all(param,"--port=","");
				port=atoi(param.c_str());
			}

		}
	}

	int vecLen=64;
	std::cout << "Testing netsink.  Sending test data to " << host << ":" << port << std::endl;

	// Add comma's to numbers
	 std::locale comma_locale(std::locale(), new comma_numpunct());

	    // tell cout to use our new locale.
	 std::cout.imbue(comma_locale);

	tcp_sink_impl *testTCP=NULL;
	udp_sink_impl *testUDP=NULL;

	if (useTCP)
		testTCP = new tcp_sink_impl(sizeof(char),vecLen,host, port);
	else
		testUDP = new udp_sink_impl(sizeof(char),vecLen,host, port,headerType);

	int i;
	int iterations=100;
	std::chrono::time_point<std::chrono::steady_clock> start, end;
	std::chrono::duration<double> elapsed_seconds;
	float elapsed_time,throughput_original,throughput;
	std::vector<const void *> inputPointers;
	std::vector<void *> outputPointers;

	std::string testString="";
	int counter=0;
	int localblocksize=8192;

	for (int i=0;i<localblocksize;i++) {
		testString += std::to_string(counter);
		counter++;

		if (counter > 9)
			counter = 0;
	}

	inputPointers.push_back(testString.c_str());

	if (useTCP) {
		start = std::chrono::steady_clock::now();
		for (i=0;i<iterations;i++) {
			testTCP->work_test(localblocksize/vecLen,inputPointers,outputPointers);
		}

		end = std::chrono::steady_clock::now();
	}
	else {
		start = std::chrono::steady_clock::now();
		for (i=0;i<iterations;i++) {
			testUDP->work_test(localblocksize/vecLen,inputPointers,outputPointers);
		}

		end = std::chrono::steady_clock::now();
	}

	elapsed_seconds = end-start;

	elapsed_time = elapsed_seconds.count() / iterations;
	throughput_original = localblocksize / elapsed_time;

	std::cout << "Code Run Time:   " << std::fixed << std::setw(11)
    << std::setprecision(2) << elapsed_time << " s  (" << throughput_original << " Bps, " << (throughput_original*8)<< " bps)" << std::endl;

	if (useTCP)
		delete testTCP;
	else
		delete testUDP;

	return 0;

}

