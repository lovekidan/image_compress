#ifndef NINE_PATCH_UTIL
#define NINE_PATCH_UTIL

#include <time.h>
#include <cstdint>
#include <utility>
#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>
#undef max

using namespace std;
using namespace cv;

namespace PicOpt
{
	namespace Utility
	{
		inline unsigned long GetTickCount()
		{
			struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
		}

		static cv::RNG kRandom(GetTickCount());

		inline cv::Mat ZoomIn(const cv::Mat &img, double ratio)
		{
			if (ratio == 1)
			{
				return img;
			}

			cv::Mat resized;
			cv::resize(img,
				resized,
				cv::Size(int(img.size().width * ratio), int(img.size().height * ratio)),
				0,
				0,
				cv::INTER_NEAREST);
			return resized;
		}

		inline cv::Scalar RandomColor()
		{
			return cv::Scalar(kRandom.uniform(0, 255), kRandom.uniform(0, 255), kRandom.uniform(0, 255));
		}

		inline bool IsHorizontal(const cv::Vec4i &line)
		{
			return abs(line[1] - line[3]) < abs(line[0] - line[2]);
		}

		inline void Regular(cv::Vec4i &line, bool bHorizontal)
		{
			if ((bHorizontal && line[0] > line[2]) ||
				(!bHorizontal && line[1] > line[3]))
			{
				std::swap(line[0], line[2]);
				std::swap(line[1], line[3]);
			}
		}

		template<typename _Ty>
		inline bool IsGridValid(const _Ty &grid)
		{
			for (size_t i = 0; i < _Ty::channels; ++i)
			{
				if (grid[i] > 0)
				{
					return true;
				}
			}

			return false;
		}

		template<typename _Ty>
		inline void VecMin(const _Ty &l, const _Ty &r, _Ty &out)
		{
			for (size_t i = 0; i < _Ty::channels; ++i)
			{
				out[i] = std::min(l[i], r[i]);
			}
		}

		inline bool IsThreeGrid(const cv::Vec4i &grid)
		{
			return (grid[0] == 0 && grid[1] > 0 && grid[2] == 0 && grid[3] > 0) ||
				(grid[0] > 0 && grid[1] == 0 && grid[2] > 0 && grid[3] == 0);
		}

		/*
		*   from http://docs.opencv.org/modules/core/doc/basic_structures.html
		*/
		template<typename T>
		void AlphaBlendRGBA(const cv::Mat& src1, const cv::Mat& src2, cv::Mat& dst)
		{
			const float alpha_scale = (float)std::numeric_limits<T>::max();
			const float inv_scale = 1.f / alpha_scale;

			CV_Assert(src1.type() == src2.type());
			CV_Assert(src1.type() == CV_MAKETYPE(DataType<T>::depth, 4));
			CV_Assert(src1.size() == src2.size());
			Size size = src1.size();
			dst.create(size, src1.type());

			// here is the idiom: check the arrays for continuity and,
			// if this is the case,
			// treat the arrays as 1D vectors
			if (src1.isContinuous() && src2.isContinuous() && dst.isContinuous())
			{
				size.width *= size.height;
				size.height = 1;
			}
			size.width *= 4;

			for (int i = 0; i < size.height; i++)
			{
				// when the arrays are continuous,
				// the outer loop is executed only once
				const T* ptr1 = src1.ptr<T>(i);
				const T* ptr2 = src2.ptr<T>(i);
				T* dptr = dst.ptr<T>(i);

				for (int j = 0; j < size.width; j += 4)
				{
					float alpha = ptr1[j + 3] * inv_scale;
					float beta = 1 - alpha;
					dptr[j] = saturate_cast<T>(ptr1[j] * alpha + ptr2[j] * beta);
					dptr[j + 1] = saturate_cast<T>(ptr1[j + 1] * alpha + ptr2[j + 1] * beta);
					dptr[j + 2] = saturate_cast<T>(ptr1[j + 2] * alpha + ptr2[j + 2] * beta);
					dptr[j + 3] = saturate_cast<T>((1 - (1 - alpha)*(1 - beta))*alpha_scale);
				}
			}
		}

		inline cv::Mat AlphaBlendWithGray(const cv::Mat &src)
		{
			cv::Mat out;
			cv::Mat tmp(src.size(), src.type(), cv::Scalar(100, 127, 99, 255));
			AlphaBlendRGBA<uchar>(src, tmp, out);
			return out;
		}

		inline cv::Vec4i SubBorder(const cv::Vec4i &in, int border)
		{
			return cv::Vec4i(in[0] - border, in[1] - border,
				in[2] - border, in[3] - border);
		}

		bool IsMatrixIdentical(const cv::Mat &left,
			const cv::Mat &right);

		// from here: 
		// http://docs.opencv.org/doc/tutorials/highgui/video-input-psnr-ssim/video-input-psnr-ssim.html
		double GetPSNR(const cv::Mat &left, const cv::Mat &right);
		cv::Scalar GetMSSIM(const cv::Mat &left, const cv::Mat &right);

		cv::Vec4i GetMinLine(cv::Vec4i left, cv::Vec4i right, bool is_horizontal);
		cv::Mat DrawGridOnPic(const cv::Mat &src, const cv::Vec4i &grid);
		cv::Mat Generate9GridPic(const cv::Mat &src, const cv::Vec4i &grid);
		cv::Mat StretchPicWith9Grid(const cv::Mat &src, const cv::Vec4i &grid, const cv::Size &rc_dst);

		/*
		*   grid[0-3]: left, top, right, bottom
		*   grids-index's position in 9 grid:
		*       0, 1, 2
		*       3, 4, 5
		*       6, 7, 8
		*
		*/
		void Get9GridRect(const cv::Rect &outer, const cv::Vec4i &grid, cv::Rect *grids);
		bool Get9GridParamFromPic(const cv::Mat &src, cv::Vec4i &grid, cv::Vec4i &padding, cv::Mat *img = nullptr);
		cv::Mat GetGrayImageHistogram(const cv::Mat &image);

		typedef enum _ImageType
		{
			ImageType_Raw = 0,
			ImageType_Jpeg,
			ImageType_Png,
			ImageType_Bmp,
			ImageType_Apng,
			ImageType_Gif,
			ImageType_Gft,
			ImageType_Ico,
			ImageType_None,
		}ImageType;


		class ImageDesc
		{
		public:
			ImageDesc()
				: grid(0, 0, 0, 0)
				, padding(0, 0, 0, 0)
				, size(0)
				, min_quanlity(100)
				, max_quanlity(100)
			{
			}

			ImageDesc(const ImageDesc& obj)
			{
				Copy(obj);
			}

			ImageDesc &operator=(const ImageDesc& obj)
			{
				Copy(obj);
				return *this;
			}

			void Copy(const ImageDesc& obj)throw()
			{
				name = obj.name;
				image = obj.image;
				grid = obj.grid;
				padding = obj.padding;
				size = obj.size;
				min_quanlity = obj.min_quanlity;
				max_quanlity = obj.max_quanlity;
			}


			std::string name;
			cv::Mat image;
			cv::Vec4i grid;
			cv::Vec4i padding;
			uint64_t size;
			uint32_t min_quanlity;
			uint32_t max_quanlity;
		};

		bool CheckOptResult(const cv::Mat &org,
			const cv::Vec4i &org_grid,
			const cv::Mat &opt,
			const cv::Vec4i &opt_grid,
			uint32_t quality);

		inline bool CheckOptResult(const Utility::ImageDesc &org,
			const Utility::ImageDesc &opt,
			uint32_t quality)
		{
			return CheckOptResult(org.image,
				org.grid,
				opt.image,
				opt.grid,
				quality);
		}


		bool ReadImage(ImageDesc &img);
		uint64_t WriteToPngFile(const ImageDesc &img_out);
        uint64_t WriteToCompliedNinePngFile(const ImageDesc &img_out);
        uint64_t WriteToUnCompliedNinePngFile(const ImageDesc &img_out);

		//std::wstring Utf8ToUtf16(const std::string &str);
		//std::string Utf16ToUtf8(const std::wstring &str);
		boost::filesystem::path MakeRelative(boost::filesystem::path path, boost::filesystem::path file);
	}
}

#endif