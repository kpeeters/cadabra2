
#pragma once

#include <internal/unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string>

FILE * popen2(std::string command, std::string type, int & pid);
int    pclose2(FILE * fp, pid_t pid);
