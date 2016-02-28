#include "nine_patch_optimize.hpp"
#include <cstdint>
#include "nine_patch_util.hpp"
#include "nine_patch_global_config.hpp"


using namespace cv;
using namespace std;


namespace
{
	bool CopyImageRect(const Mat &src, const cv::Rect &rc_src,
		Mat &dst, const cv::Rect &rc_dst)
	{
		if (rc_src.area() <= 0 ||
			rc_dst.area() <= 0)
		{
			return false;
		}

		src(rc_src).copyTo(dst(rc_dst));
		return true;
	}

	cv::Rect SubPointFromRect(cv::Rect rc, cv::Point pt, bool is_hrz)
	{
		if (!rc.contains(pt))
		{
			return rc;
		}

		if (is_hrz)
		{
			int offset = pt.x - rc.x;
			bool is_left = offset < (rc.width / 2);
			rc.x = is_left ? pt.x : rc.x;
			rc.width = is_left ? (rc.width - offset) : offset;
		}
		else
		{
			int offset = pt.y - rc.y;
			bool is_top = offset < (rc.height / 2);
			rc.y = is_top ? pt.y : rc.y;
			rc.height = is_top ? (rc.height - offset) : offset;
		}

		return rc;
	}

}


PicOpt::Optimize9Grid::Optimize9Grid()
    : max_center_psnr_(0)
	, center_width_(1)
{
}


PicOpt::Optimize9Grid::~Optimize9Grid()
{
}

void PicOpt::Optimize9Grid::SetMaxCenterPSNR(double psnr)
{
	max_center_psnr_ = psnr;
}

void PicOpt::Optimize9Grid::SetCenterRectWidth(int width)
{
	center_width_ = width;
}

bool PicOpt::Optimize9Grid::Optimize(const Mat &src,
	const Vec4i &grid,
	Mat &new_img,
	Vec4i &new_grid)
{
	using namespace Utility;

	GlobalConfig &config = GlobalConfig::GetInstance();
	bool b_ret = false;
	new_img = src;
	Mat out;
	Vec2i new_vec;
	Vec4i tmp_grid = new_grid = grid;
	if (!OptimizeOneDirection(true, src, Vec2i(grid[0], grid[2]), out, new_vec))
	{
		return false;
	}

	tmp_grid[0] = new_vec[0];
	tmp_grid[2] = new_vec[1];
	if (CheckOptResult(src, grid, out, tmp_grid, config.GetOutputQuality()))
	{
		new_grid = tmp_grid;
		new_img = out;
		b_ret = true;
	}


	tmp_grid = new_grid;
	if (!OptimizeOneDirection(false, new_img, Vec2i(grid[1], grid[3]), out, new_vec))
	{
		return b_ret;
	}

	tmp_grid[1] = new_vec[0];
	tmp_grid[3] = new_vec[1];
	if (CheckOptResult(src, grid, out, tmp_grid, config.GetOutputQuality()))
	{
		new_grid = tmp_grid;
		new_img = out;
		b_ret = true;
	}
	return b_ret;
}

bool PicOpt::Optimize9Grid::OptimizeOneDirection(bool is_horizontal,
	const cv::Mat &img,
	const cv::Vec2i &grid,
	cv::Mat &new_img,
	cv::Vec2i &new_grid)
{
	int end_pos = is_horizontal ? img.cols : img.rows;
	int border_sum = grid[1] + grid[0];
	int target_len = end_pos - border_sum;
	if (target_len < 2)
	{
		return false;
	}

	Mat premulti_img = (img.channels() == 4) ? Utility::AlphaBlendWithGray(img) : img;
	Mat img_gray;
	cvtColor(premulti_img, img_gray, COLOR_RGB2GRAY);

	int x_order = is_horizontal ? 1 : 0;
	int y_order = !is_horizontal ? 1 : 0;
	Mat gradient;
	Scharr(img_gray, gradient, CV_16S, x_order, y_order);

	cv::Rect resize_rect(Point(), gradient.size());
	if (is_horizontal)
	{
		resize_rect.x = grid[0];
		resize_rect.width = target_len;
	}
	else
	{
		resize_rect.y = grid[0];
		resize_rect.height = target_len;
	}

	Mat abs_grad;
	convertScaleAbs(gradient, abs_grad);
	Mat &grad_center = abs_grad(resize_rect);

	uint32_t output_quality = GlobalConfig::GetInstance().GetOutputQuality();
	if (target_len == 2)
	{   // for small picture
		if (!NeedOptSmallSize(grad_center, is_horizontal))
		{
			return false;
		}
	}

	new_img = ResizeImageRect(img, resize_rect, is_horizontal, new_grid);
	return true;
}

bool PicOpt::Optimize9Grid::NeedOptSmallSize(const Mat &grad_center, bool is_horizontal)
{
	if (is_horizontal)
	{
		Mat left = grad_center(Rect(0, 0, grad_center.cols / 2, grad_center.rows));
		Mat right = grad_center(Rect(left.cols, 0, grad_center.cols - left.cols, grad_center.rows));
		return Utility::IsMatrixIdentical(left, right);
	}
	else
	{
		Mat top = grad_center(Rect(0, 0, grad_center.cols, grad_center.rows / 2));
		Mat bottom = grad_center(Rect(0, top.rows, grad_center.cols, grad_center.rows - top.rows));
		return Utility::IsMatrixIdentical(top, bottom);
	}
}

Mat PicOpt::Optimize9Grid::ResizeImageRect(const Mat &img,
	const cv::Rect &rc,
	bool is_hrz,
	Vec2i &new_grid)
{
	if (rc.area() <= 0)
	{
		return img;
	}

	auto size = img.size();
	if (is_hrz)
	{
		cv::Rect left(Point(), size);
		cv::Rect center = left;
		cv::Rect right = left;

		int width = (std::min)(rc.width, center_width_);
		left.width = rc.x;
		center.x = rc.x + (rc.width - width) / 2;
		center.width = width;
		right.x = rc.x + rc.width;            // note that the line is [x, y) 
		right.width = size.width - right.x;

		Mat out(size.height, left.width + center.width + right.width, img.type());
		cv::Rect out_center = center;
		out_center.x = left.width;
		cv::Rect out_right = right;
		out_right.x = left.width + center.width;
		CopyImageRect(img, left, out, left);
		CopyImageRect(img, center, out, out_center);
		CopyImageRect(img, right, out, out_right);
		new_grid[0] = left.width;
		new_grid[1] = right.width;
		return out;
	}
	else
	{
		cv::Rect top(Point(), size);
		cv::Rect center = top;
		cv::Rect bottom = top;

		int height = (std::min)(rc.height, center_width_);
		top.height = rc.y;
		center.y = rc.y + (rc.height - height) / 2;
		center.height = height;
		bottom.y = rc.y + rc.height;
		bottom.height = size.height - bottom.y;

		Mat out(top.height + center.height + bottom.height, size.width, img.type());
		cv::Rect out_center = center;
		out_center.y = top.height;
		cv::Rect out_bottom = bottom;
		out_bottom.y = top.height + center.height;
		CopyImageRect(img, top, out, top);
		CopyImageRect(img, center, out, out_center);
		CopyImageRect(img, bottom, out, out_bottom);
		new_grid[0] = top.height;
		new_grid[1] = bottom.height;
		return out;
	}
}
