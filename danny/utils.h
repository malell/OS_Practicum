
#ifndef UTILS_H
#define UTILS_H

#include "fdreader.h"

//File types
#define TXT   1
#define JPG   0

void printFiles(Directory *directory);
void showTextFile(Text text);
int fileType(char* filename);
void freeDirStruct(Directory *directory);


#endif //UTILS_H
