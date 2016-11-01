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

static CvCapture* s_capture = NULL;
static IplImage* s_fileImage = NULL;

static IplImage* detectSkinColor(IplImage* src)
{
    assert(src != NULL);

    cvCvtColor(src, src, CV_BGR2HSV);

    IplImage* mask = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++){
            int h = src->imageData[src->widthStep * y + x * 3 + 0];
            int s = src->imageData[src->widthStep * y + x * 3 + 1];
            int v = src->imageData[src->widthStep * y + x * 3 + 2];
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

    cvCvtColor(src, src, CV_HSV2BGR);
    return mask;
}

static IplImage* renderInvisible(const IplImage* src, const IplImage* bg)
{
    assert(src != NULL || bg != NULL);

    IplImage* dst = cvCloneImage(src);
    IplImage* mask = detectSkinColor(dst);
    for (int y = 0; y < dst->height; y++) {
        for (int x = 0; x < dst->width; x++) {
            if (mask->imageData[mask->widthStep * y + x] != 0) {
                dst->imageData[dst->widthStep * y + x * 3 + 0] = bg->imageData[bg->widthStep * y + x * 3 + 0];
                dst->imageData[dst->widthStep * y + x * 3 + 1] = bg->imageData[bg->widthStep * y + x * 3 + 1];
                dst->imageData[dst->widthStep * y + x * 3 + 2] = bg->imageData[bg->widthStep * y + x * 3 + 2];
            }
        }
    }
    cvReleaseImage(&mask);
    return dst;
}

static IplImage* loadImage(const char* filename)
{
    assert(filename != NULL);

    IplImage* img = cvLoadImage(filename, CV_LOAD_IMAGE_COLOR);
    if (img == NULL) {
        fprintf(stderr, "ERROR: Failed to load image: %s\n", filename);
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

static IplImage* getImage()
{
    if (s_capture != NULL) {
        return cvQueryFrame(s_capture);
    } else if (s_fileImage != NULL) {
        return s_fileImage;
    } else {
        s_capture = cvCaptureFromCAM(CV_CAP_ANY);
        if (s_capture == NULL) {
            fprintf(stderr, "ERROR: Camera not found\n");
            return NULL;
        }
        cvSetCaptureProperty(s_capture, CV_CAP_PROP_FRAME_WIDTH, kWidth);
        cvSetCaptureProperty(s_capture, CV_CAP_PROP_FRAME_HEIGHT, kHeight);
        return cvQueryFrame(s_capture);
    }
}

int main(int argc, char** argv)
{
    if (argc > 1) {
        s_fileImage = loadImage(argv[1]);
    }
    IplImage* background = NULL;

    Mode mode = Mode_CAPTURE;
    while (1) {
        IplImage* image = getImage();
        if (image == NULL) {
            return 1;
        }

        if (mode == Mode_MASK) {
            IplImage* mask = detectSkinColor(image);
            cvShowImage(kWindowName, mask);
            releaseImage(mask);
        } else if (mode == Mode_INVISIBLE) {
            IplImage* invisible = renderInvisible(image, background);
            cvShowImage(kWindowName, invisible);
            releaseImage(invisible);
        } else {
            cvShowImage(kWindowName, image);
        }

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
            background = releaseImage(background);
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
