
#pragma once

#include <stdio.h>
#include <string>

FILE * popen2(std::string command, std::string type, int & pid);
int    pclose2(FILE * fp, pid_t pid);
