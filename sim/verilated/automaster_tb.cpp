////////////////////////////////////////////////////////////////////////////////
//
// Filename:	automaster_tb.cpp
//
// Project:	ICO Zip, iCE40 ZipCPU demonsrtation project
//
// Purpose:	This file calls and accesses the main.v function via the
//		MAIN_TB class found in main_tb.cpp.  When put together with
//	the other components here, this file will simulate (all of) the
//	host's interaction with the FPGA circuit board.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2015-2017, Gisselquist Technology, LLC
//
// This program is free software (firmware): you can redistribute it and/or
// modify it under the terms of  the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
//
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
////////////////////////////////////////////////////////////////////////////////
//
//
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

#include "verilated.h"
#include "Vmain.h"
#include "design.h"
#include "cpudefs.h"

#include "testb.h"
// #include "twoc.h"

#include "port.h"
#include "zipelf.h"

#define	BASECLASS	Vmain
#include "main_tb.cpp"

void	usage(void) {
	fprintf(stderr, "USAGE: main_tb [zipcpu-elf-file]\n");
}

int	main(int argc, char **argv) {
	const	char *elfload = NULL,
#ifdef	SDSPI_ACCESS
			*sdimage_file = NULL,
#endif
			*trace_file = NULL; // "trace.vcd";
	bool	debug_flag = false, willexit = false;
//	int	fpga_port = FPGAPORT, serial_port = -(FPGAPORT+1);
//	int	copy_comms_to_stdout = -1;
	Verilated::commandArgs(argc, argv);
	MAINTB	*tb = new MAINTB;

	for(int argn=1; argn < argc; argn++) {
		if (argv[argn][0] == '-') for(int j=1;
					(j<512)&&(argv[argn][j]);j++) {
			switch(tolower(argv[argn][j])) {
#ifdef	SDSPI_ACCESS
			case 'c': sdimage_file = argv[++argn]; j = 1000; break;
#endif
			case 'd': debug_flag = true;
				if (trace_file == NULL)
					trace_file = "trace.vcd";
				break;
			// case 'p': fpga_port = atoi(argv[++argn]); j=1000; break;
			// case 's': serial_port=atoi(argv[++argn]); j=1000; break;
			case 't': trace_file = argv[++argn]; j=1000; break;
			case 'h': usage(); break;
			default:
				fprintf(stderr, "ERR: Unexpected flag, -%c\n\n",
					argv[argn][j]);
				usage();
				break;
			}
		} else if (iself(argv[argn])) {
			elfload = argv[argn];
#ifdef	SDSPI_ACCESS
		} else if (0 == access(argv[argn], R_OK)) {
			sdimage_file = argv[argn];
#endif
		} else {
			fprintf(stderr, "ERR: Cannot read %s\n", argv[argn]);
			perror("O/S Err:");
			exit(EXIT_FAILURE);
		}
	}

	if (elfload) {
		/*
		if (serial_port < 0)
			serial_port = 0;
		if (copy_comms_to_stdout < 0)
			copy_comms_to_stdout = 0;
		tb = new TESTBENCH(fpga_port, serial_port,
			(copy_comms_to_stdout)?true:false, debug_flag);
		*/
		willexit = true;
	} else {
		/*
		if (serial_port < 0)
			serial_port = -serial_port;
		if (copy_comms_to_stdout < 0)
			copy_comms_to_stdout = 1;
		tb = new TESTBENCH(fpga_port, serial_port,
			(copy_comms_to_stdout)?true:false, debug_flag);
		*/
	}

	if (debug_flag) {
		printf("Opening Bus-master with\n");
		printf("\tDebug Access port = %d\n", FPGAPORT); // fpga_port);
		printf("\tSerial Console    = %d\n", FPGAPORT+1);
			// (serial_port == 0) ? " (Standard output)" : "");
		/*
		printf("\tDebug comms will%s be copied to the standard output%s.",
			(copy_comms_to_stdout)?"":" not",
			((copy_comms_to_stdout)&&(serial_port == 0))
			? " as well":"");
		*/
		printf("\tVCD File         = %s\n", trace_file);
	} if (trace_file)
		tb->opentrace(trace_file);

	tb->reset();
#ifdef	SDSPI_ACCESS
	tb->setsdcard(sdimage_file);
#endif

	if (elfload) {
		uint32_t	entry;
		ELFSECTION	**secpp = NULL, *secp;
		elfread(elfload, entry, secpp);

		if (secpp) for(int i=0; secpp[i]->m_len; i++) {
			secp = secpp[i];
			tb->load(secp->m_start, secp->m_data, secp->m_len);
		}

		tb->m_core->cpu_ipc = entry;
		tb->tick();
		tb->m_core->cpu_ipc = entry;
		tb->m_core->cpu_cmd_halt = 0;
		tb->tick();
	}

	if (willexit) {
		while(!tb->done())
			tb->tick();
	} else
		while(true)
			tb->tick();

	tb->close();
	delete tb;

	return	EXIT_SUCCESS;
}

