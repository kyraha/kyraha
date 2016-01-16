#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <iostream>
#include <list>
#include <algorithm>


class DataSet : public std::list < cv::Vec3d > {
public:
	cv::Vec3d GetMedian();
};

cv::Vec3d DataSet::GetMedian()
{
	if (size() > 2) {
		std::vector<double> ordX, ordY, ordZ;
		for (cv::Vec3d dp : *this) {
			ordX.push_back(dp[0]);
			ordY.push_back(dp[1]);
			ordZ.push_back(dp[2]);
		}
		std::sort(ordX.begin(), ordX.end());
		std::sort(ordY.begin(), ordY.end());
		std::sort(ordZ.begin(), ordZ.end());
		return cv::Vec3d(ordX[ordX.size() / 2], ordY[ordY.size() / 2], ordZ[ordZ.size() / 2]);
	}
	else if (size() > 0) return *(this->begin());
	else return cv::Vec3d(0, 0, 0);
}

std::vector<cv::Point3d> objectPoints;


cv::Vec3d CalculateLocation(std::vector<cv::Point> target)
{
	/* Logitech camera, millimeters
	cv::Matx33d cameraMatrix(
	4.9410254557831900e+002, 0, 3.7487505597946938e+002,
	0, 4.9180258750779660e+002, 2.0903256450552996e+002,
	0, 0, 1);
	cv::Matx<double, 5, 1> distCoeffs(
	-2.0994774118936483e-002,
	4.9899418109244344e-002,
	-6.7418809421477795e-004,
	-5.2140053606489992e-004,
	-6.9063255104469826e-002);
	*/

	/* Microsoft HD3000 camera, inches */
	cv::Matx33d camera_matrix(
		7.4230925920305481e+002, 0., 3.0383585011521706e+002, 0.,
		7.4431328863404576e+002, 2.3422929172706634e+002, 0., 0., 1.);
	cv::Matx<double, 5, 1> distortion_coefficients(
		2.0963551753568421e-001, -1.4796846132520820e+000, 0., 0., 2.7677879392937270e+000);

	//Extract 4 corner points assuming the blob is a rectanle, more or less horizontal
	std::vector<cv::Point> hull(4);
	hull[0] = cv::Point(10000, 10000);		// North-West
	hull[1] = cv::Point(0, 10000);			// North-East
	hull[2] = cv::Point(0, 0);				// South-East
	hull[3] = cv::Point(10000, 0);			// South-West
	for (cv::Point point : target) {
		if (hull[0].x + hull[0].y > point.x + point.y) hull[0] = point;
		if (hull[1].y - hull[1].x > point.y - point.x) hull[1] = point;
		if (hull[2].x + hull[2].y < point.x + point.y) hull[2] = point;
		if (hull[3].x - hull[3].y > point.x - point.y) hull[3] = point;
	}

	//cv::polylines(Im, hull, true, cv::Scalar(0, 255, 0));

	// Make 'em double
	std::vector<cv::Point2d> imagePoints(4);
	imagePoints[0] = hull[0];
	imagePoints[1] = hull[1];
	imagePoints[2] = hull[2];
	imagePoints[3] = hull[3];

	cv::Vec3d rvec, tvec;
	cv::solvePnP(objectPoints, imagePoints, camera_matrix, distortion_coefficients, rvec, tvec, false, CV_EPNP);

	cv::Matx33d Rmat;
	cv::Rodrigues(rvec, Rmat);

	return -(Rmat.t() * tvec);
}

int main(int argc, char** argv)
{
	cv::Vec3i BlobLower(144,  36, 125);
	cv::Vec3i BlobUpper(192, 255, 255);

	cv::VideoCapture capture;
	capture.open(0);
	if (!capture.isOpened()) {
		std::cerr << "Could not initialize video capture" << std::endl;
		return -2;
	}
	cv::Mat Im;
	cv::Mat hsvIm;
	cv::Mat BlobIm;
	cv::namedWindow("Image", CV_WINDOW_AUTOSIZE);
	cv::namedWindow("Blob", CV_WINDOW_AUTOSIZE);

	/* Real FIRSTSTRONGHOLD tower, inches */
	objectPoints.push_back(cv::Point3d(-10, 0, 0));
	objectPoints.push_back(cv::Point3d(10, 0, 0));
	objectPoints.push_back(cv::Point3d(10, 14, 0));
	objectPoints.push_back(cv::Point3d(-10, 14, 0));

	/* Whiteboard res target, millimeters
	objectPoints.push_back(cv::Point3d(-135, 0, 0));
	objectPoints.push_back(cv::Point3d(135, 0, 0));
	objectPoints.push_back(cv::Point3d(135, 150, 0));
	objectPoints.push_back(cv::Point3d(-135, 150, 0));
	*/



	std::vector<cv::Point> stencil;
	stencil.push_back(cv::Point(  9,   0));
	stencil.push_back(cv::Point( 32,   0));
	stencil.push_back(cv::Point( 26,  76));
	stencil.push_back(cv::Point(184,  76));
	stencil.push_back(cv::Point(180,   0));
	stencil.push_back(cv::Point(203,   0));
	stencil.push_back(cv::Point(212, 100));
	stencil.push_back(cv::Point(  0, 100));

	cv::createTrackbar("H. Lower", "Blob", &(BlobLower[0]), 255);
	cv::createTrackbar("H. Upper", "Blob", &(BlobUpper[0]), 255);
	cv::createTrackbar("S. Lower", "Blob", &(BlobLower[1]), 255);
	cv::createTrackbar("S. Upper", "Blob", &(BlobUpper[1]), 255);
	cv::createTrackbar("V. Lower", "Blob", &(BlobLower[2]), 255);
	cv::createTrackbar("V. Upper", "Blob", &(BlobUpper[2]), 255);

	cv::Point textOrg;
	DataSet locations;

	for (; true;) {
		int key = 0xff & cv::waitKey(50);
		if ((key & 255) == 27) break;

		capture >> Im;
		if (Im.empty()) {
			std::cout << " Error reading from camera" << std::endl;
			continue;
		}
		cv::cvtColor(Im, hsvIm, CV_BGR2HSV);
		cv::inRange(hsvIm, BlobLower, BlobUpper, BlobIm);

		textOrg.x = 20;
		textOrg.y = Im.rows - 20;
		/*
		//morphological opening (remove small objects from the foreground)
		cv::erode(BlobIm, BlobIm, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
		cv::dilate(BlobIm, BlobIm, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
		//morphological closing (fill small holes in the foreground)
		cv::dilate(BlobIm, BlobIm, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
		cv::erode(BlobIm, BlobIm, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
		cv::medianBlur(BlobIm, BlobIm, 21);
		*/

		//Extract Contours
		cv::Mat bw;
		BlobIm.convertTo(bw, CV_8UC1);
		std::vector<std::vector<cv::Point>> contours;

		cv::findContours(bw, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

		if (contours.size() > 0) {
			std::vector<cv::Point> target;
			double sim1 = 1000.0;
			for (std::vector<cv::Point> cont : contours)
			{
				// Only process cont if it is big enough, otherwise it's either too far or just a noise
				if (cv::contourArea(cont) > 1000) {
					double similarity = cv::matchShapes(stencil, cont, CV_CONTOURS_MATCH_I3, 1);
					if (similarity < sim1)
					{
						target = cont;
						sim1 = similarity;
					}
				}
			}

			/*
			for (std::vector<cv::Point> it : contours) {
				cv::polylines(Im, it, true, cv::Scalar(255, 0, 0));
			}
			std::cout << "Similarity " << sim1 << std::endl;
			*/

			if (target.size() > 0 && sim1 < 2.0) {
				cv::polylines(Im, target, true, cv::Scalar(0, 200, 255),4);
				cv::Vec3d cameralocation = CalculateLocation(target);

				// Store calculations in a queue but use a list instead so we can iterate
				locations.push_front(cameralocation);
				if (locations.size()>50) locations.pop_back();

				// When we collect enough data get the median value for each coordinate
				// Median rather than average because median tolerate noise better
				if (locations.size()>2) {
					// Replace camera location with improved values
					cameralocation = locations.GetMedian();
				}
				double Ro = sqrtl(cameralocation.dot(cameralocation));
				std::ostringstream oss;
				oss << cameralocation << " " << Ro << std::endl;
				cv::putText(Im, oss.str(), textOrg, 1, 2, cv::Scalar(0, 200,255), 2);
			}
		}


		cv::imshow("Image", Im);
		cv::imshow("Blob", BlobIm);
	}
	return 0;
}


