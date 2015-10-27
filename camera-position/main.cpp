#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <getopt.h>

namespace {

static const int kChessPatternRows = 7;
static const int kChessPatternColumns = 10;
static const std::string kClickWindowName = "Click image points";

enum EstimationMode {
    kDefault,
    kChessPattern,
};

cv::Mat* clickImage = NULL;
std::size_t maxClickedCount = 0;
std::vector<cv::Point2f>* clickedPoints = NULL;

/**
 * ファイルから物体座標空間における物体上の点座標を読み込みます。
 *
 * @param[in] filename ファイル名
 * @param[out] objectPoints 物体上の点
 * @return 読み込めた場合はtrue、そうでなければfalse
 */
bool readObjectPoints(const std::string& filename,
        std::vector<cv::Point3f>& objectPoints) {
    std::ifstream ifs(filename.c_str());
    if (!ifs.is_open()) {
        std::cerr << "ERROR: Failed to open file: " << filename << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(ifs, line)) {
        int x, y, z;
        sscanf(line.c_str(), "%d,%d,%d", &x, &y, &z);
        objectPoints.push_back(cv::Point3f(x, y, z));
    }
    return true;
}

void onMouse(int event, int x, int y, int flags, void* params) {
    switch (event) {
        case cv::EVENT_LBUTTONDOWN:
            assert(clickImage != NULL);
            assert(clickedPoints != NULL);

            if (clickedPoints->size() >= maxClickedCount) {
                return;
            }
            cv::Point2f point(x, y);
            clickedPoints->push_back(point);
            std::cout << "count=" << clickedPoints->size() << ", clicked=" << point << std::endl;
            cv::circle(*clickImage, point, 5, CV_RGB(255, 0, 0), 3);
            cv::imshow(kClickWindowName, *clickImage);
            break;
    }
}

/**
 * 画像から画像上の対応点を読み込みます。
 *
 * @param[in] filename 画像ファイル名
 * @param[in] numPoints 対応点の数
 * @param[out] imagePoints 画像上の対応点
 * @return 読み込めた場合はtrue、そうでなければfalse
 */
bool readImagePoints(const std::string& filename,
        int numPoints,
        std::vector<cv::Point2f>& imagePoints) {
    cv::Mat image = cv::imread(filename);
    if (image.data == NULL) {
        std::cerr << "ERROR: Failed to read image" << filename << std::endl;
        return false;
    }
    clickImage = &image;
    maxClickedCount = numPoints;
    clickedPoints = &imagePoints;

    cv::namedWindow(kClickWindowName, cv::WINDOW_AUTOSIZE);
    cv::imshow(kClickWindowName, *clickImage);
    cv::setMouseCallback(kClickWindowName, onMouse);
    cv::waitKey(0);

    clickImage = NULL;
    maxClickedCount = 0;
    clickedPoints = NULL;
    return true;
}

/**
 * 画像からチェスボードの内側交点位置を求めます。
 *
 * @param[in] image 画像
 * @param[in] patternSize チェスボードの行と列ごとの内側交点の個数
 * @param[out] corners チェスボードの交点位置
 * @return 求めることができた場合はtrue、そうでなければfalse
 */
bool findChessboardCorners(cv::Mat& image, cv::Size& patternSize,
        std::vector<cv::Point2f>& corners) {
    bool found = cv::findChessboardCorners(image, patternSize, corners);
    if (!found) {
        return false;
    }
    cv::Mat grayImage(image.rows, image.cols, CV_8UC1);
    cv::cvtColor(image, grayImage, CV_BGR2GRAY);
    cv::cornerSubPix(grayImage,
            corners,
            cv::Size(3, 3),
            cv::Size(-1, -1),
            cv::TermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03));
    return true;
}

/**
 * 画像からチェスボード上の対応点を読み込みます。
 *
 * @param[in] filename 画像ファイル名
 * @param[out] imagePoints 画像上の対応点
 * @return 読み込めた場合はtrue、そうでなければfalse
 */
bool readImagePointsOnChessboard(const std::string& filename,
        std::vector<cv::Point2f>& imagePoints) {
    // チェスボードの交点を検出する
    cv::Mat image = cv::imread(filename);
    if (image.data == NULL) {
        std::cerr << "ERROR: Failed to read image" << filename << std::endl;
        return false;
    }
    cv::Size patternSize(kChessPatternColumns, kChessPatternRows);
    bool found = findChessboardCorners(image, patternSize, imagePoints);
    if (!found) {
        std::cerr << "ERROR: Failed to find chessboard corners\n";
        return false;
    }

    // 検出した交点を表示する
    std::string windowName = "Chessboard Corners";
    cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
    cv::drawChessboardCorners(image, patternSize, imagePoints, found);
    cv::imshow(windowName, image);
    cv::waitKey(0);
    return true;
}

/**
 * ファイルからカメラの内部パラメータを読み込みます。
 *
 * @param[in] filename ファイル名
 * @param[out] intrinsic カメラの内部パラメータ行列
 * @param[out] distortion 歪み係数ベクトル
 * @return 読み込めた場合はtrue、そうでなければfalse
 */
bool readCameraParameters(const std::string& filename,
        cv::Mat& intrinsic, cv::Mat& distortion) {
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        std::cerr << "ERROR: Failed to open the file: " << filename << std::endl;
        return false;
    }
    fs["intrinsic"] >> intrinsic;
    fs["distortion"] >> distortion;
    fs.release();
    return intrinsic.total() != 0 && distortion.total() != 0;
}

/**
 * 物体座標空間におけるカメラ位置を推定します。
 *
 * @param[in] estimationMode 推定処理のモード
 * @param[in] objectPointsFileName 物体上の点座標が書かれたファイル名
 * @param[in] imageFileName 対応する物体が写っている画像ファイル名
 * @param[in] cameraParamsFileName カメラの内部パラメータが書かれたファイル名
 * @param[out] rvec カメラの回転ベクトル
 * @param[out] tvec カメラの並進ベクトル
 * @return 推定できた場合はtrue、そうでなければfalse
 */
bool estimateCameraPosition(EstimationMode estimationMode,
        const std::string& objectPointsFileName,
        const std::string& imageFileName,
        const std::string& cameraParamsFileName,
        cv::Mat& rvec,
        cv::Mat& tvec) {
    std::vector<cv::Point3f> objectPoints;
    if (!readObjectPoints(objectPointsFileName, objectPoints)) {
        std::cerr << "ERROR: Failed to read object points\n";
        return false;
    }

    std::vector<cv::Point2f> imagePoints;
    bool result = false;
    switch (estimationMode) {
        case kChessPattern:
            result = readImagePointsOnChessboard(imageFileName, imagePoints);
            break;

        case kDefault:
            result = readImagePoints(imageFileName, objectPoints.size(), imagePoints);
            break;

        default:
            assert(false && "Unknown mode");
    }
    if (!result) {
        std::cerr << "ERROR: Failed to read image points\n";
        return false;
    }

    cv::Mat intrinsic, distortion;
    if (!readCameraParameters(cameraParamsFileName, intrinsic, distortion)) {
        std::cerr << "ERROR: Failed to read camera parameters\n";
        return false;
    }

    cv::solvePnP(objectPoints, imagePoints, intrinsic, distortion, rvec, tvec);
    return true;
}

} // unnamed namespace

int main(int argc, char** argv) {
    std::string mode;
    int opt;
    while ((opt = getopt(argc, argv, "m:")) != -1) {
        switch (opt) {
            case 'm':
                mode = std::string(optarg);
                break;
        }
    }
    if (argc - optind < 3) {
        std::cerr << "usage: "
                << argv[0]
                << " [options]"
                " <object points file> <image file> <camera parameters file>\n"
                " -m\tmode ('chess' or default)"
                << std::endl;
        return 1;
    }
    EstimationMode estimationMode = mode == "chess" ? kChessPattern : kDefault;
    std::string objectPointsFileName(argv[optind + 0]);
    std::string imageFileName(argv[optind + 1]);
    std::string cameraParamsFileName(argv[optind + 2]);

    cv::Mat rvec, tvec;
    bool result = estimateCameraPosition(estimationMode,
            objectPointsFileName,
            imageFileName,
            cameraParamsFileName,
            rvec, tvec);
    std::cout << "rvec:\n" << rvec << std::endl;
    std::cout << "tvec:\n" << tvec << std::endl;
    return result;
}
