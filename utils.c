/*
 * utils.c
 *
 *  Created on: 10 Nov 2014
 *      Author: marcin
 */
#include "utils.h"

int readFile(const char* path, char** fileContent, struct stat* fileStat ) {
	int myFile = open(path, O_RDONLY);
	if (myFile < 0) {
		syslog(LOG_ERR, "Cannot open taskfile.");
		exit(0);
	}
	if((fstat(myFile, fileStat)) == -1) {	// wczytaj informacje o pliku i zapisz do struktury
		syslog(LOG_ERR, "Cannot fetch information about taskfile.");
		exit(1);
	}

	// alokacja miejsca na zawartosc Pliku (tyle bajtow ile ma plik)
	*fileContent = (char*)malloc(fileStat->st_size);
	if (fileContent == NULL) {
		syslog(LOG_ERR, "Malloc error: errno %d.", errno );
	}

	// wczytywanie zawartosci
	if ((read(myFile, *fileContent, fileStat->st_size)) == -1) {
		syslog(LOG_ERR, "Cannot read taskfile: errno %d.", errno );
		exit(1);
	}

	close(myFile);
	return 0;

}

StructTime* currentTime() {
	 	 time_t CurrentTime;
	        time(&CurrentTime);
	        char value[3];
	        value[2] = '\0';
	        int hour,minute;
	        char* rezultat=ctime(&CurrentTime);
	        value[0]=rezultat[11];
	        value[1]=rezultat[12];
	        sscanf(value,"%d",&hour);
	        value[0]=rezultat[14];
	        value[1]=rezultat[15];
	        sscanf(value,"%d",&minute);

	        StructTime *StructureWithTime = (StructTime*)malloc(sizeof(StructTime));
	        if (!StructureWithTime) {
	        	syslog(LOG_ERR, "Cannot malloc real time structure");
	        	exit(1);
	        }
	        StructureWithTime->hour=hour;
	        StructureWithTime->minute=minute;

	return StructureWithTime;
}
