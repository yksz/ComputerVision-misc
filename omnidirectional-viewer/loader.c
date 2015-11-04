#include "loader.h"
#include <stdio.h>
#include <opencv/highgui.h>

static char* filePath = "";

void Loader_init(char* filename) {
    filePath = filename;
}

IplImage* Loader_loadImage(void)
{
    IplImage* image = cvLoadImage(filePath, CV_LOAD_IMAGE_COLOR);
    if (image == NULL) {
        fprintf(stderr, "Failed to load image: %s\n", filePath);
        return NULL;
    }

    // 上下が逆のときは反転させる
    if (image->origin == 0) {
        cvFlip(image, image, -1);
    }
    return image;
}

