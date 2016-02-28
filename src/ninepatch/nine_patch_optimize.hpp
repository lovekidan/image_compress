#ifndef NINE_PATCH_OPTIMIZE
#define NINE_PATCH_OPTIMIZE

#include <imgproc/imgproc.hpp>

namespace PicOpt
{
	class Optimize9Grid
	{
	public:
		Optimize9Grid();
		~Optimize9Grid();

		void SetMaxCenterPSNR(double psnr);
		void SetCenterRectWidth(int width);

		// grid[0-3] = left, top, right, bottom
		bool Optimize(const cv::Mat &src,
			const cv::Vec4i &grid,
			cv::Mat &new_img,
			cv::Vec4i &new_grid);

	private:
		bool OptimizeOneDirection(bool is_horizontal,
			const cv::Mat &img,
			const cv::Vec2i &grid,
			cv::Mat &new_img,
			cv::Vec2i &new_grid);

		bool NeedOptSmallSize(const cv::Mat &grad_center, bool is_horizontal);
		cv::Mat ResizeImageRect(const cv::Mat &img,
			const cv::Rect &rc,
			bool is_hrz,
			cv::Vec2i &new_grid);

		double max_center_psnr_;
		int center_width_;
	};
}


#endif