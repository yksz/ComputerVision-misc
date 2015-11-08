#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

namespace {

static const int kDefaultNumImages = 3;
static const int kChessPatternRows = 7;
static const int kChessPatternColumns = 10;
static const float kChessGridSize = 24.0; // [mm]

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
 * @param[in] image 画像
 * @param[out] imagePoints 画像上の対応点
 * @return 読み込めた場合はtrue、そうでなければfalse
 */
bool readImagePoints(cv::Mat& image,
        std::vector<cv::Point2f>& imagePoints) {
    // チェスボードの交点を検出する
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
 * カメラキャリブレーションを実行します。
 *
 * @param[in] imageDirName 使用する画像ファイルが置いてあるディレクトリ
 * @param[in] numImages 使用する画像の数
 * @param[out] intrinsic カメラの内部パラメータ行列
 * @param[out] distortion 歪み係数ベクトル
 * @param[out] rvecs 各画像におけるカメラの回転ベクトル
 * @param[out] tvecs 各画像におけるカメラの並進ベクトル
 * @return キャリブレーションに成功した場合はtrue、そうでなければfalse
 */
bool calibrateCamera(const std::string& imageDirName, int numImages,
        cv::Mat& intrinsic, cv::Mat& distortion,
        std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs) {
    std::vector<std::vector<cv::Point2f> > imagePointsList;
    cv::Size imageSize;
    for (int i = 0; i < numImages; i++) {
        std::stringstream ss;
        ss << imageDirName << "/" << i << ".png";
        std::string filename = ss.str();
        cv::Mat image = cv::imread(filename);
        if (image.data == NULL) {
            std::cerr << "ERROR: Failed to load image: " << filename << std::endl;
            continue;
        }
        if (imageSize.area() == 0) {
            imageSize = image.size();
        }
        std::vector<cv::Point2f> imagePoints;
        if (readImagePoints(image, imagePoints)) {
            std::cout << filename << "..." << "ok\n";
            imagePointsList.push_back(imagePoints);
        } else {
            std::cout << filename << "..." << "fail\n";
        }
    }
    if (imagePointsList.empty()) {
        return false;
    }

    std::vector<std::vector<cv::Point3f> > objectPointsList;
    std::vector<cv::Point3f> objectPoints;
    readObjectPoints(objectPoints);
    for (std::size_t i = 0; i < imagePointsList.size(); i++) {
        objectPointsList.push_back(objectPoints);
    }

    cv::calibrateCamera(objectPointsList, imagePointsList, imageSize,
            intrinsic, distortion, rvecs, tvecs);
    return true;
}

/**
 * ファイルにカメラ情報を書き込みます。
 *
 * @param[in] filename ファイル名
 * @param[in] intrinsic カメラの内部パラメータ行列
 * @param[in] distortion 歪み係数ベクトル
 * @param[in] rvec カメラの回転ベクトル
 * @param[in] tvec カメラの並進ベクトル
 * @return 書き込めた場合はtrue、そうでなければfalse
 */
bool writeCameraInfo(const std::string& filename,
        cv::Mat& intrinsic, cv::Mat& distortion,
        cv::Mat& rvec, cv::Mat& tvec) {
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        std::cerr << "ERROR: Failed to open the file: " << filename << std::endl;
        return false;
    }
    fs << "intrinsic" << intrinsic;
    fs << "distortion" << distortion;
    fs << "rotation" << rvec;
    fs << "translation" << tvec;
    fs.release();
    return true;
}

} // unnamed namespace

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::cerr << "usage: "
                << argv[0]
                << " <image directory> [num of images]"
                << std::endl;
        return 1;
    }
    const std::string imageDirName(argv[1]);
    int numImages = kDefaultNumImages;
    if (argc > 2) {
        int num = atoi(argv[2]);
        numImages = num ? num : numImages;
    }

    cv::Mat intrinsic, distortion;
    std::vector<cv::Mat> rvecs, tvecs;
    if (!calibrateCamera(imageDirName, numImages, intrinsic, distortion, rvecs, tvecs)) {
        std::cerr << "ERROR: Failed to calibrate camera" << std::endl;
        return 1;
    }
    std::cout << std::endl;
    std::cout << "intrinsic:\n" << intrinsic << std::endl;
    std::cout << "distortion:\n" << distortion << std::endl;
    std::cout << std::endl;
    std::cout << imageDirName << "/0.png: " << std::endl;
    std::cout << "rvec:\n" << rvecs[0] << std::endl;
    std::cout << "tvec:\n" << tvecs[0] << std::endl;

    const std::string cameraInfoFileName = "camera.xml";
    if (writeCameraInfo(cameraInfoFileName,
                intrinsic, distortion, rvecs[0], tvecs[0])) {
        std::cout << "Write the camera info to " << cameraInfoFileName << std::endl;
    }
    return 0;
}
