#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

namespace {

static const int kChessPatternRows = 7;
static const int kChessPatternColumns = 10;
static const float kChessGridSize = 24.0; // [mm]

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
 * 物体座標空間における物体上の点座標を読み込みます。
 *
 * @param[out] objectPoints 物体上の点
 */
void readObjectPoints(std::vector<cv::Point3f>& objectPoints) {
    for (int i = 0; i < kChessPatternRows; i++) {
        for (int j = 0; j < kChessPatternColumns; j++) {
            cv::Point3f point;
            point.x = j * kChessGridSize;
            point.y = i * kChessGridSize;
            point.z = 0.0;
            objectPoints.push_back(point);
        }
    }
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
 * 画像から画像上の対応点を読み込みます。
 *
 * @param[in] filename ファイル名
 * @param[out] imagePoints 画像上の対応点
 * @return 読み込めた場合はtrue、そうでなければfalse
 */
bool readImagePoints(const std::string& filename,
        std::vector<cv::Point2f>& imagePoints) {
    // チェスボードの交点を検出する
    cv::Mat image = cv::imread(filename);
    cv::Size patternSize(kChessPatternColumns, kChessPatternRows);
    bool found = findChessboardCorners(image, patternSize, imagePoints);
    if (!found) {
        std::cerr << "ERROR: Failed to find chessboard corners\n";
        return false;
    }

    // 検出した交点を表示する
    std::string windowName = "ChessboardCorners";
    cv::namedWindow(windowName, CV_WINDOW_AUTOSIZE);
    cv::drawChessboardCorners(image, patternSize, imagePoints, found);
    cv::imshow(windowName, image);
    cv::waitKey(0);
    return true;
}

} // unnamed namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: "
                << argv[0]
                << " <camera parameters file> <image file>\n";
        return 1;
    }
    std::string cameraParamsFileName(argv[1]);
    std::string imageFileName(argv[2]);

    cv::Mat intrinsic, distortion;
    if (!readCameraParameters(cameraParamsFileName, intrinsic, distortion)) {
        std::cerr << "ERROR: Failed to read camera parameters\n";
        return 1;
    }

    std::vector<cv::Point3f> objectPoints;
    readObjectPoints(objectPoints);

    std::vector<cv::Point2f> imagePoints;
    if (!readImagePoints(imageFileName, imagePoints)) {
        std::cerr << "ERROR: Failed to read image points\n";
        return 1;
    }

    cv::Mat rvec, tvec;
    cv::solvePnP(objectPoints, imagePoints, intrinsic, distortion, rvec, tvec);

    std::cout << "rvec: \n" << rvec << std::endl;
    std::cout << "tvec: \n" << tvec << std::endl;
    return 0;
}
