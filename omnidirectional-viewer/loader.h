#ifndef LOADER_H
#define LOADER_H

#include <opencv/cv.h>

void Loader_init(char* filename);
IplImage* Loader_loadImage(void);

#endif /* LOADER_H */

