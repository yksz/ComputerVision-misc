#ifndef LOADER_H
#define LOADER_H

#include <stdbool.h>
#include <opencv/cv.h>

bool Loader_initialize(const char* dir);
void Loader_finalize(void);
IplImage* Loader_loadImage(void);

#endif /* LOADER_H */
