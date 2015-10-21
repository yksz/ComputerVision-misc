#include <stdio.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>

static const char* kWindowName = "Capture";
static const double kWidth = 640;
static const double kHeight = 480;
static const int kMessageSize = 64;

int main(int argc, char** argv)
{
    // カメラからのビデオキャプチャを初期化する
    CvCapture* capture = cvCaptureFromCAM(CV_CAP_ANY);
    if (capture == NULL) {
        fprintf(stderr, "ERROR: Camera not found\n");
        return 1;
    }

    // キャプチャサイズを設定する
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, kWidth);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, kHeight);

    // ウィンドウを作成する
    cvNamedWindow(kWindowName, CV_WINDOW_AUTOSIZE);

    // フォント構造体を初期化する
    CvFont font;
    cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 1.0f, 1.0f, 0.0f, 1, CV_AA);

    // カメラから画像をキャプチャする
    while (1) {
        long startTick = cvGetTickCount();
        IplImage* image = cvQueryFrame(capture); // カメラから一つのフレームを取り出して返す
        long stopTick = cvGetTickCount();

        char message[kMessageSize];
        snprintf(message, kMessageSize, "%.3f [ms]", (stopTick - startTick) / cvGetTickFrequency() / 1000);
        cvPutText(image, message, cvPoint(10, 20), &font, CV_RGB(0, 0, 0)); // 画像に文字列を描画する
        cvShowImage(kWindowName, image); // ウィンドウに画像を表示する

        int key = cvWaitKey(1); // キーが押されるまで待機する
        if (key == 0x1b) { // Escキー
            break;
        } else if (key == 's') {
            char* filename = "capture.png";
            printf("Save the capture image: %s\n", filename);
            //cvSaveImage(filename, image); // OpenCV 1.0
            cvSaveImage(filename, image, 0); // OpenCV 2.0
        }
    }

    cvReleaseCapture(&capture);
    cvDestroyWindow(kWindowName);
    return 0;
}
