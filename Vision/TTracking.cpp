#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <iostream>
#include <list>

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
//	cv::namedWindow("Test", CV_WINDOW_AUTOSIZE);

	cv::createTrackbar("H. Lower", "Blob", &(BlobLower[0]), 255);
	cv::createTrackbar("H. Upper", "Blob", &(BlobUpper[0]), 255);
	cv::createTrackbar("S. Lower", "Blob", &(BlobLower[1]), 255);
	cv::createTrackbar("S. Upper", "Blob", &(BlobUpper[1]), 255);
	cv::createTrackbar("V. Lower", "Blob", &(BlobLower[2]), 255);
	cv::createTrackbar("V. Upper", "Blob", &(BlobUpper[2]), 255);

	std::vector<cv::Point3d> objectPoints;
	objectPoints.push_back(cv::Point3d(-10,  0, 0));
	objectPoints.push_back(cv::Point3d( 10,  0, 0));
	objectPoints.push_back(cv::Point3d( 10, 14, 0));
	objectPoints.push_back(cv::Point3d(-10, 14, 0));

	std::vector<cv::Point> stencil;
	stencil.push_back(cv::Point(  9,   0));
	stencil.push_back(cv::Point( 32,   0));
	stencil.push_back(cv::Point( 26,  76));
	stencil.push_back(cv::Point(184,  76));
	stencil.push_back(cv::Point(180,   0));
	stencil.push_back(cv::Point(203,   0));
	stencil.push_back(cv::Point(212, 100));
	stencil.push_back(cv::Point(  0, 100));

	cv::Matx33d camera_matrix(
		7.4230925920305481e+002, 0., 3.0383585011521706e+002, 0.,
		7.4431328863404576e+002, 2.3422929172706634e+002, 0., 0., 1.);
	cv::Matx<double, 5, 1> distortion_coefficients(
		2.0963551753568421e-001, -1.4796846132520820e+000, 0., 0., 2.7677879392937270e+000);

	cv::Point textOrg;
	std::list<cv::Vec3d> qRo;

	for (; true;) {
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
		//std::vector<cv::Vec4i> hierarchy;

		cv::findContours(bw, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

		if (contours.size() > 0) {
			// Find Largest Contour
			size_t maxid = contours.size();
			size_t secid = contours.size();
			double sim1 = 100.0, sim2 = 100.0;
			for (size_t i = 0; i < contours.size(); i++)
			{
				if (cv::contourArea(contours[i]) > 256) {
					double similarity = cv::matchShapes(stencil, contours[i], CV_CONTOURS_MATCH_I3, 1);
					if (similarity < sim1)
					{
						secid = maxid;
						maxid = i;
						sim1 = similarity;
					}
					else if (similarity < sim2) {
						secid = i;
						sim2 = similarity;
					}

				}
			}
			for (std::vector<std::vector<cv::Point>>::iterator it = contours.begin(); it != contours.end(); ++it) {
				cv::polylines(Im, *it, true, cv::Scalar(255, 0, 0));
			}
			//Extract corner points assuming the two blobs are a credit card
			std::vector<cv::Point> hull;
			if (maxid < contours.size()) {
				hull.push_back(cv::Point(bw.cols, bw.rows));	// North-West
				hull.push_back(cv::Point(0, bw.rows));			// North-East
				hull.push_back(cv::Point(0, 0));				// South-East
				hull.push_back(cv::Point(bw.cols, 0));			// South-West
				for (size_t id = maxid; id == maxid; id = secid) {
					for (size_t i = 0; i < contours[id].size(); ++i) {
						if (hull[0].x + hull[0].y > contours[id][i].x + contours[id][i].y) {
							hull[0].x = contours[id][i].x;
							hull[0].y = contours[id][i].y;
						}
						if (bw.cols - hull[1].x + hull[1].y > bw.cols - contours[id][i].x + contours[id][i].y) {
							hull[1].x = contours[id][i].x;
							hull[1].y = contours[id][i].y;
						}
						if (hull[2].x + hull[2].y < contours[id][i].x + contours[id][i].y) {
							hull[2].x = contours[id][i].x;
							hull[2].y = contours[id][i].y;
						}
						if (hull[3].x + bw.rows - hull[3].y > contours[id][i].x + bw.rows - contours[id][i].y) {
							hull[3].x = contours[id][i].x;
							hull[3].y = contours[id][i].y;
						}
					}
				}
				cv::polylines(Im, hull, true, cv::Scalar(0, 255, 0));

				std::vector<cv::Point2d> imagePoints(hull.size());
				for (size_t i = 0; i < hull.size(); ++i) {
					imagePoints[i] = hull[i];
				}

				cv::Vec3d rvec, tvec;
				cv::solvePnP(objectPoints, imagePoints, camera_matrix, distortion_coefficients, rvec, tvec, false, CV_EPNP);
				cv::Matx33d Rmat;
				cv::Rodrigues(rvec, Rmat);
				cv::Vec3d cameralocation;
				cameralocation = -(Rmat.t() * tvec);
				qRo.push_front(cameralocation);
				if (qRo.size()>50) {
					qRo.pop_back();
				}
				if (qRo.size()>2) {
					std::list<double> ordX, ordY, ordZ;
					std::list<double>::iterator it;
					size_t i;

					for (std::list<cv::Vec3d>::iterator dp = qRo.begin(); dp != qRo.end(); ++dp) {
						ordX.push_back((*dp)[0]);
						ordY.push_back((*dp)[1]);
						ordZ.push_back((*dp)[2]);
					}
					ordX.sort();
					ordY.sort();
					ordZ.sort();
					i = ordX.size() / 2; it = ordX.begin();
					while (i > 0 && it != ordX.end()) ++it, --i;
					cameralocation[0] = *it;
					i = ordY.size() / 2; it = ordY.begin();
					while (i > 0 && it != ordY.end()) ++it, --i;
					cameralocation[1] = *it;
					i = ordZ.size() / 2; it = ordZ.begin();
					while (i > 0 && it != ordZ.end()) ++it, --i;
					cameralocation[2] = *it;
				}
				double Ro = sqrtl(cameralocation.dot(cameralocation));
				std::ostringstream oss;
				oss << "Camera is at: "<< Ro <<", XYZ: "<< cameralocation << std::endl;
				cv::putText(Im, oss.str(), textOrg, 1, 1, cv::Scalar(255,0,255));
			}
		}


		cv::imshow("Image", Im);
		cv::imshow("Blob", BlobIm);
//		cv::imshow("Test", hsvIm);
		int key = 0xff & cv::waitKey(50);

		if ((key & 255) == 27) break;

	}
	return 0;
}


