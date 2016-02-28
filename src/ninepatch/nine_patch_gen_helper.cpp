#include "nine_patch_gen_helper.hpp"
#include "nine_patch_util.hpp"

using namespace cv;
using namespace std;

PicOpt::Gen9Grid::Gen9Grid()
{
}


PicOpt::Gen9Grid::~Gen9Grid()
{
}

bool PicOpt::Gen9Grid::Get9GirdLines(const Mat &src, Vec4i &out)
{
	int left = src.cols / 2 - 1;
	int top = src.rows / 2 - 1;
	return Get9GirdLinesWithOldGrid(src, Vec4i(left, top, left, top), out);
}

bool PicOpt::Gen9Grid::Get9GirdLinesWithOldGrid(const Mat &src, const Vec4i &in, Vec4i &out)
{
	Mat src_blend = (src.channels() == 4) ? Utility::AlphaBlendWithGray(src) : src;
	Mat src_gray;
	cvtColor(src_blend, src_gray, COLOR_RGB2GRAY);

	Vec2i hrz_line(in[0], src.cols - in[2] - 1);
	Vec2i vrt_line(in[1], src.rows - in[3] - 1);
	GetEdgeLineSobel(src_gray, true, hrz_line);
	GetEdgeLineSobel(src_gray, false, vrt_line);

	out[0] = hrz_line[0];
	out[1] = vrt_line[0];
	out[2] = src.cols - hrz_line[1] - 1;
	out[3] = src.rows - vrt_line[1] - 1;

	return true;
}

bool PicOpt::Gen9Grid::GetEdgeLineSobel(const Mat &src, bool is_horizontal, Vec2i &out)
{
	int x_order = is_horizontal ? 1 : 0;
	int y_order = !is_horizontal ? 1 : 0;
	Mat grad;
	Scharr(src, grad, CV_16S, x_order, y_order);

	Mat abs_grad;
	convertScaleAbs(grad, abs_grad);

	const int len = is_horizontal ? src.size().width : src.size().height;
	Vec2i line(0, len - 1);
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(abs_grad, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
	for_each(contours.cbegin(), contours.cend(), [&](const vector<Point> &contour)
	{
		for_each(contour.cbegin(), contour.cend(), [&](const cv::Point &pt)
		{
			int target = is_horizontal ? pt.x : pt.y;
			if (target <= out[0] && target > line[0])
			{
				line[0] = target;
			}
			else if (target >= out[1] && target < line[1])
			{
				line[1] = target;
			}
		});
	});

	if (line[1] - line[0] == 2)
	{   // for line_length == convolution kernel_size
		line[0] += 1;
		line[1] -= 1;
	}
	out = line;
	return true;
}
