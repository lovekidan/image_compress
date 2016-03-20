#ifndef NINE_PATCH_OPTIMIZE
#define NINE_PATCH_OPTIMIZE

#include <opencv2/imgproc/imgproc.hpp>

namespace PicOpt
{
	class Optimize9Patch
	{
	public:
		Optimize9Patch();
		~Optimize9Patch();

		void SetMaxCenterPSNR(double psnr);
		void SetCenterRectWidth(int width);

		// patch[0-3] = left, top, right, bottom
		bool Optimize(const cv::Mat &src,
			const cv::Vec4i &patch,
			cv::Mat &new_img,
			cv::Vec4i &new_patch);

	private:
		bool OptimizeOneDirection(bool is_horizontal,
			const cv::Mat &img,
			const cv::Vec2i &patch,
			cv::Mat &new_img,
			cv::Vec2i &new_patch);

		bool NeedOptSmallSize(const cv::Mat &grad_center, bool is_horizontal);
		cv::Mat ResizeImageRect(const cv::Mat &img,
			const cv::Rect &rc,
			bool is_hrz,
			cv::Vec2i &new_patch);

		double max_center_psnr_;
		int center_width_;
	};
}


#endif