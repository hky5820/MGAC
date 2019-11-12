#pragma once
#include <opencv2/core.hpp>

#include <glm/glm.hpp>

namespace ms {
	enum INTERPOLATION {
		bilinear = 0
	};


	enum CHANNEL {
		RED = 0,
		GREEN,
		BLUE,
		GRAY
	};

	enum MASK_AT {
		COLOR = 0,
		DEPTH
	};

	struct DepthEdgeParam {
		DepthEdgeParam() {};

		DepthEdgeParam(
			float threshold_,
			int radius_) :
			threshold(threshold_),
			radius(radius_) {}

		// [Coplanarity Check] : || A - X || + || B - X || ) / || A - B ||  ���� threshold,
		// threshold�� ��������, ���� ũ�� ���̴� line���� ������ �ִ� point�鸸 ����ǰ� ��
		// range : 1~2 ����
		float threshold;
		int radius; // Coplanarity Check �� ��, neighborhood�� ����
	};

	struct MorphSnakeParam {
		MorphSnakeParam() {};
		MorphSnakeParam(
			double sigma_,
			int channel_,
			int iteration_,
			int smoothing_,
			int ballon_) :
			sigma(sigma_),
			channel(channel_),
			iteration(iteration_),
			smoothing(smoothing_),
			ballon(ballon_) {}
		double sigma;  // Gaussian�� ���Ǵ� sigma
		int channel;   // RGB���� Gray�� �����, ���� ä�� ����(first,second,third) Ȥ�� OpenCV cvtColor �Լ� ( GRAY )
		int iteration;
		int smoothing; // smoothing Ƚ��
		int ballon;    // 1 or -1 ( ��â or ��� )
	};

	struct CannyParam {
		CannyParam() {};
		CannyParam(
			int low_threshold_,
			int high_threshold_,
			bool L2gradient_) :
			low_threshold(low_threshold_),
			high_threshold(high_threshold_),
			L2gradient(L2gradient_) {}

		int low_threshold;
		int high_threshold;
		bool L2gradient;
		// a flag, indicating whether a more accurate L2 norm =sqrt((dI/dx)2+(dI/dy)2) should be used to calculate the image gradient magnitude ( L2gradient=true ),
		// or whether the default L1 norm =|dI/dx|+|dI/dy| is enough ( L2gradient=false ).
	};

	struct InitLevelSetParam {
		InitLevelSetParam() {};
		InitLevelSetParam(
			int center_row_,
			int center_col_,
			int radius_) :
			center_row(center_row_),
			center_col(center_col_),
			radius(radius_) {}

		int center_row;
		int center_col;
		int radius;
	};

	struct VisualizationParam {
		VisualizationParam(bool depth_edge_on_, bool canny_edge_on_, bool warpping_on_) :
			depth_edge_on(depth_edge_on_), canny_edge_on(canny_edge_on_), warpping_on(warpping_on_) {}
		bool depth_edge_on;
		bool canny_edge_on;
		bool warpping_on;
	};

	struct Intrinsic_ {
		Intrinsic_() {};
		Intrinsic_(
			float fx_,
			float fy_,
			float ppx_,
			float ppy_,
			int width_,
			int height_) :
			fx(fx_),
			fy(fy_),
			ppx(ppx_),
			ppy(ppy_),
			width(width_),
			height(height_) {}
		float fx;
		float fy;
		float ppx;
		float ppy;
		int width;
		int height;
	};
};