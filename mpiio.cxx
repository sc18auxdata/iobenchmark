/*
 *  MPI collective I/O benchmark
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>

#include "common.h"

#define NAME_LENGTH 256

int oneKB=1024;

int rank, size;
int count, mode;

double wFS, rFS;

MPI_File fileHandle;
MPI_Status status;

char testFileName[NAME_LENGTH];

int SKIP = 5;
int MAXTIMESD = 5;

void cleanup_() ;

/*
 * * * * * * * * * Collective MPI-IO to IO nodes from all compute nodes - shared file * * * * * * * *
 */
int writeFile(dataBlock *datum, int count) 
{
  int result, nbytes;

  MPIO_Request req_iw;
  MPI_Status st_iw;

  result = MPI_File_iwrite_at_all (fileHandle, (MPI_Offset)rank*count*sizeof(double), datum->getAlphaBuffer(), count, MPI_DOUBLE, &req_iw);
  if (result != MPI_SUCCESS) 
     prnerror (result, "MPI_File_iwrite_at_all error:");
  MPI_Wait(&req_iw, &st_iw);
	MPI_Get_elements( &status, MPI_CHAR, &nbytes );

	return nbytes;
}

void file_write(dataBlock *datum) {

  int totalBytes = 0;
  double tIOStart, tIOEnd;

  MPI_File_open (MPI_COMM_WORLD, testFileName, mode, MPI_INFO_NULL, &fileHandle);

  //warm up
	for (int i=1; i<=SKIP; i++)
	   totalBytes += writeFile(datum, count);
  //write
	tIOStart = MPI_Wtime();
	for (int i=1; i<=MAXTIMESD; i++)
	   totalBytes += writeFile(datum, count);
	tIOEnd = MPI_Wtime();
  wFS = (tIOEnd - tIOStart)/MAXTIMESD;
	MPI_File_close (&fileHandle);

}

int readFile(dataBlock *datum, int count) 
{
  int result, nbytes;

	MPI_Status st_iw;
  MPIO_Request req_iw;

	result = MPI_File_iread_at_all (fileHandle, (MPI_Offset)rank*count*sizeof(double), datum->getAlphaBuffer(), count, MPI_DOUBLE, &req_iw);
	if (result != MPI_SUCCESS) 
		 prnerror (result, "MPI_File_iread_at_all error:");
	MPI_Wait(&req_iw, &st_iw);
	MPI_Get_elements( &status, MPI_CHAR, &nbytes );

	return nbytes;
}

void file_read(dataBlock *datum) {

  int totalBytes = 0;
  double tIOStart, tIOEnd;

  MPI_File_open (MPI_COMM_WORLD, testFileName, mode, MPI_INFO_NULL, &fileHandle);

  //warm up
	for (int i=1; i<=SKIP; i++)
  	 totalBytes += readFile(datum, count);
  //read
	tIOStart = MPI_Wtime();
	for (int i=1; i<=MAXTIMESD; i++)
	   totalBytes += readFile(datum, count);
	tIOEnd = MPI_Wtime();
  rFS = (tIOEnd - tIOStart)/MAXTIMESD;
	MPI_File_close (&fileHandle);

}

int main(int argc, char *argv[]) {

  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);

  sprintf(testFileName,"TestFile-%d",size);

  /* #doubles for I/O */
  count = atoi(argv[1]) * oneKB;

	/* allocate buffer */
  dataBlock *datum = new dataBlock(count);
	datum->allocElement ();

  assert (datum->getAlphaBuffer() != NULL);

  /* set file open mode */
  mode = MPI_MODE_CREATE | MPI_MODE_RDWR; 

  rFS = 0, wFS = 0;

  file_write(datum);  // write
  file_read(datum);   // read

  cleanup_();
  MPI_Finalize();

}

void cleanup_() {

  double max_rFS, max_wFS;

  /* maximum time to write and read */
	MPI_Reduce(&rFS, &max_rFS, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	MPI_Reduce(&wFS, &max_wFS, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

  if (rank == 0) 
     printf ("0: Times: %d | %6.4lf %6.4lf\n", size, max_rFS, max_wFS);

}

