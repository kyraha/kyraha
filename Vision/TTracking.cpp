#define _USE_MATH_DEFINES
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <iostream>
#include <list>
#include <algorithm>

static const double CAPTURE_FOCAL = 290;
static const double CameraZeroDist = 130;
static const double CameraHeight = 97-12;

class DataSet : public std::list<float> {
public:
	float GetMedian();
};

class RobotVideo {
public:

	//static const int CAPTURE_COLS=640, CAPTURE_ROWS=480;
	static const int CAPTURE_COLS = 424, CAPTURE_ROWS = 240;
	static const int CAPTURE_PORT = 0;
	static const int MIN_AREA = 135; // Min area in pixels, 3*(25+40+25) is a rough estimate
	static const int MAX_TARGETS = 2;

private:
	std::vector<std::vector<cv::Point>> m_boxes;
	std::vector<double> m_turns;

	RobotVideo(RobotVideo const&);
	RobotVideo &operator =(RobotVideo const&);
public:
	RobotVideo();
	int mutex_lock() { return 1; };
	int mutex_unlock() { return 1; };
	void ProcessContours(std::vector<std::vector<cv::Point>> contours);

	// These guys need mutex locked but user should do that so can wrap them in a bunch
	size_t HaveHeading() { return m_boxes.size(); };
	float GetTurn(size_t i = 0);
	float GetDistance(size_t i = 0);
};

RobotVideo::RobotVideo()
	: m_boxes()
	, m_turns(MAX_TARGETS)
{
}

float RobotVideo::GetDistance(size_t i)
{
	// Distance by the height. Farther an object with known height lower it appears on the image.
	// The real height of the target is 97 inches.
	// Camera is 12 inches above the floor.
	float dy = (m_boxes[i][1].y + m_boxes[i][0].y - CAPTURE_ROWS) / 2.0;
	float tower = CameraHeight;
	float alpha = atan2f(tower, CameraZeroDist);
	return tower / tanf(alpha - atan2f(dy, CAPTURE_FOCAL));
}

float RobotVideo::GetTurn(size_t i)
{
	return m_turns[i];
}


/**
* \brief Stencil is a simplified "contour" that is an "ideal" shape that we're looking for.
*
* The stencil consists of 8 points - the 2d corners of the vision target how it would
* appear in the picture. The origin is in the left-top corner.
*/
static const 	std::vector<cv::Point> stencil = {
	{ 32, 0 },
	{ 26, 76 },
	{ 184, 76 },
	{ 180, 0 },
	{ 203, 0 },
	{ 212, 100 },
	{ 0, 100 },
	{ 9, 0 }
};


float DataSet::GetMedian()
{
	if (size() > 2) {
		std::vector<float> ord;
		for (float dp : *this) ord.push_back(dp);
		std::sort(ord.begin(), ord.end());
		return ord[ord.size() / 2];
	}
	else if (size() > 0) return *(this->begin());
	else return 0;
}

/*
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
*/

std::vector<cv::Point3f> objectPoints;


cv::Vec4f CalculateLocation(std::vector<cv::Point> target)
{
	/* Logitech camera, millimeters */
	cv::Matx33d camera_matrix(
	4.9410254557831900e+002, 0, 3.7487505597946938e+002,
	0, 4.9180258750779660e+002, 2.0903256450552996e+002,
	0, 0, 1);
	cv::Matx<double, 5, 1> distortion_coefficients(
	-2.0994774118936483e-002,
	4.9899418109244344e-002,
	-6.7418809421477795e-004,
	-5.2140053606489992e-004,
	-6.9063255104469826e-002);


	/* Microsoft HD3000 camera, inches 
	cv::Matx33d camera_matrix(
		7.4230925920305481e+002, 0., 3.0383585011521706e+002, 0.,
		7.4431328863404576e+002, 2.3422929172706634e+002, 0., 0., 1.);
	cv::Matx<double, 5, 1> distortion_coefficients(
		2.0963551753568421e-001, -1.4796846132520820e+000, 0., 0., 2.7677879392937270e+000);
		*/

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
	std::vector<cv::Point2f> imagePoints(4);
	imagePoints[0] = hull[0];
	imagePoints[1] = hull[1];
	imagePoints[2] = hull[2];
	imagePoints[3] = hull[3];

	cv::Vec3d rvec, tvec;
	cv::Matx33d Rmat;

	cv::solvePnP(objectPoints, imagePoints, camera_matrix, distortion_coefficients, rvec, tvec, false, CV_EPNP);
	cv::Rodrigues(rvec, Rmat);

	cv::Vec3f location = -(Rmat.t() * tvec);
	return cv::Vec4f(location[0], location[1], location[2], 0.5*(hull[0].x + hull[1].x));
}

/** \brief Process the list of found contours for FIRSTSTRONGHOLD
*
* This is the main logic for target processing in regards to FRC Stronghold game of 2016
* The idea is to go through the list and rate each contour by the similarity with U-shaped vision target.
* Then chose the top two by the rating since only two targets can be seen at a time.
* This function updates the object's member data that can be queried later from outside.
* \param contours Vector of contours. Where a contour is a Vector of Points
*/
void RobotVideo::ProcessContours(std::vector<std::vector<cv::Point>> contours) {

	// To rearrange the set of random contours into a rated list of targets
	// we're going to need a new vector that we can sort
	struct Target {
		double rating;
		std::vector<cv::Point> contour;
	};
	std::vector<struct Target> targets;

	for (std::vector<cv::Point> cont : contours)
	{
		// Only process a contour if it is big enough, otherwise it's either too far away or just a noise
		if (cv::contourArea(cont) > MIN_AREA) {
			double similarity = cv::matchShapes(stencil, cont, CV_CONTOURS_MATCH_I3, 1);

			// Less the similarity index closer the contour matches the stencil shape
			// We are interested only in very similar ones
			if (similarity < 4.0) {
				if (targets.empty()) {
					// When we just started the first contour is our best candidate
					targets.push_back({ similarity, cont });
				}
				else {
					bool found = false;
					for (std::vector<struct Target>::iterator it = targets.begin(); it != targets.end(); ++it) {
						// Run through all targets we have found so far and find the position where to insert the new one
						if (similarity < it->rating) {
							targets.insert(it, { similarity, cont });
							found = true;
							break;
						}
					}
					if (!found) targets.push_back({ similarity, cont });

					// If there are too many targets after the insert pop the last one
					if (targets.size() > MAX_TARGETS) targets.pop_back();
				}
			}
		}
	}

	float zenith = CAPTURE_FOCAL*CameraZeroDist / CameraHeight;
	float meridian = CAPTURE_FOCAL*CameraHeight / CameraZeroDist + zenith;
	// Now as we have the top MAX_TARGETS contours qualified as targets
	// Extract four point from each contour that describe the rectangle and store the results
	std::vector<std::vector<cv::Point>> boxes;
	std::vector<double> turns;
	for (struct Target target : targets) {
		//Extract 4 corner points assuming the blob is a rectangle, more or less horizontal
		std::vector<cv::Point> hull(4);
		hull[0] = cv::Point(10000, 10000);		// North-West
		hull[1] = cv::Point(0, 10000);			// North-East
		hull[2] = cv::Point(0, 0);				// South-East
		hull[3] = cv::Point(10000, 0);			// South-West
		for (cv::Point point : target.contour) {
			if (hull[0].x + hull[0].y > point.x + point.y) hull[0] = point;
			if (hull[1].y - hull[1].x > point.y - point.x) hull[1] = point;
			if (hull[2].x + hull[2].y < point.x + point.y) hull[2] = point;
			if (hull[3].x - hull[3].y > point.x - point.y) hull[3] = point;
		}

		float dX = CAPTURE_COLS/2.0 - 0.5*(hull[0].x + hull[1].x);
		float dY = zenith + 0.5*(hull[0].y + hull[1].y);
		float azimuth = meridian*dX / dY;
		double real_angle = atan2(azimuth, CAPTURE_FOCAL); // Preferences::GetInstance()->GetFloat("CameraFocal", CAPTURE_FOCAL));

		turns.push_back(real_angle * 180 / M_PI); // +Preferences::GetInstance()->GetFloat("CameraBias", 0));
		boxes.push_back(hull);
	}

	if (MAX_TARGETS == 2 && boxes.size() == 2) {
		if (boxes.front().front().x > boxes.back().front().x) {
			boxes.front().swap(boxes.back());
			double tmp = turns.front();
			turns.front() = turns.back();
			turns.back() = tmp;

		}
	}

	// To avoid misreading of our data from other threads we lock our mutex
	// before updating the shared data and then unlock it when done.
	mutex_lock();
	m_boxes = boxes;
	m_turns = turns;
	mutex_unlock();
}

int main(int argc, char** argv)
{
	cv::Vec3i BlobLower(65, 100, 65);
	cv::Vec3i BlobUpper(90, 255, 255);
	RobotVideo robot;

	std::string filename = "";
	std::vector<std::string> files = {
		"2016-04-07 09-25-03.626.jpg",
		"2016-04-07 09-25-09.525.jpg",
		"2016-04-07 09-25-34.682.jpg",
		"2016-04-07 09-25-37.192.jpg",
		"2016-04-07 09-25-54.888.jpg",
		"2016-04-07 09-26-37.143.jpg",
		"2016-04-07 09-26-58.278.jpg",
		"2016-04-07 09-27-11.157.jpg",
		"2016-04-07 09-27-23.921.jpg",
		"2016-04-07 09-27-39.722.jpg",
		"2016-04-07 09-28-04.714.jpg",
		"2016-04-07 09-28-42.974.jpg",
		"2016-04-07 09-29-25.497.jpg",
		"2016-04-07 09-32-19.042.jpg",
		"2016-04-07 09-32-28.667.jpg",
		"2016-04-07 09-32-33.610.jpg",
		"2016-04-07 09-33-14.309.jpg",
		"2016-04-07 09-33-41.161.jpg",
		"2016-04-07 09-35-33.208.jpg",
		"2016-04-07 09-35-42.301.jpg",
		"2016-04-07 09-35-58.840.jpg",
		"2016-04-07 09-36-14.463.jpg",
		"2016-04-07 09-36-45.076.jpg",
		"2016-04-07 09-36-58.870.jpg"
	};
	std::vector<std::string>::iterator p_file = files.begin();
	if (argc > 1) filename = argv[1];
	else filename = *p_file++;

#ifdef CAMERA_FEED
	cv::VideoCapture capture;
	capture.open(0);
	if (!capture.isOpened()) {
		std::cerr << "Could not initialize video capture" << std::endl;
		return -2;
	}
	//After Opening Camera we need to configure the returned image setting
	//all opencv v4l2 camera controls scale from 0.0 - 1.0
#endif

	cv::Mat Im;
	cv::Mat hsvIm;
	cv::Mat BlobIm;
	cv::namedWindow("Image", CV_WINDOW_AUTOSIZE);
	cv::namedWindow("Blob", CV_WINDOW_AUTOSIZE);

	/* Real FIRSTSTRONGHOLD tower, inches 
	objectPoints.push_back(cv::Point3d(-10, 0, 0));
	objectPoints.push_back(cv::Point3d(10, 0, 0));
	objectPoints.push_back(cv::Point3d(10, 14, 0));
	objectPoints.push_back(cv::Point3d(-10, 14, 0));
	*/

	/* Whiteboard res target, millimeters */
	objectPoints.push_back(cv::Point3d(-135, 0, 0));
	objectPoints.push_back(cv::Point3d(135, 0, 0));
	objectPoints.push_back(cv::Point3d(135, 150, 0));
	objectPoints.push_back(cv::Point3d(-135, 150, 0));

	cv::createTrackbar("H. Lower", "Blob", &(BlobLower[0]), 255);
	cv::createTrackbar("H. Upper", "Blob", &(BlobUpper[0]), 255);
	cv::createTrackbar("S. Lower", "Blob", &(BlobLower[1]), 255);
	cv::createTrackbar("S. Upper", "Blob", &(BlobUpper[1]), 255);
	cv::createTrackbar("V. Lower", "Blob", &(BlobLower[2]), 255);
	cv::createTrackbar("V. Upper", "Blob", &(BlobUpper[2]), 255);

	cv::Point textOrg;
	DataSet locationsX;
	DataSet locationsY;
	DataSet locationsZ;
	DataSet locationsA;

	cv::Moments stMoments = cv::moments(stencil);
	double stHu[7];
	cv::HuMoments(stMoments, stHu);

	for (; true;) {
		int key = 0xff & cv::waitKey(50);
		if ((key & 255) == 27) break;
		if ((key & 255) == 32 && p_file != files.end()) filename = *p_file++;

#ifdef CAMERA_FEED
		capture >> Im;
#else
		//Im = cv::imread("C:\\Users\\U0172740\\Documents\\Visual Studio 2013\\Projects\\TargetTracking\\x64\\Debug\\Pit.PNG");
		Im = cv::imread(filename);
#endif

		if (Im.empty()) {
			std::cout << " Error reading from camera" << std::endl;
			int key = 0xff & cv::waitKey(2000);
			if ((key & 255) == 27) break;
			else continue;
		}
		cv::cvtColor(Im, hsvIm, CV_BGR2HSV);
		cv::inRange(hsvIm, BlobLower, BlobUpper, BlobIm);

		textOrg.x = 10;
		textOrg.y = 10;
		std::ostringstream oss;
		for(double *p=stHu; p < stHu+7; ++p) oss << (int)(100.0**p)/100.0 << ", ";
		cv::putText(Im, oss.str(), textOrg, 1, 1, cv::Scalar(60, 60, 255), 1);
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
			robot.ProcessContours(contours);
			double sim1 = 1000.0;
			for (std::vector<cv::Point> cont : contours)
			{
				// Only process cont if it is big enough, otherwise it's either too far or just a noise
				if (cv::contourArea(cont) > 135) {
					cv::RotatedRect rect = minAreaRect(cont);
					double similarity = cv::matchShapes(stencil, cont, CV_CONTOURS_MATCH_I3, 1);
					if (similarity < sim1)
					{
						sim1 = similarity;
					}
					textOrg.x = cont[0].x;
					textOrg.y = cont[0].y;
					std::ostringstream oss;
					oss << 0.01*((int)(100*similarity));
					cv::putText(Im, oss.str(), textOrg, 1, 1, cv::Scalar(0, 200, 55), 1);
					cv::Point2f vertices[4]; rect.points(vertices);
					for (int i = 0; i < 4; i++)	cv::line(Im, vertices[i], vertices[(i + 1) % 4], cv::Scalar(200, 55, 55));
				}
			}
			if (robot.HaveHeading()) {
				std::ostringstream oss1, oss2;
				cv::Scalar ossColor(260, 0, 255);
				if (robot.HaveHeading() > 0) {
					oss1 << "Turn: ";
					oss2 << "Dist: ";
					if (robot.HaveHeading() > 1) {
						oss1 << robot.GetTurn(0) << " : " << robot.GetTurn(1);
						oss2 << robot.GetDistance(0) << " : " << robot.GetDistance(1);
					}
					else {
						oss1 << robot.GetTurn();
						oss2 << robot.GetDistance();
					}
				}
				else {
					oss1 << "No target";
					oss2 << "No target";
					ossColor = cv::Scalar(0, 100, 255);
				}
				cv::putText(Im, oss1.str(), cv::Point(20, robot.CAPTURE_ROWS - 32), 1, 1, ossColor, 1);
				cv::putText(Im, oss2.str(), cv::Point(20, robot.CAPTURE_ROWS - 16), 1, 1, ossColor, 1);

			}
		}


		cv::imshow("Image", Im);
		cv::imshow("Blob", BlobIm);
	}
	return 0;
}



