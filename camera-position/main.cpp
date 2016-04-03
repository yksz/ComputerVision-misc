#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

namespace {

const int kChessPatternRows = 7;
const int kChessPatternColumns = 10;
const int kChessPatternSize = kChessPatternRows * kChessPatternColumns;
const float kChessSize = 24.0; // [mm]

/**
 * $B%U%!%$%k$+$i%+%a%i$NFbIt%Q%i%a!<%?$rFI$_9~$_$^$9!#(B
 *
 * @param[in] paramsFileName $B%U%!%$%kL>(B
 * @param[out] intrinsic $B%+%a%i$NFbIt%Q%i%a!<%?9TNs(B
 * @param[out] distortion $BOD$_78?t%Y%/%H%k(B
 * @return $BFI$_9~$a$?>l9g$O(Btrue$B!"$=$&$G$J$1$l$P(Bfalse
 */
bool readCameraParameters(const std::string& paramsFileName,
        cv::Mat& intrinsic, cv::Mat& distortion) {
    cv::FileStorage fs(paramsFileName, cv::FileStorage::READ);
    fs["intrinsic"] >> intrinsic;
    fs["distortion"] >> distortion;
    return intrinsic.total() != 0 && distortion.total() != 0;
}

/**
 * $BJ*BN:BI86u4V$K$*$1$kJ*BN>e$NE@:BI8$rFI$_9~$_$^$9!#(B
 *
 * @param[out] objectPoints $BJ*BN>e$NE@(B
 */
void readObjectPoints(std::vector<cv::Point3f>& objectPoints) {
    for (int i = 0; i < kChessPatternRows; i++) {
        for (int j = 0; j < kChessPatternColumns; j++) {
            cv::Point3f point;
            point.x = j * kChessSize;
            point.y = i * kChessSize;
            point.z = 0.0;
            objectPoints.push_back(point);
        }
    }
}

/**
 * $B2hA|$+$i%A%'%9%\!<%I$NFbB&8rE@0LCV$r5a$a$^$9!#(B
 *
 * @param[in] image $B2hA|(B
 * @param[in] patternSize $B%A%'%9%\!<%I$N9T$HNs$4$H$NFbB&8rE@$N8D?t(B
 * @param[out] corners $B%A%'%9%\!<%I$N8rE@0LCV(B
 * @return $B5a$a$k$3$H$,$G$-$?>l9g$O(Btrue$B!"$=$&$G$J$1$l$P(Bfalse
 */
bool findChessboardCorners(cv::Mat& image, cv::Size& patternSize,
        std::vector<cv::Point2f>& corners) {
    bool found = cv::findChessboardCorners(image, patternSize, corners);
    if (!found) {
        std::cerr << "ERROR: ChessboardCorners not found\n";
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
 * $B2hA|$+$i2hA|>e$NBP1~E@$rFI$_9~$_$^$9!#(B
 *
 * @param[in] imageFileName $B%U%!%$%kL>(B
 * @param[out] imagePoints $B2hA|>e$NBP1~E@(B
 * @return $BFI$_9~$a$?>l9g$O(Btrue$B!"$=$&$G$J$1$l$P(Bfalse
 */
bool readImagePoints(const std::string& imageFileName,
        std::vector<cv::Point2f>& imagePoints) {
    // $B%A%'%9%\!<%I$N8rE@$r8!=P$9$k(B
    cv::Mat image = cv::imread(imageFileName);
    cv::Size patternSize(kChessPatternColumns, kChessPatternRows);
    bool found = findChessboardCorners(image, patternSize, imagePoints);
    if (!found) {
        std::cerr << "ERROR: Could not find chessboard corners\n";
        return false;
    }

    // $B8!=P$7$?8rE@$rI=<($9$k(B
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
                << " <camera params file> <chessboard image file>\n";
        return 1;
    }
    std::string cameraParamsFileName(argv[1]);
    std::string chessboardImage(argv[2]);

    cv::Mat intrinsic, distortion;
    if (!readCameraParameters(cameraParamsFileName, intrinsic, distortion)) {
        std::cerr << "ERROR: Failed to read intrinsic parameters\n";
        return 1;
    }

    std::vector<cv::Point3f> objectPoints;
    readObjectPoints(objectPoints);

    std::vector<cv::Point2f> imagePoints;
    if (!readImagePoints(chessboardImage, imagePoints)) {
        std::cerr << "ERROR: Failed to read image points\n";
        return 1;
    }

    cv::Mat rvec, tvec;
    cv::solvePnP(objectPoints, imagePoints, intrinsic, distortion, rvec, tvec);

    std::cout << "rvec: \n" << rvec << std::endl;
    std::cout << "tvec: \n" << tvec << std::endl;
    return 0;
}
