#ifndef LOADER_H
#define LOADER_H

#include <stdbool.h>
#include <opencv/cv.h>

bool Loader_init(char* dir);
void Loader_free(void);
IplImage* Loader_loadImage(void);

#endif /* LOADER_H */
