#include "segmentor.h"

#include <chrono>
#include <iostream>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "helper.h"
#include "warpper.h"
#include "depth_edge_detector.h"
#include "filter.h"
#include "morphsnake.h"

typedef std::chrono::high_resolution_clock::time_point c_time;

using namespace ms;


Segmentor::Segmentor(const Intrinsic_ & color_intrinsic, const Intrinsic_ & depth_intrinsic, const glm::fmat4x4 & d2c_extrinsic){
	depth_intrinsic_ = depth_intrinsic;
	warpper_ = new Warpper(color_intrinsic, depth_intrinsic, d2c_extrinsic);
	d_edge_detector_ = new DepthEdgeDetector();
	filter_ = new Filter();
	ms_ = new MorphSnake();
}

Segmentor::~Segmentor() {
	if (warpper_)
		delete warpper_;

	if (d_edge_detector_)
		delete d_edge_detector_;

	if (filter_)
		delete filter_;

	if (ms_)
		delete ms_;
}

cv::Mat Segmentor::doSegmentation(
	const cv::Mat& color, 
	const cv::Mat& depth, 
	const DepthEdgeParam & de_param, const CannyParam & cn_param, const MorphSnakeParam & ms_param, const InitLevelSetParam& ls_param,
	int downscale, 
	int mask_in_depth_or_color,
	const VisualizationParam& vs_param) {

	// Depth Image To Point Cloud
	// Resizing�� �������� ����, cv::mat�� x,y,z�� 3channel�� ����
	// Depth to Point Cloud �� ���� Depth Image���� �̷�� ����. 
	// ( resizing�� �ϸ� depth intrinsic�� ����� �� ����. )

	cv::Mat pointcloud_mat = cv::Mat::zeros(depth.rows, depth.cols, CV_32FC3);
	pc_helper::depthToPointcloud_Mat(depth, pointcloud_mat, depth_intrinsic_.fx, depth_intrinsic_.fy, depth_intrinsic_.ppx, depth_intrinsic_.ppy);

	cv::Mat resized_color, resized_depth, resized_pc_mat;
	cv::resize(color, resized_color, cv::Size(color.cols / downscale, color.rows / downscale), 0, 0, CV_INTER_NN);
	cv::resize(depth, resized_depth, cv::Size(depth.cols / downscale, depth.rows / downscale), 0, 0, CV_INTER_NN);
	cv::resize(pointcloud_mat, resized_pc_mat, cv::Size(pointcloud_mat.cols / downscale, pointcloud_mat.rows / downscale), 0, 0, CV_INTER_NN);

	c_time h_start = std::chrono::high_resolution_clock::now();
	warpper_->setHomography(resized_color, resized_depth, downscale);
	c_time h_end = std::chrono::high_resolution_clock::now();

	cv::Mat warpped_color = cv::Mat::zeros(resized_depth.rows, resized_depth.cols, CV_8UC3);
	//c_time warp_start = std::chrono::high_resolution_clock::now();
	warpper_->warpRGB_ColorToDepth(resized_color, warpped_color, INTERPOLATION::bilinear);
	//c_time warp_end = std::chrono::high_resolution_clock::now();

	if (vs_param.warpping_on) {
		cv::Mat output;
		cv::resize(warpped_color, output, cv::Size(warpped_color.cols * downscale, warpped_color.rows * downscale), 0, 0, CV_INTER_NN);
		return output;
	}

	// Calculation Edge in Depth Image
	cv::Mat d_edge = cv::Mat::zeros(resized_depth.rows, resized_depth.cols, CV_8UC1);
	//c_time find_edge_start = std::chrono::high_resolution_clock::now();
	d_edge_detector_->findEdge(resized_pc_mat, d_edge, de_param.threshold, de_param.radius);
	//c_time find_edge_end = std::chrono::high_resolution_clock::now();

	if (vs_param.depth_edge_on) {
		cv::imshow("Depth_Edge", d_edge);
	}
	

	// Resized Warpped RGB to Gray (16 bit)
	cv::Mat gray = filter_->RGB2GRAY_16(warpped_color, ms_param.channel);
	cv::Mat canny = filter_->canny(gray, cn_param.low_threshold, cn_param.high_threshold, cn_param.L2gradient);

	if (vs_param.canny_edge_on) {
		cv::imshow("Canny_Edge", canny);
	}

	// Making Inver Edge Map
	// gray, edge img (from depth img)
	// ���ο��� gray img�� �̿��ؼ�, canny edge�� gradient magnitude img�� �����.
	// d_edge (depth edge), canny edge, gradient magnitude�� edge weight�� �̿��� inverse edge map�� �����.
	//c_time iem_start = std::chrono::high_resolution_clock::now();
	cv::Mat inv_edge_map = filter_->inverse_edge_map(d_edge, canny, ms_param.sigma, 3);
	//c_time iem_end = std::chrono::high_resolution_clock::now();


	// Make Init Level Set (Circle)
	cv::Mat init_ls = filter_->make_init_ls({ inv_edge_map.rows , inv_edge_map.cols }, { ls_param.center_row / downscale, ls_param.center_col / downscale }, ls_param.radius);

	//c_time ms_start = std::chrono::high_resolution_clock::now();
	cv::Mat mask = ms_->morphological_geodesic_active_contour(inv_edge_map, canny, init_ls, ms_param.iteration,  ms_param.smoothing, ms_param.ballon);
	//c_time ms_end = std::chrono::high_resolution_clock::now();

#if 1
	std::chrono::duration<double> h_time = h_end - h_start;
	if (h_size > 50) {
		std::cout << "homography " << h_sum / h_size << " seconds.";
		std::cout << std::endl;
		h_sum = 0;
		h_size = 0;
	}
	h_size++;
	h_sum += h_time.count();

	//std::chrono::duration<double> warp_time = warp_end - warp_start;
	//if (warp_size > 50) {
	//	std::cout << "D2PC " << warp_sum / warp_size<< " seconds.";
	//	std::cout << std::endl;
	//	warp_sum = 0;
	//	warp_size = 0;
	//}
	//warp_size++;
	//warp_sum += warp_time.count();

	//std::chrono::duration<double> find_edge_time = find_edge_end - find_edge_start;
	//if (findEdge_size > 50) {
	//	std::cout << "findedge " << findEdge_sum / findEdge_size << " seconds.";
	//	std::cout << std::endl;
	//	findEdge_sum = 0;
	//	findEdge_size = 0;
	//}
	//findEdge_size++;
	//findEdge_sum += find_edge_time.count();

	//std::chrono::duration<double> iem_time = iem_end - iem_start;
	//if (iem_size > 50) {
	//	std::cout << "iem " << iem_sum / iem_size << " seconds.";
	//	std::cout << std::endl;
	//	iem_sum = 0;
	//	iem_size = 0;
	//}
	//iem_size++;
	//iem_sum += iem_time.count();


	//std::chrono::duration<double> ms_time = ms_end - ms_start;
	//if (ms_size > 50) {
	//	std::cout << "ms " << ms_sum / ms_size << " seconds.";
	//	std::cout << std::endl;
	//	ms_sum = 0;
	//	ms_size = 0;
	//}
	//ms_size++;
	//ms_sum += ms_time.count();
#endif



	if (mask_in_depth_or_color == MASK_AT::COLOR) {
		cv::Mat warpped_mask  = cv::Mat::zeros(mask.rows, mask.cols, CV_8UC1);
		warpper_->warpGray_DepthToColor(mask, warpped_mask);

		cv::Mat output;
		cv::resize(warpped_mask, output, cv::Size(warpped_mask.cols * downscale, warpped_mask.rows * downscale), 0, 0, CV_INTER_NN);

		return output;
	}

	// ���� ũ��� ����� �ش�.
	cv::Mat output;
	cv::resize(mask, output, cv::Size(mask.cols * downscale, mask.rows * downscale), 0, 0, CV_INTER_NN);
	return output;
}