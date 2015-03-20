#include "stdafx.h"

cv::Mat capture_img;
cv::Mat canny_img;
cv::Mat dilate_img;
cv::Mat inv_img;
cv::Mat canvas_img;

bool press_button_l = false;
int mouse_x;
int mouse_y;

cv::Rect correct_rect(const cv::Rect &src, const cv::Size &size)
{
	cv::Rect r = src;

	if (r.x < 0) r.x = 0;
	if (r.y < 0) r.y = 0;
	if (size.width - 1 < r.x) r.x = size.width - 1;
	if (size.height - 1 < r.y) r.y = size.height - 1;

	if (r.width <= 0) r.width = 1;
	if (r.height <= 0) r.height = 1;
	if (size.width < r.x + r.width) r.width = size.width - r.x;
	if (size.height < r.y + r.height) r.height = size.height - r.y;

	return r;
}

void onMouse(int event, int x, int y, int, void*)
{
	if (capture_img.empty()) return;

	mouse_x = x;
	mouse_y = y;

	if (event == cv::EVENT_LBUTTONDOWN) {
		press_button_l = true;
	}
	else if (event == cv::EVENT_LBUTTONUP) {
		press_button_l = false;
	}
}

cv::Rect create_roi(int x, int y, int w)
{
	cv::Rect roi;

	roi.x = x - w / 2;
	roi.y = y - w / 2;
	roi.width = w;
	roi.height = w;

	roi = correct_rect(roi, canvas_img.size());

	return roi;
}

cv::Mat create_mask(const cv::Rect &roi)
{
	cv::Mat mask(roi.size(), CV_8UC1);

	int r = roi.width / 2;
	if (r < roi.height / 2) r = roi.height / 2;

	mask = 0;
	cv::circle(mask, cv::Point(roi.width / 2, roi.height / 2), r, cv::Scalar(255), -1);

	return mask;
}

int main(int argc, char* argv[])
{
	cv::VideoCapture capture;
	bool rv = capture.open(0);
	if (rv == false) {
		printf("error : capture.open() failed...\n");
		return -1;
	}

	capture >> capture_img;
	canvas_img.create(capture_img.size(), CV_8UC1);
	canvas_img = cv::Scalar(255, 255, 255);

	cv::namedWindow("canvas_img");
	cv::setMouseCallback("canvas_img", onMouse, NULL);

	while (true) {
		capture >> capture_img;
		cv::Canny(capture_img, canny_img, 100, 300);
		cv::dilate(canny_img, dilate_img, cv::Mat());
		cv::bitwise_not(dilate_img, inv_img);

		if (press_button_l == true) {
			cv::Rect roi = create_roi(mouse_x, mouse_y, 30);
			cv::Mat mask = create_mask(roi);
			inv_img(roi).copyTo(canvas_img(roi), mask);
		}

		cv::imshow("captuer_img", capture_img);
		cv::imshow("canvas_img", canvas_img);

		int c = cv::waitKey(1);
		if (c == 27) {
			break;
		}
		else if (c == 'c') {
			canvas_img = cv::Scalar(255, 255, 255);
		}
	}

	capture.release();
	cv::destroyAllWindows();

}

