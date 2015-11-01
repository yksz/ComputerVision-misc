#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>

static const char* kBackgroundImageFileName = "background.png";
static const char* kWindowName = "Invisible";
static const double kWidth = 640;
static const double kHeight = 480;

typedef enum {
    Mode_CAPTURE, Mode_MASK, Mode_INVISIBLE
} Mode;

static CvCapture* capture;

static IplImage* detectSkinColor(IplImage* img)
{
    assert(img != NULL);

    cvCvtColor(img, img, CV_BGR2HSV);

    IplImage* mask = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1);
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++){
            int h = img->imageData[img->widthStep * y + x * 3 + 0];
            int s = img->imageData[img->widthStep * y + x * 3 + 1];
            int v = img->imageData[img->widthStep * y + x * 3 + 2];
            if (h > 3 && h < 23) { // skin color detected
                mask->imageData[mask->widthStep * y + x] = 255;
            } else {
                mask->imageData[mask->widthStep * y + x] = 0;
            }
        }
    }
    cvSmooth(mask, mask, CV_MEDIAN, 3, 0, 0, 0);
    cvErode(mask, mask, NULL, 3);
    cvDilate(mask, mask, NULL, 1);

    cvCvtColor(img, img, CV_HSV2BGR);
    return mask;
}

static void renderInvisible(IplImage* img, IplImage* bg)
{
    assert(img != NULL);
    assert(bg != NULL);

    IplImage* mask = detectSkinColor(img);
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            if (mask->imageData[mask->widthStep * y + x] != 0) {
                img->imageData[img->widthStep * y + x * 3 + 0] = bg->imageData[bg->widthStep * y + x * 3 + 0];
                img->imageData[img->widthStep * y + x * 3 + 1] = bg->imageData[bg->widthStep * y + x * 3 + 1];
                img->imageData[img->widthStep * y + x * 3 + 2] = bg->imageData[bg->widthStep * y + x * 3 + 2];
            }
        }
    }
    cvShowImage(kWindowName, img);
    cvReleaseImage(&mask);
}

static IplImage* loadImage(const char* filename)
{
    assert(filename != NULL);

    IplImage* img = cvLoadImage(filename, CV_LOAD_IMAGE_COLOR);
    if (img == NULL) {
        fprintf(stderr, "Failed to load image: %s\n", filename);
        return NULL;
    }
    return img;
}

static IplImage* releaseImage(IplImage* img)
{
    if (img != NULL) {
        cvReleaseImage(&img);
    }
    return NULL;
}

static IplImage* getImage(const char* filename)
{
    if (capture != NULL) {
        return cvQueryFrame(capture);
    } else if (filename != NULL) {
        return loadImage(filename);
    } else {
        capture = cvCaptureFromCAM(CV_CAP_ANY);
        if (capture == NULL) {
            fprintf(stderr, "ERROR: Camera not found\n");
            return false;
        }
        cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, kWidth);
        cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, kHeight);
        return cvQueryFrame(capture);
    }
}

int main(int argc, char** argv)
{
    char* testImageFileName = NULL;
    if (argc > 1) {
        testImageFileName = argv[1];
    }

    Mode mode = Mode_CAPTURE;
    IplImage* background = NULL;
    while (1) {
        IplImage* image = getImage(testImageFileName);
        if (image == NULL) {
            return 1;
        }

        switch (mode) {
            case Mode_CAPTURE:
                break;
            case Mode_MASK:
                image = detectSkinColor(image);
                break;
            case Mode_INVISIBLE:
                renderInvisible(image, background);
                break;
        }
        cvShowImage(kWindowName, image);

        int key = cvWaitKey(1);
        if (key == 'q') {
            break;
        } else if (key == 's') {
            printf("Save a background image: %s\n", kBackgroundImageFileName);
            cvSaveImage(kBackgroundImageFileName, image, 0);
        } else if (key == 'c') {
            mode = Mode_CAPTURE;
            background = releaseImage(background);
        } else if (key == 'm') {
            mode = Mode_MASK;
        } else if (key == 'i') {
            mode = Mode_INVISIBLE;
            releaseImage(background);
            if ((background = loadImage(kBackgroundImageFileName)) == NULL) {
                return 1;
            }
        }
    }

    cvDestroyAllWindows();
    releaseImage(background);
    return 0;
}
