#define _SVID_SOURCE

#include "loader.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <opencv/highgui.h>

static const char kSeparator = '/';
static const size_t kMaxPathLength = 64;

static char* dirname;
static struct dirent** list;
static int listSize;
static int listIndex;

static int filter(const struct dirent* file)
{
    if (file->d_name[0] == '.') {
        return 0;
    }
    return 1;
}

bool Loader_init(char* dir)
{
    assert(dir != NULL);

    dirname = dir;
    listSize = scandir(dirname, &list, filter, alphasort);
    if (listSize == -1) {
        fprintf(stderr, "Failed to scan directory: %s\n", dirname);
        return false;
    }
    return true;
}

void Loader_free(void)
{
    for (int i = 0; i < listSize; i++) {
        free(list[i]);
    }
    free(list);
}

static void appendToPath(char* path, char* dir, char* file, size_t n)
{
    strncat(path, dirname, n);
    if (path[strlen(path) - 1] != kSeparator) {
        strncat(path, &kSeparator, n);
    }
    strncat(path, file, n);
}

IplImage* Loader_loadImage(void)
{
    if (listIndex >= listSize) {
        listIndex = 0;
    }

    char path[kMaxPathLength];
    memset(path, 0, kMaxPathLength);
    appendToPath(path, dirname, list[listIndex]->d_name, kMaxPathLength);

    IplImage* image = cvLoadImage(path, CV_LOAD_IMAGE_COLOR);
    if (image == NULL) {
        fprintf(stderr, "Failed to load image: %s\n", path);
        return NULL;
    }
    // 上下が逆のときは反転させる
    if (image->origin == 0) {
        cvFlip(image, image, -1);
    }

    listIndex++;
    return image;
}
