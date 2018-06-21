//
// @file datah.h
// Betriebssysteme senderReceiver.
// Beispiel 3
//
// @author Dominic Mages <ic17b014@technikum-wien.at>
// @author Ralph Hödl <ic17b003@technikum-wien.at>
// @author David North <ic17b086@technikum-wien.at>
// @date 2018/06/14
//
// @version 001
//
//

#ifndef DATAH_H
#define DATAH_H

#include <stdio.h>
#include <stdlib.h>

long sharedSize(int argc, char *argv[]);
int sharedSend(long shmSize, FILE *stream);
int sharedReceive(long shmSize, FILE *stream);

#endif 