#ifndef NINE_PATCH_GEN_HELPER
#define NINE_PATCH_GEN_HELPER

#include <opencv2/imgproc/imgproc.hpp>

namespace PicOpt
{
	class Gen9Patch
	{
	public:
		Gen9Patch();
        ~Gen9Patch();

		// out[0-3] = left, top, right, bottom
		bool Get9PatchLines(const cv::Mat &src, cv::Vec4i &out);
		bool Get9PatchLinesWithOldPatch(const cv::Mat &src, const cv::Vec4i &in, cv::Vec4i &out);

	private:
		bool GetEdgeLineSobel(const cv::Mat &src,
			bool is_horizontal,
			cv::Vec2i &out);
	};
}

#endif