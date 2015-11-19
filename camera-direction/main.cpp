#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

/**
 * 射影行列を計算します。
 *
 * @param[in] intrinsic カメラの内部パラメータ行列
 * @param[in] rvec カメラの回転ベクトル
 * @param[in] tvec カメラの並進ベクトル
 * @return 射影行列
 */
static cv::Mat calculateProjectionMatrix(cv::Mat& intrinsic, cv::Mat& rvec, cv::Mat& tvec) {
//
// 世界座標系 -> 画像座標系
// s*p = A*[R|t]*P
//                                     |X|
//  |u|   |fx  0 cx|   |r1 r2 r3 tx|   |Y|
// s|v| = | 0 fy cy| * |r4 r5 r6 ty| * |Z|
//  |1|   | 0  0  1|   |r7 r8 r9 tz|   |1|

    cv::Mat rotMat;
    cv::Rodrigues(rvec, rotMat);

    cv::Mat rtMat(3, 4, CV_64FC1);
    for (int i = 0; i < rotMat.rows; i++) {
        for (int j = 0; j < rotMat.cols; j++) {
            rtMat.at<double>(i, j) = rotMat.at<double>(i, j);
        }
    }
    for (int i = 0; i < tvec.rows; i++) {
        rtMat.at<double>(i, 3) = tvec.at<double>(i, 0);
    }

    return intrinsic * rtMat;
}

/**
 * ファイルからカメラパラメータを読み込みます。
 *
 * @param[in] filename ファイル名
 * @param[out] intrinsic カメラの内部パラメータ行列
 * @param[out] distortion 歪み係数ベクトル
 * @return 読み込めた場合はtrue、そうでなければfalse
 */
static bool readCameraParameters(const std::string& filename,
        cv::Mat& intrinsic, cv::Mat& distortion) {
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        std::cerr << "ERROR: Failed to open file: " << filename << std::endl;
        return false;
    }
    fs["intrinsic"] >> intrinsic;
    fs["distortion"] >> distortion;
    fs.release();
    return intrinsic.total() != 0 && distortion.total() != 0;
}

/**
 * ファイルからカメラ位置を読み込みます。
 *
 * @param[in] filename ファイル名
 * @param[in] rvec カメラの回転ベクトル
 * @param[in] tvec カメラの並進ベクトル
 * @return 読み込めた場合はtrue、そうでなければfalse
 */
static bool readCameraPosition(const std::string& filename,
        cv::Mat& rvec, cv::Mat& tvec) {
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        std::cerr << "ERROR: Failed to open file: " << filename << std::endl;
        return false;
    }
    fs["rotation"] >> rvec;
    fs["translation"] >> tvec;
    fs.release();
    return rvec.total() != 0 && tvec.total() != 0;
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        std::cerr << "usage: "
                << argv[0]
                << " <camera parameters file> <camera position file>"
                << std::endl;
        return 1;
    }
    const std::string cameraParamsFileName(argv[1]);
    const std::string cameraPositionFileName(argv[2]);

    cv::Mat intrinsic, distortion;
    if (!readCameraParameters(cameraParamsFileName, intrinsic, distortion)) {
        return 1;
    }
    cv::Mat rvec, tvec;
    if (!readCameraPosition(cameraPositionFileName, rvec, tvec)) {
        return 1;
    }
    cv::Mat projMat = calculateProjectionMatrix(intrinsic, rvec, tvec);

    cv::Mat rotMat, transVect;
    cv::Mat rotMatX, rotMatY, rotMatZ;
    cv::Vec3d eulerAngles;
    cv::decomposeProjectionMatrix(projMat, intrinsic, rotMat, transVect,
            rotMatX, rotMatY, rotMatZ, eulerAngles);
    std::cout << "[roll, pitch, yaw] = " << eulerAngles << std::endl;
    return 0;
}
