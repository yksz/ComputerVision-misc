#define _SVID_SOURCE

#include "loader.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <opencv/highgui.h>

static const char kSeparator = '/';
static const size_t kMaxPathLength = 64;

static const char* s_dirname;
static struct dirent** s_list;
static int s_listSize;
static int s_listIndex;

static int filter(const struct dirent* file)
{
    if (file->d_name[0] == '.') {
        return 0;
    }
    return 1;
}

bool Loader_init(const char* dir)
{
    assert(dir != NULL);

    s_dirname = dir;
    s_listSize = scandir(s_dirname, &s_list, filter, alphasort);
    if (s_listSize == -1) {
        fprintf(stderr, "ERROR: Failed to scan directory: %s\n", s_dirname);
        return false;
    }
    return true;
}

void Loader_finalize(void)
{
    for (int i = 0; i < s_listSize; i++) {
        free(s_list[i]);
    }
    free(s_list);
}

static void getFilePath(char* path, const char* dir, const char* file, size_t n)
{
    strncat(path, dir, n);
    if (path[strlen(path) - 1] != kSeparator) {
        strncat(path, &kSeparator, n);
    }
    strncat(path, file, n);
}

IplImage* Loader_loadImage(void)
{
    if (s_listIndex >= s_listSize) {
        s_listIndex = 0;
    }

    char path[kMaxPathLength];
    memset(path, 0, kMaxPathLength);
    getFilePath(path, s_dirname, s_list[s_listIndex]->d_name, kMaxPathLength);

    IplImage* image = cvLoadImage(path, CV_LOAD_IMAGE_COLOR);
    if (image == NULL) {
        fprintf(stderr, "ERROR: Failed to load image: %s\n", path);
        return NULL;
    }
    // 上下が逆のときは反転させる
    if (image->origin == 0) {
        cvFlip(image, image, -1);
    }

    s_listIndex++;
    return image;
}
