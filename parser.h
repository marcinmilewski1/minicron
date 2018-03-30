/*
 * parser.h
 *
 *  Created on: 10 Nov 2014
 *      Author: marcin
 */

#ifndef PARSER_H_
#define PARSER_H_

typedef struct Command{
struct Command *next;
int hour;
int minute;
char *commandContent;
int info;
char **parsedCommandContent;
} Command;



void parseAndAddToList(char* taskFileContent,int taskFileSize, Command **CommandList );
void addToCommandList(Command **CommandList, int hour, int minute, char *commandContent, int info);
Command* sortCommandList(Command *CommandList);
void deleteFirst(Command **pointer);
char** breakCommand(char* command);
void freeList(Command *CommandList);
void free2dArray(char **tab);
void printCommandList(Command **CommandList);
#endif /* PARSER_H_ */
