//============================================================================================================
//!
//! \file main.c
//!
//! \brief Program for converting NCom files to text. Also demonstrates how to use the C NCom libraries.
//!
//============================================================================================================

#define MAIN_DEV_ID "111027"  //!< Development identification.

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "NComRxC.h"


//============================================================================================================
// Prototypes for some helper functions.

static void report(const NComRxC *nrx);
static void print(FILE *fp, const NComRxC *nrx);


//============================================================================================================
//! \brief Function where the program starts execution.
//!
//! Program for converting NCom files to text.
//!
//! \return Exit status.

int main(
		int argc,           //!< Number of arguments supplied to the program.
		const char *argv[]  //!< Array of arguments supplied to the program.
	)
{
	FILE *fpin   = NULL;  // input file
	FILE *fpout  = NULL;  // output file
	FILE *fptrig = NULL;  // optional trigger file

	int c;                // char from input file
	NComRxC *nrx;         // NComRxC object

	// Output the header and the Development ID
	printf("NComC_file: Converts NCom file data to text. (ID: " MAIN_DEV_ID ")\n");

	// Check the command line for 2 or 3 user parameters
	if(argc !=3 && argc != 4)
	{
		fprintf(stderr, "Usage: NComC_file <input file> <output file> [<trig_file>]\n");
		exit(EXIT_FAILURE);
	}

	// Open the input file
	fpin = fopen(argv[1], "rb");
	if(fpin == NULL)
	{
		fprintf(stderr, "Error: Could not open input file '%s'.\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// Open the output file
	fpout = fopen(argv[2], "wt");
	if(fpout == NULL)
	{
		fprintf(stderr, "Error: Could not open output file '%s'.\n", argv[2]);
		if(fpin != NULL) fclose(fpin);
		exit(EXIT_FAILURE);
	}

	// Open the (optional) output trigger text file
	if(argc >= 4)
	{
		fptrig = fopen(argv[3], "wt");
		if(fptrig == NULL)
		{
			fprintf(stderr, "Error: Could not open output trigger file '%s'.\n", argv[3]);
			if(fpin  != NULL) fclose(fpin);
			if(fpout != NULL) fclose(fpout);
			exit(EXIT_FAILURE);
		}
	}

	// Create NCom decoder and check
	nrx = NComCreateNComRxC();
	if(nrx == NULL)
	{
		fprintf(stderr, "Error: Unable to create NCom decoder.\n");
		if(fpin   != NULL) fclose(fpin);
		if(fpout  != NULL) fclose(fpout);
		if(fptrig != NULL) fclose(fptrig);
		exit(EXIT_FAILURE);
	}

	// Read all of the input file and convert to text
	while((c = fgetc(fpin)) != EOF)
	{
		// Decode the data
		if(NComNewChar(nrx, (unsigned char) c) == COM_NEW_UPDATE)
		{
			// For regular updates then output to main output file, otherwise,
			// for falling edge input triggers then output to trigger file.
			switch(nrx->mOutputPacketType)
			{
				case OUTPUT_PACKET_REGULAR : print(fpout,  nrx); break;
				case OUTPUT_PACKET_IN1DOWN : print(fptrig, nrx); break;
				default : break;
			}
		}

		// Report some statistics every 4096 chars processed
		if( (NComNumChars(nrx) & UINT32_C(0xFFF)) == 0) report(nrx);
	}

	// Report final statistics
	report(nrx);
	printf("\n");

	// Clean up
	if(fpin   != NULL) fclose(fpin);
	if(fpout  != NULL) fclose(fpout);
	if(fptrig != NULL) fclose(fptrig);
	NComDestroyNComRxC(nrx);

	return EXIT_SUCCESS;
}


//============================================================================================================
//! \brief Simple decoding progress report.

static void report(const NComRxC *nrx)
{
	assert(nrx);

	printf("\rChars Read %" PRIu32 ", Packets Read %" PRIu32 ", Chars Skipped %" PRIu32,
		NComNumChars(nrx), NComNumPackets(nrx), NComSkippedChars(nrx));

	fflush(stdout);
}


//============================================================================================================
//! \brief Used to write some of the NCom data to a file pointer.
//!
//! There are only a few examples here of how to use the data values.

static void print(FILE *fp, const NComRxC *nrx)
{
	assert(fp);
	assert(nrx);

	// Print the time.
	if (nrx->mIsTimeValid)
	{
		double     gps2machine, mMachineTime;
		time_t     t1;
		struct tm *td;
		int        ms;

		// Convert GPS seconds (from 1980-01-06 00:00:00) to machine seconds (from 1970-01-01 00:00:00). It is
		// very likely the machine will adjust for leap seconds, hence the correct GPS UTC difference is
		// applied. If the local machine time does not start from 1970-01-01 00:00:00 then the value of
		// gps2machine below needs to change.
		gps2machine  = 315964800.0;
		mMachineTime = nrx->mTime + gps2machine + nrx->mTimeUtcOffset;

		// Compute local time
		t1 = (time_t) floor(mMachineTime);
		td = localtime(&t1);
		ms = floor(0.5 + (mMachineTime - t1) * 1000.0);
		if(ms < 0) ms = 0; else if(ms > 999) ms = 999;

		// Print: GPS time, local date, time zone.
		fprintf(fp, "%10.3f,%04d-%02d-%02d %02d:%02d:%02d.%03d,",
			nrx->mTime,
			1900+td->tm_year, 1+td->tm_mon, td->tm_mday, td->tm_hour, td->tm_min, td->tm_sec, ms);
	}
	else
	{
		fprintf(fp, ",,");
	}

	// Print the latitude.
	if(nrx->mIsLatValid) fprintf(fp, "%.8f", nrx->mLat);
	fprintf(fp, ",");

	// Print the longitude.
	if(nrx->mIsLonValid) fprintf(fp, "%.8f", nrx->mLon);
	fprintf(fp, ",");

	// Print the horizontal distance travelled.
	if(nrx->mIsDist2dValid) fprintf(fp, "%.3f", nrx->mDist2d);

	fprintf(fp, "\n");
}
