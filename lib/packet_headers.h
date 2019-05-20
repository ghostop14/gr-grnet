/*
 * packet_headers.h
 *
 *  Created on: May 20, 2019
 *      Author: mpiscopo
 */

#ifndef LIB_PACKET_HEADERS_H_
#define LIB_PACKET_HEADERS_H_

#include "udpHeaderTypes.h"

class HeaderSeqNum {
public:
	// size: 8 (64-bit)
	uint64_t seqnum;

	HeaderSeqNum() {seqnum=0;};
};

class HeaderSeqPlusSize {
public:
	// size: 10 (80-bit)
	uint64_t seqnum;
	int16_t length;

	HeaderSeqPlusSize() {seqnum=0;length=0;};
};

// CHDR Definition: https://files.ettus.com/manual/page_rtp.html
/*

Bits 	Meaning
63:62 	Packet Type
61 	Has fractional time stamp (1: Yes)
60 	End-of-burst or error flag
59:48 	12-bit sequence number
47:32 	Total packet length in Bytes
31:0 	Stream ID (SID). For the format of SID, see uhd::sid_t.



Bit 63 	Bit 62 	Bit 61 	Bit 60 	Packet Type
0 	0 	x 	0 	Data
0 	0 	x 	1 	Data (End-of-burst)
0 	1 	x 	0 	Flow Control
1 	0 	x 	0 	Command Packet
1 	1 	x 	0 	Command Response
1 	1 	x 	1 	Command Response (Error)

 */

class CHDR {
public:
	// size: 8 (64-bit)
	uint32_t sid;
	uint16_t length;
	int16_t seqPlusFlags;  // first 12 bits are sequence number,

	CHDR() { sid = 0; length = 0; seqPlusFlags = 0;};

	inline int getSeqNum(void) { return (seqPlusFlags & 0x0FFF); };

	// if a timestamp is present, there's an 8-bit fractional timestamp following the header.
	inline bool hasFractionalTimestamp(void) { int16_t timeBit; timeBit = seqPlusFlags & 0x2000; if (timeBit > 0) return true; else return false; };
};

#endif /* LIB_PACKET_HEADERS_H_ */
