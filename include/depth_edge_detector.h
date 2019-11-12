#pragma once
#include <opencv2/core.hpp>

class DepthEdgeDetector {
public:
	DepthEdgeDetector() {}
	~DepthEdgeDetector() {}

public:
	// Find Edge in PointCloud Image ( Point Cloud�� OpenCV 3Channel�� ����Ǿ� �ִ� ���� )
	cv::Mat findEdge(const cv::Mat & pc_image, cv::Mat& edge_map, float threshold, int radius);
};