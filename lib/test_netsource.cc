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

#include <chrono>
#include <boost/algorithm/string/replace.hpp>

#include "tcp_source_impl.h"
#include "udp_source_impl.h"

using namespace gr::grnet;

int
main (int argc, char **argv)
{
	int port=2000;
	bool useTCP=true;
	int headerType=HEADERTYPE_NONE;

	if (argc > 1) {
		// 1 is the file name
		if (strcmp(argv[1],"--help")==0) {
			std::cout << std::endl;
			std::cout << "Usage: [--udp] [--port=<port>] [--header | --header-crc]" << std::endl;
			std::cout << "Writes 819200 bytes to specified host and port (default is localhost:2000).  Default is TCP unless --udp is provided." << std::endl;
			std::cout << "For udp, if --header is supplied, a 32-bit sequence number and 32-bit data size field are expected as a packet header." << std::endl;
			std::cout << "For udp, if --header-crc is supplied, the header is expected along with a trailing 32-bit CRC." << std::endl;
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

			if (param.find("--port") != std::string::npos) {
				boost::replace_all(param,"--port=","");
				port=atoi(param.c_str());
			}

		}
	}

	int vecLen=64;
	std::cout << "Testing netsource.  Listening on port " << port << std::endl;

	tcp_source_impl *testTCP=NULL;
	udp_source_impl *testUDP=NULL;

	std::cout << "Waiting for connection..." << std::endl;
	if (useTCP)
		testTCP = new tcp_source_impl(sizeof(char),vecLen,port);
	else
		testUDP = new udp_source_impl(sizeof(char),vecLen,port,HEADERTYPE_NONE, 1472, false,true);

	std::cout << "Connected.  Waiting for data..." << std::endl;
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
	int numBytes;

	char buffer[localblocksize];

	outputPointers.push_back((void *)buffer);

	if (useTCP) {

		while (testTCP->dataAvailable() < localblocksize) {
			sleep(1);
		}

		std::cout << "Data received.  Processing..." << std::endl;

		numBytes = localblocksize;

		start = std::chrono::steady_clock::now();
		for (i=0;i<iterations;i++) {
			testTCP->work_test(localblocksize/vecLen,inputPointers,outputPointers);
		}

		end = std::chrono::steady_clock::now();
	}
	else {
		while (testUDP->dataAvailable() < localblocksize) {
			sleep(1);
		}

		numBytes = testUDP->dataAvailable();

		iterations = 1;

		std::cout << "Data received.  Processing " <<  numBytes << " bytes" << std::endl;
		start = std::chrono::steady_clock::now();
		testUDP->work_test(localblocksize/vecLen,inputPointers,outputPointers);

		end = std::chrono::steady_clock::now();

		numBytes = localblocksize;
	}

	elapsed_seconds = end-start;

	elapsed_time = elapsed_seconds.count() / iterations;
	throughput_original = localblocksize / elapsed_time;

	std::cout << "Code Run Time:   " << std::fixed << std::setw(11)
    << std::setprecision(6) << elapsed_time << " s  (" << throughput_original << " Bps)" << std::endl;

	if (useTCP)
		delete testTCP;
	else
		delete testUDP;

	return 0;

}

