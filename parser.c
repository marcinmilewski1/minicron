/*
 * parser.c
 *
 *  Created on: 10 Nov 2014
 *      Author: marcin
 */
#include "utils.h"
#include "parser.h"

void addToCommandList(Command **CommandList, int hour, int minute, char *commandContent, int info) {
	// jesli brak elementow w liscie zaalokuj pierwszy element
	if (*CommandList == NULL) {
		*CommandList = (Command*) malloc(sizeof(Command));
		if (*CommandList == NULL) {
			syslog(LOG_ERR, "CommandList malloc error  %d.", errno );
			exit(EXIT_FAILURE);
		}
		(*CommandList)->hour=hour;
		(*CommandList)->minute=minute;
		(*CommandList)->info=info;
		(*CommandList)->commandContent = commandContent;
		(*CommandList)->next = NULL;
		(*CommandList)->parsedCommandContent = breakCommand(commandContent);
	}
	else {
	// zapamietaj poczatek listy:
	Command *pocz = *CommandList;

	// jesli sa elementy, przesun do ostatniego i dodaj
	while (((*CommandList)->next) != NULL) {
		*CommandList = (*CommandList)->next;
	}

	(*CommandList)->next = (Command*)malloc(sizeof(Command));
	if (*CommandList == NULL) {
		syslog(LOG_ERR, "CommandList malloc error  %d.", errno );
				exit(EXIT_FAILURE);
			}
	 *CommandList = (*CommandList)->next;
	(*CommandList)->hour=hour;
	(*CommandList)->minute=minute;
	(*CommandList)->info=info;
	(*CommandList)->commandContent = commandContent;
	(*CommandList)->next = NULL;
	(*CommandList)->parsedCommandContent = breakCommand(commandContent);

	// powrot na poczatek
	*CommandList = pocz;
	}
}

void parseAndAddToList(char* taskFileContent, int taskFileSize, Command **CommandList) {
	// linia - xx:xx:komenda:x
	int hour = 0, minute = 0, info = 0, block = 0;
	int flag = 0; // 0 - hour, 1 - minute, 2 - command, 3 - info
	char *commandContent = NULL;
	int i;

	for (i = 0; i < taskFileSize; i++) {
		if (taskFileContent[i] == ':' || taskFileContent[i] == '\n') {
			taskFileContent[i] = '\0';	// zamiana terminatora na null, latwiejsza oblsuga komend
			flag++;
			continue;
		}
		switch (flag) {
		case 0:
			hour += (taskFileContent[i] - '0') * 10;
			i++;
			if (taskFileContent[i] == ':') {
				syslog(LOG_ERR, "Wrong time format"); // przypadek x:xx:komenda:x zamiast xx:xx:komenda:x
				exit(EXIT_FAILURE);
			}
			hour += (taskFileContent[i]) - '0';
			if (hour >= 24 || hour < 0) {
				syslog(LOG_ERR, "Hour out of bounds" );
				exit(EXIT_FAILURE);
			}
			break;
		case 1:
			minute += (taskFileContent[i] - '0') * 10;
			i++;
			if (taskFileContent[i] == ':') {
				syslog(LOG_ERR, "Wrong time format" );	// przypadek x:xx:komenda:x zamiast xx:xx:komenda:x
				exit(EXIT_FAILURE);
			}
			minute += (taskFileContent[i]) - '0';
			if (minute >= 60 || minute < 0) {
				syslog(LOG_ERR, "Minutes out of bounds" );
				exit(EXIT_FAILURE);
			}
			break;
		case 2:
			if (block == 1) continue;		// blokowanie przed przesuwaniem poczatku komendy
			commandContent = taskFileContent + i;	// poczatek komendy
			block = 1;
			break;
		case 3:
			info = (taskFileContent[i] - '0');
			if (info < 0 || info > 2) {
				syslog(LOG_ERR, "Info argument out of bounds" );
				exit(EXIT_FAILURE);
			}
			addToCommandList(CommandList, hour, minute, commandContent, info);
			flag = -1;	// nastepny znak spowoduje wyzerowanie flagi i zaczecie pars. wiersza od nowa
			hour = 0;
			minute = 0;
			info = 0;
			block = 0;
			commandContent = NULL;
			break;
		default:
			syslog(LOG_ERR, "Parser error" );
			exit(EXIT_FAILURE);
		}
	}
}

Command* sortCommandList(Command *CommandList) {
	if (CommandList == NULL || (CommandList)->next == NULL) return CommandList;	// jesli brak el. lub nie ma go

	// head is the first element of resulting sorted list
	Command* head = NULL;

	while(CommandList != NULL) { // przechodzimy liste
		Command *current = CommandList;
		CommandList = CommandList->next;
		float time1 =0;
		float time2 = 0;

		if (head && current) {
		time1 = (current->hour) + ((float)(current->minute)/100);
		time2 = (head->hour) + ((float)(head->minute)/100);
		}

		if (head == NULL ||  time1 < time2) {
			// insert into the head of the sorted list
			// or as the first element into an empty sorted list
			current->next = head;
			head = current;
		}
		else {
			// insert current element into proper position in non-empty sorted list
			Command *p = head;
			while (p != NULL) {
				float time1 = 0;
				float time2 = 0;
				if (p && current && p->next) {
					time1 = (current->hour) + ((float)(current->minute)/100);
					time2 = (p->next->hour) + ((float)(p->next->minute)/100);
				}
				if (p->next == NULL || time1 < time2) // last element of sorted list or middle of the list
				{
					// insert into middle of the sorted list or as the last element
					current->next = p->next;
					p->next = current;
					break; //done
				}
				p = p->next;
			}
		}
	}
	return head;
}

void deleteFirst(Command **pointer) {
	Command *CommandList = (*pointer);
	if (CommandList != NULL) {
		Command *help;
		help = CommandList;
		CommandList = help->next;
		free2dArray(help->parsedCommandContent);
		free(help);
		*pointer = CommandList;
	} else {
		syslog(LOG_ERR, "Cannot delete from empty list.");
		exit(EXIT_FAILURE);
	}
}




char** breakCommand(char* command)
{

int i=0;
char** paramets;

int sizecommand=0;
sizecommand=strlen(command);

int numberspaces=0,size=0;
while(i<sizecommand && command[i]!='|')
        {

        if(command[i]==' ')
                {
                numberspaces++;
                }
        i++;
        }
if(i<sizecommand && command[i]=='|')
numberspaces--;


paramets=(char**)malloc((numberspaces+2) * sizeof(char*));


int counter=0;

i=0;
int j=0,k=0,help=0;
while(i<sizecommand)
{

        if(command[i]==' ')
        {
                if(command[i+1]=='|')
                break;

                paramets[k]=(char*)malloc((counter+1) * sizeof(char*));
                j=0;
                help=i-counter;
                while(help<i)
                        {
                        paramets[k][j]=command[help];
                        j++;
                        help++;
                        }
                paramets[k][j]='\0';
		counter=0;
                k++;
                i++;
                continue;

        }
counter++;
i++;
}


j=0;
help=i-counter;
paramets[k]=(char*)malloc((counter+1) * sizeof(char*));

while(help<sizecommand && command[help]!=' ')
        {
        paramets[k][j]=command[help];
        j++;
        help++;
        }
paramets[k][j]='\0';
paramets[k+1]=NULL;

return paramets;
}




void freeList(Command *CommandList) {
	if (CommandList == NULL) return;;

	Command *temp = NULL;
	while(CommandList) {
		temp = CommandList;

		// Nie trzeba zwalniac zawartosci listy poniewaz sa to wskazniki na zawartosc pliku ktora zwalniamy wczesinej
		// Zwalniamy tylko char**

		free2dArray(temp->parsedCommandContent);	// alokowana oddzielnie

		CommandList = CommandList->next;
		free(temp);
	}
}

void free2dArray(char **tab) {
	/* Zwalnia tablice stringow zakonczona NULLEM */
	if (tab == NULL) return ;;	// jesli zawartosc wskaznika to NULL

	char *current = *tab;
	int i = 0;
	while (current) {
		//current = NULL;
		free (current);
		i++;
		current = *(tab + i);
	}
	tab = NULL;
}

void printCommandList(Command **CommandList) {
	if (*CommandList == NULL) return ;;

	Command *start = *CommandList;
	Command *temp = start;
	int i = 1;
	syslog(LOG_NOTICE, "Pozostale zadania do wykonania:");
	while(temp) {
		syslog(LOG_NOTICE, "%d. %d:%d:%s:%d",
				i,
				temp->hour,
				temp->minute,
				temp->commandContent,
				temp->info);
		temp = temp->next;
		i++;
	}
	*CommandList = start;	// powrot na poczatek
}
