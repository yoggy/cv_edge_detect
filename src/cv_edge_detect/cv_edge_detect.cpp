#include "stdafx.h"
#include "FPSCounter.h"

cv::VideoCapture capture;
cv::Mat capture_img;
cv::Mat canny_img;
cv::Mat dilate_img;
cv::Mat inv_img;
cv::Mat canvas_img;

cv::Mat test_pattern_img;
bool use_test_pattern_img = false;

bool press_button_l = false;
cv::Point mouse_pos, old_mouse_pos;

#define OSC_HOST "127.0.0.1"
#define OSC_PORT 7000
#define OSC_OUTPUT_BUFFER_SIZE 1024

UdpTransmitSocket *udp = nullptr;
char osc_buf[OSC_OUTPUT_BUFFER_SIZE];
osc::OutboundPacketStream osc_msg(osc_buf, OSC_OUTPUT_BUFFER_SIZE);

FPSCounter fps_mouse("mouse", 100, true);
FPSCounter fps_capture("capture", 100, true);

void osc_send_pop()
{
	osc_msg.Clear();
	osc_msg << osc::BeginMessage("/pop");
	osc_msg << osc::EndMessage;
	udp->Send(osc_msg.Data(), osc_msg.Size());
}

void osc_send_scratch(const float &volume)
{
	osc_msg.Clear();
	osc_msg << osc::BeginMessage("/scratch");
	osc_msg << volume;
	osc_msg << osc::EndMessage;
	udp->Send(osc_msg.Data(), osc_msg.Size());
}

bool is_inner(const cv::Point &p, const cv::Size & size)
{
	if (p.x < 0) return false;
	if (p.y < 0) return false;
	if (size.width <= p.x) return false;
	if (size.height <= p.y) return false;

	return true;
}

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

cv::Rect create_roi(const int &x, const int &y, const int &w)
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

void process_pseudo_pen_drawing(cv::Mat &canvas_img, const cv::Mat &src_img, const int &x, const int &y)
{
	// drawing effect
	cv::Rect roi = create_roi(x, y, 30);
	cv::Mat mask = create_mask(roi);

	cv::Mat roi_img;
	cv::Mat pen_img;

	src_img(roi).copyTo(roi_img);

	pen_img = roi_img * 0.90;

	pen_img.copyTo(canvas_img(roi), mask);
}

void process_pseudo_frottage(const cv::Mat &src_img, cv::Mat &canvas_img)
{
	if (press_button_l == false) {
		osc_send_scratch(0.0);
		return;
	}

	cv::Point st = old_mouse_pos;
	cv::Point et = mouse_pos;
	cv::Point diff = et - st;
	float diff_len = (float)sqrt(diff.x * diff.x + diff.y * diff.y);

	int step = abs(diff.x);
	if (step < abs(diff.y)) step = abs(diff.y);

	if (step == 0) {
		osc_send_scratch(0.0f);
		return;
	}

	// drawing
	for (float p = 0.0f; p <= 1.0f; p += (1.0f / (float)step * 10)) {
		float x = st.x + diff.x * p;
		float y = st.y + diff.y * p;
		process_pseudo_pen_drawing(canvas_img, src_img, (int)x, (int)y);
	}

	// sound effect (noise)
	float p = diff_len / 200;
	if (p > 0.8f) p = 0.8f;
	p += 0.2f;
	osc_send_scratch(p);

	// sound effect (pop noise)
	int c_min = 255;
	int c_max = 0;

	for (float p = 0.0f; p <= 1.0f; p += (1.0f / (float)step)) {
		cv::Point pt = cv::Point((int)(st.x + diff.x * p), (int)(st.y + diff.y * p));
		if (is_inner(pt, src_img.size()) == false) continue;

		uchar c = src_img.at<uchar>(pt);
		if (c <= c_min) {
			c_min = c;
		}
		if (c_max <= c) {
			c_max = c;
		}
	}
	if (c_max - c_min > 100) {
		osc_send_pop();
	}
}

cv::Mat get_source_image() {
	if (use_test_pattern_img) {
		return test_pattern_img;
	}

	return inv_img;
}

void onMouse(int event, int x, int y, int, void*)
{
	if (capture_img.empty()) return;

	old_mouse_pos = mouse_pos;
	mouse_pos = cv::Point(x, y);

	if (event == cv::EVENT_LBUTTONDOWN && press_button_l == false) {
		press_button_l = true;
		fps_mouse.clear();
		old_mouse_pos = cv::Point(x, y);
	}
	else if (event == cv::EVENT_LBUTTONUP) {
		press_button_l = false;
		osc_send_scratch(0.0);
	}

	if (press_button_l) {
		fps_mouse.check();
		process_pseudo_frottage(get_source_image(), canvas_img);
	}
}

void process_capture()
{
	capture >> capture_img;
	cv::Canny(capture_img, canny_img, 100, 300);
	cv::dilate(canny_img, dilate_img, cv::Mat());
	cv::bitwise_not(dilate_img, inv_img);
}

void init_osc() {
	udp = new UdpTransmitSocket(IpEndpointName(OSC_HOST, OSC_PORT));
	osc_send_scratch(0.0);
}

void finish_osc() {
	osc_send_scratch(0.0f);
	delete udp;
	udp = nullptr;
}

void init_opencv() {
	capture >> capture_img;
	canvas_img.create(capture_img.size(), CV_8UC1);
	canvas_img = cv::Scalar(255, 255, 255);

	test_pattern_img.create(capture_img.size(), CV_8UC1);
	test_pattern_img = cv::Scalar(255, 255, 255);
	int w = test_pattern_img.cols;
	int h = test_pattern_img.rows;
	cv::line(test_pattern_img, cv::Point(w / 3, 0), cv::Point(w / 3, h), cv::Scalar(0), 3);
	cv::line(test_pattern_img, cv::Point(w / 3 * 2, 0), cv::Point(w / 3 * 2, h), cv::Scalar(0), 3);

	cv::namedWindow("canvas_img");
	cv::setMouseCallback("canvas_img", onMouse, NULL);
}

void finish_opencv() {
	capture.release();
	cv::destroyAllWindows();
}

int main(int argc, char* argv[])
{
	bool rv = capture.open(0);
	if (rv == false) {
		printf("error : capture.open() failed...\n");
		return -1;
	}

	init_osc();
	init_opencv();

	while (true) {
		process_capture();

		cv::imshow("captuer_img", capture_img);
		cv::imshow("source_image", get_source_image());
		cv::imshow("canvas_img", canvas_img);

		int c = cv::waitKey(1);
		if (c == 27) {
			break;
		}
		else if (c == 'c') {
			canvas_img = cv::Scalar(255, 255, 255);
		}
		else if (c == 't') {
			use_test_pattern_img = !use_test_pattern_img;
		}

		fps_capture.check();
	}

	finish_opencv();
	finish_osc();

	return 0;
}
