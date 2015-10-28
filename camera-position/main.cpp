#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

namespace {

static const std::string kClickWindowName = "Click image points";

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

void drawCross(cv::Mat& image, cv::Point2f& point, const cv::Scalar& color, int length, int thickness = 1) {
    cv::line(image, cv::Point2f(point.x - length, point.y), cv::Point2f(point.x + length, point.y), color, thickness);
    cv::line(image, cv::Point2f(point.x, point.y - length), cv::Point2f(point.x, point.y + length), color, thickness);
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
            drawCross(*clickImage, point, cv::Scalar(0, 0, 255), 7, 2);
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
 * @param[in] objectPointsFileName 物体上の点座標が書かれたファイル名
 * @param[in] imageFileName 対応する物体が写っている画像ファイル名
 * @param[in] cameraParamsFileName カメラの内部パラメータが書かれたファイル名
 * @param[out] rvec カメラの回転ベクトル
 * @param[out] tvec カメラの並進ベクトル
 * @return 推定できた場合はtrue、そうでなければfalse
 */
bool estimateCameraPosition(const std::string& objectPointsFileName,
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
    if (!readImagePoints(imageFileName, objectPoints.size(), imagePoints)) {
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
    if (argc < 3) {
        std::cerr << "usage: "
                << argv[0]
                << " <object points file> <image file> <camera parameters file>"
                << std::endl;
        return 1;
    }
    std::string objectPointsFileName(argv[1]);
    std::string imageFileName(argv[2]);
    std::string cameraParamsFileName(argv[3]);

    cv::Mat rvec, tvec;
    bool result = estimateCameraPosition(
            objectPointsFileName,
            imageFileName,
            cameraParamsFileName,
            rvec, tvec);
    std::cout << "rvec:\n" << rvec << std::endl;
    std::cout << "tvec:\n" << tvec << std::endl;
    return result;
}
