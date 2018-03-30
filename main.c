/*
 * main.c
 *
 *  Created on: 10 Nov 2014
 *      Author: marcin
 */
#include "utils.h"
#include "parser.h"
#include "init.h"

/* 	argc = 0 - minicron
	argc = 1 - taskfile
	argc = 2 - outfile */
void sigIntHandler(int signum);
void sigUsr1Handler(int singum);
void sigUsr2Handler(int signum);
volatile sig_atomic_t flag =-1;

int main(int argc, const char** argv) {

	skeleton_daemon();	// inicjalizacja daemona

	signal(SIGINT, sigIntHandler);
	signal(SIGUSR1, sigUsr1Handler);
	signal(SIGUSR2, sigUsr2Handler);

	// wczytujemy plik taskfile
	struct stat* taskFileStat = (struct stat*)malloc(sizeof(struct stat));	// przyszle informacje o pliku taskfile, jego rozmiar
	char* taskFileContent = NULL;	// przyszla zawartosc pliku taskfile
	readFile(argv[1], &taskFileContent, taskFileStat);	// alokuj miejsce i wczytaj zawartosc pliku taskfile

	// Otwieramy outfile
	int outFile = 0;
	if ((outFile = open(argv[2], O_WRONLY|O_CREAT|O_APPEND, 0755)) == -1) {
		syslog(LOG_ERR, "Cannot open outfile.");
		exit(EXIT_FAILURE);
	}
	// czyscimy outfile z poprzednich zapisow
	ftruncate(outFile,(off_t)0);

	// tworzymy liste komend i ja uzupelniamy
	Command *commandList = NULL;
	parseAndAddToList(taskFileContent, taskFileStat->st_size, &commandList); // parsowanie zawartosci pliku taskfile do listy
	if (commandList == NULL) {
		syslog(LOG_ERR, "No command list" );
		exit (EXIT_FAILURE);
	}

	// sortujemy ta liste ze wzgledu na godzine i minute
	commandList = sortCommandList(commandList);

	// zapisz rzeczywisty czas
	struct StructTime *realTime = currentTime();
	//syslog(LOG_NOTICE,"hour: %d, minute: %d ",realTime->hour, realTime->minute);
	int parsedRealTime = ((realTime->hour) *3600) + ((realTime->minute) * 60);	// rzeczywisty czas w sekundach
	//syslog(LOG_NOTICE,"parsed real time: %d ",parsedRealTime);

	syslog(LOG_NOTICE ,"Minicron daemon started work");

	// the big loop
	while (1) {
		if (flag == 0) { 	// SIGINT
			// free
			close(outFile);
			free(taskFileStat);
			free(taskFileContent);
			freeList(commandList); // lista zostanie zwolniona w czasie pracy
			free(realTime);
			syslog(LOG_NOTICE, "Terminate request, SIGINT handled." );
			exit(EXIT_SUCCESS);
		}

		if (flag == 1) {	//wczytaj ponownie taskfile
			// free
			syslog(LOG_NOTICE ,"Minicron daemon will restart work with new taskfile");
			free(taskFileStat);
			free(taskFileContent);
			freeList(commandList);
			close(outFile);

			// ponownie pobierz czas
			realTime = currentTime();
			parsedRealTime = ((realTime->hour) *3600) + ((realTime->minute) * 60);

				// wczytujemy plik

				taskFileStat = (struct stat*)malloc(sizeof(struct stat));
				taskFileContent = NULL;
				readFile(argv[1], &taskFileContent, taskFileStat);
				// Otwieramy outfile
				if ((outFile = open(argv[2], O_WRONLY|O_CREAT|O_APPEND, 0755)) == -1) {
					syslog(LOG_ERR, "Cannot open outfile.");
					exit(EXIT_FAILURE);
				}
				// nie czyscimy outfile z poprzednich zapisow

				// tworzymy liste komend i ja uzupelniamy
				commandList = NULL;
				parseAndAddToList(taskFileContent, taskFileStat->st_size, &commandList);
				if (commandList == NULL) {
					syslog(LOG_ERR, "No command list" );
					exit (EXIT_FAILURE);
				}

				// sortujemy ta liste ze wzgledu na godzine i minute
				commandList = sortCommandList(commandList);
				flag = -1;
				parsedRealTime = ((realTime->hour) *3600) + ((realTime->minute) * 60);
		}
		if (flag == 2){
			printCommandList(&commandList);
			flag = -1;
		}

		if(commandList == NULL) {
					syslog(LOG_NOTICE, "No other commands in taskfile to do. Daemon will finish active commands and stop working.");
					close(outFile);
					free(taskFileStat);
					free(taskFileContent);
					free(realTime);
					exit (EXIT_SUCCESS);
		}

		// Sprawdz czas wykonania elementu i spij
		int parsedCommandTime =  ((commandList->hour) *3600) + ((commandList->minute) * 60);	// czas wykonania komendy w sekundach
		int sleepingTime = parsedCommandTime - parsedRealTime;	// czas spania to czas nastepnej komendy - czas rzeczywisty

		if (sleepingTime < 0) {	// jesli roznica mniejsza niz zero, komenda sie nie wykona
			syslog(LOG_NOTICE, "Too late to exec command - %d:%d:%s:%d",
					    						commandList->hour,
					    						commandList->minute,
					    						commandList->commandContent,
					    						commandList->info);
			deleteFirst(&commandList); // usun pierwszy i przejdz do nastepnego elementu listy
			continue;
		}

		// SPIJ
		//syslog(LOG_NOTICE,"spie sekund: %d ", sleepingTime);
		sleep(sleepingTime);
		if (flag == 1 || flag == 2) continue;
		parsedRealTime = parsedCommandTime;	// rzeczywisty czas to czas w ktorym wykona sie zadana komenda

		// pipe do przechwycenia wyjscia programu
		pid_t commandPid = 0;
		int outfd[2];
		int infd[2];
		int inErrfd[2];

		pipe(outfd); // where the parent is goint to write to
		pipe(inErrfd);	// where the parent is going to read - children stderr
		pipe(infd); // from where parent is going to read	[1] - for write, [0] - for read

		// WYKONAJ BIEZACY ELEMENT
		// FORK
		if (flag == 0 || flag == 1) continue;
		commandPid = fork();
		    if (commandPid < 0) {
		    	syslog(LOG_NOTICE,"Cannot fork process");
		    	exit(EXIT_FAILURE);
		    }
		    else if (commandPid == 0) {	// jestesmy w procesie potomnym
		    	if((setpgid(commandPid,commandPid)==-1)) {
		    		syslog(LOG_ERR, "Cannot create new group for process");
		    		exit(EXIT_FAILURE);
		    	}


		    	close(STDOUT_FILENO);
		    	close(STDIN_FILENO);
		    	close(STDERR_FILENO);
		    	close(outFile);
		    	closelog();

		    	dup2(infd[1], STDOUT_FILENO);
		    	dup2(inErrfd[1], STDERR_FILENO);
		    	dup2(outfd[0], STDIN_FILENO);

		    	close(outfd[0]);	// not required for the child
		    	close(outfd[1]);
		    	close(infd[0]);
		    	close(infd[1]);
		    	close(inErrfd[0]);
		    	close(inErrfd[1]);

		    	syslog(LOG_NOTICE, "Doing command - %d:%d:%s:%d",
		    						commandList->hour,
		    						commandList->minute,
		    						commandList->commandContent,
		    						commandList->info);
		    	// EXEC - przypadl by sie fork i waitpid() po to aby wiedziec czy proces sie wykonal
		    	execv(commandList->parsedCommandContent[0], commandList->parsedCommandContent);
		    	exit(EXIT_SUCCESS);
		    }
		    else {	// jestesmy w procesie nadrzednym
		    	char children_output[PIPE_BUF];	// rozmiar stdout wyjscia to rozmiar bufora potoku
		    	char children_error_output[PIPE_BUF];

		    	close(outfd[0]); // these are being used by child
		    	close(infd[1]);
		    	close(inErrfd[1]);

		    	children_output[read(infd[0],children_output,PIPE_BUF) - 1] = '\0'; /* Read from child’s stdout */
		    	if (children_output == NULL) {
		    		syslog(LOG_ERR, "Children STDOUT reading ERROR!");
		    		exit(EXIT_FAILURE);
		    	}

		    	children_error_output[read(inErrfd[0],children_error_output,PIPE_BUF)] = '\0'; /* Read from child’s stderr */
		    	if (children_output == NULL) {
		    		syslog(LOG_ERR, "Children STDERR reading ERROR!");
		    		exit(EXIT_FAILURE);
		    	}


		    	switch (commandList->info) {
		    	case 0: //użytkownik chce otrzymać treść, jaką polecenie wypisało na standardowe wyjście (stdout)
		    		//syslog(LOG_ERR, "WYPISUJE stdout: %s", children_output);
		    		dprintf(outFile, "%d:%d:%s:%d STDOUT: %s\n",
		    				commandList->hour,
							commandList->minute,
							commandList->commandContent,
							commandList->info,
		    				children_output);
		    		break;

		    	case 1: //użytkownik chce otrzymać treść, jaką polecenie wypisało na wyjście błędów (stderr).
		    		//syslog(LOG_ERR, "WYPISUJE stderr: %s",children_error_output);
		    		dprintf(outFile, "%d:%d:%s:%d STDERR: %s\n",
		    				commandList->hour,
							commandList->minute,
							commandList->commandContent,
							commandList->info,
							children_error_output);
		    		break;

		    	case 2: //użytkownik chce otrzymać treść, jaką polecenie wypisało na standardowe wyjście i wyjście błędów.
		    		//syslog(LOG_ERR, "WYPISUJE stdout: %s\n i stderr: %s\n",children_output, children_error_output);
		    		dprintf(outFile, "%d:%d:%s:%d STDOUT: %s\n%d:%d:%s:%d STDERR: %s\n",
		    				    				commandList->hour,
		    									commandList->minute,
		    									commandList->commandContent,
		    									commandList->info,
												children_output,
												commandList->hour,
												commandList->minute,
												commandList->commandContent,
												commandList->info,
		    									children_error_output);
		    		break;

		    	default:
		    		syslog(LOG_ERR, "Wrong info parametr");
		    		exit(EXIT_FAILURE);
		    	}
		    	// zamykamy deskryptory i usuwamy wykonany element z listy
		    	close(outfd[1]);
		    	close(inErrfd[0]);
		    	close(infd[0]);
		    	deleteFirst(&commandList);
		    }
		}
	close(outFile);
	free(taskFileStat);
	free(taskFileContent);
	freeList(commandList); // lista zostanie zwolniona w czasie pracy
	free(realTime);

	exit( EXIT_SUCCESS);
}

void sigIntHandler(int signum) {
	flag = 0;
}

void sigUsr1Handler(int singum) {
	flag = 1;
}
void sigUsr2Handler(int signum) {
	flag = 2;
}
