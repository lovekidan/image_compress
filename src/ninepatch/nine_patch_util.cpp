#include "nine_patch_util.hpp"
#include <cstdint>
#include <functional>
#include <fstream>
#include <locale>
//#include <codecvt>
#include <exception>
#include "../pngimage/png_image.hpp"
#include "../libpng/png.h"
#include "nine_patch_global_config.hpp"

using namespace cv;
using namespace std;

namespace
{
	const Scalar kBlackColor = Scalar(0, 0, 0, 255);
	const Vec4b kBlackColor32 = Vec4b(0, 0, 0, 255);

	template<typename _TyFunc>
	class ScopeGuardImpl
	{
	public:
		ScopeGuardImpl(std::function<_TyFunc> func)
			: exit_func_(func)
		{
		}

		~ScopeGuardImpl()
		{
			exit_func_();
		}

	private:
		std::function<_TyFunc> exit_func_;
	};
	typedef ScopeGuardImpl<void()> ScopeGuard;



	int RectRight(const cv::Rect &rc)
	{
		return rc.br().x;
	}

	int RectBottom(const cv::Rect &rc)
	{
		return rc.br().y;
	}

	Vec4i GetGridInRect(const cv::Rect &rc, const Vec4i &grid)
	{
		return Vec4i((std::max)(rc.x, grid[0]),
			(std::max)(rc.y, grid[1]),
			(std::min)(rc.x + rc.width, grid[2]),
			(std::min)(rc.y + rc.height, grid[3]));
	}

	bool GetBlackLine(const Mat &edge, Vec2i &line)
	{
		Mat tmp = edge;
		if (tmp.rows > tmp.cols)
		{
			cv::transpose(edge, tmp);
		}

		Vec4b prev;
		line[0] = -1;
		line[1] = -1;
		for (int j = 0; j < tmp.cols; ++j)
		{
			Vec4b value = tmp.at<Vec4b>(0, j);
			if (value[3] != 0 && value != kBlackColor32)
			{
				return false;
			}

			if (line[0] == -1 && prev[3] == 0 && value == kBlackColor32)
			{
				line[0] = j;
			}
			else if (line[1] == -1 && prev == kBlackColor32 && value[3] == 0)
			{
				line[1] = tmp.cols - j;
			}
			prev = value;
		}

		return line[0] != -1 && line[1] != -1;
	}
}


bool PicOpt::Utility::IsMatrixIdentical(const Mat &left,const Mat &right)
{
	if (left.empty() && right.empty())
	{
		return true;
	}

	if (left.size != right.size)
	{
		return false;
	}

	cv::Mat diff;
	cv::compare(left, right, diff, CMP_NE);

	cv::Mat diff_gray = diff;
	if (diff_gray.channels() != 1)
	{
		cvtColor(diff, diff_gray, COLOR_RGB2GRAY);
	}
	int nz = cv::countNonZero(diff_gray);
	return nz == 0;
}

double PicOpt::Utility::GetPSNR(const Mat &left, const Mat &right)
{
	Mat s1;
	absdiff(left, right, s1);       // |I1 - I2|
	s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
	s1 = s1.mul(s1);           // |I1 - I2|^2

	Scalar s = sum(s1);        // sum elements per channel

	double sse = 0; // sum channels
	for (int i = 0; i < s1.channels(); ++i)
	{
		sse += s.val[i];
	}

	if (sse <= 1e-10) // for small values return zero
	{
		return 0;
	}
	else
	{
		double mse = sse / (double)(left.channels() * left.total());
		double psnr = 10.0 * log10((255 * 255) / mse);
		return psnr;
	}
}

Scalar PicOpt::Utility::GetMSSIM(const Mat &left, const Mat &right)
{
	const double C1 = 6.5025, C2 = 58.5225;
	/***************************** INITS **********************************/
	int d = CV_32F;

	Mat I1, I2;
	left.convertTo(I1, d);            // cannot calculate on one byte large values
	right.convertTo(I2, d);

	Mat I2_2 = I2.mul(I2);        // I2^2
	Mat I1_2 = I1.mul(I1);        // I1^2
	Mat I1_I2 = I1.mul(I2);        // I1 * I2

	/*************************** END INITS **********************************/

	Mat mu1, mu2;                   // PRELIMINARY COMPUTING
	GaussianBlur(I1, mu1, Size(11, 11), 1.5);
	GaussianBlur(I2, mu2, Size(11, 11), 1.5);

	Mat mu1_2 = mu1.mul(mu1);
	Mat mu2_2 = mu2.mul(mu2);
	Mat mu1_mu2 = mu1.mul(mu2);

	Mat sigma1_2, sigma2_2, sigma12;

	GaussianBlur(I1_2, sigma1_2, Size(11, 11), 1.5);
	sigma1_2 -= mu1_2;

	GaussianBlur(I2_2, sigma2_2, Size(11, 11), 1.5);
	sigma2_2 -= mu2_2;

	GaussianBlur(I1_I2, sigma12, Size(11, 11), 1.5);
	sigma12 -= mu1_mu2;

	///////////////////////////////// FORMULA ////////////////////////////////
	Mat t1, t2, t3;

	t1 = 2 * mu1_mu2 + C1;
	t2 = 2 * sigma12 + C2;
	t3 = t1.mul(t2);                 // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

	t1 = mu1_2 + mu2_2 + C1;
	t2 = sigma1_2 + sigma2_2 + C2;
	t1 = t1.mul(t2);                 // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

	Mat ssim_map;
	divide(t3, t1, ssim_map);        // ssim_map =  t3./t1;

	return mean(ssim_map);   // mssim = average of ssim map
}

Vec4i PicOpt::Utility::GetMinLine(Vec4i left, Vec4i right, bool is_horizontal)
{
	Vec4i ret;
	auto setStartPoint = [&](bool isLeft)
	{
		ret[0] = isLeft ? left[0] : right[0];
		ret[1] = isLeft ? left[1] : right[1];
	};

	auto setEndPoint = [&](bool isLeft)
	{
		ret[2] = isLeft ? left[2] : right[2];
		ret[3] = isLeft ? left[3] : right[3];
	};

	if (is_horizontal)
	{
		Regular(left, true);
		Regular(right, true);

		setStartPoint(left[0] > right[0]);
		setEndPoint(left[2] < right[2]);
	}
	else
	{
		Regular(left, false);
		Regular(right, false);

		setStartPoint(left[1] > right[1]);
		setEndPoint(left[3] < right[3]);
	}

	return ret;
}

Mat PicOpt::Utility::DrawGridOnPic(const Mat &src, const Vec4i &grid)
{
	Mat out;
	src.copyTo(out);
	auto &size = out.size();
	Vec4i grid_fix = grid;
	grid_fix[2] = size.width - grid[2] - 1;
	grid_fix[3] = size.height - grid[3] - 1;
	line(out, Point(grid_fix[0], 0), Point(grid_fix[0], size.height), Utility::RandomColor(), 1, LINE_4);
	line(out, Point(grid_fix[2], 0), Point(grid_fix[2], size.height), Utility::RandomColor(), 1, LINE_4);
	line(out, Point(0, grid_fix[1]), Point(size.width, grid_fix[1]), Utility::RandomColor(), 1, LINE_4);
	line(out, Point(0, grid_fix[3]), Point(size.width, grid_fix[3]), Utility::RandomColor(), 1, LINE_4);
	return out;
}

Mat PicOpt::Utility::Generate9GridPic(const Mat &src, const Vec4i &grid)
{
	Mat out;
	if (src.channels() == 4)
	{
		copyMakeBorder(src, out, 1, 1, 1, 1, BORDER_CONSTANT, Scalar(0, 0, 0, 0));
	}
	else
	{
		Mat tmp;
		cvtColor(src, tmp, COLOR_BGR2BGRA);
		copyMakeBorder(tmp, out, 1, 1, 1, 1, BORDER_CONSTANT, Scalar(0, 0, 0, 0));
	}

	// outer pic add 1 offset
	auto size_9gird = out.size();
	size_9gird.width -= 1;
	size_9gird.height -= 1;
	Vec4i grid_fix;
	grid_fix[0] = grid[0] + 1;
	grid_fix[1] = grid[1] + 1;
	grid_fix[2] = size_9gird.width - grid[2] - 1;
	grid_fix[3] = size_9gird.height - grid[3] - 1;
	line(out, Point(grid_fix[0], 0), Point(grid_fix[2], 0), kBlackColor, 1, LINE_4);
	line(out, Point(grid_fix[0], size_9gird.height), Point(grid_fix[2], size_9gird.height), kBlackColor, 1, LINE_4);
	line(out, Point(0, grid_fix[1]), Point(0, grid_fix[3]), kBlackColor, 1, LINE_4);
	line(out, Point(size_9gird.width, grid_fix[1]), Point(size_9gird.width, grid_fix[3]), kBlackColor, 1, LINE_4);

	return out;
}

Mat PicOpt::Utility::StretchPicWith9Grid(const Mat &src, const Vec4i &grid, const Size &rc_dst)
{
	cv::Rect rc_src(Point(), src.size());
	cv::Rect grids[9];
	Get9GridRect(rc_src, grid, grids);
	cv::Rect dst_grids[9];
	Get9GridRect(cv::Rect(Point(), rc_dst), grid, dst_grids);

	Mat dst(rc_dst, src.type());
	for (size_t i = 0; i < 9; ++i)
	{
		if (grids[i].area() == 0 || dst_grids[i].area() == 0)
		{
			continue;
		}

		Mat &src_part = src(grids[i]);
		Mat &dst_part = dst(dst_grids[i]);
		resize(src_part, dst_part, dst_part.size(), 0, 0, INTER_NEAREST);
	}

	return dst;
}

void PicOpt::Utility::Get9GridRect(const Rect &outer, const Vec4i &grid, cv::Rect *grids)
{
	Vec4i grid_fix = grid;
	int center_width = outer.width - grid_fix[0] - grid_fix[2];
	int center_height = outer.height - grid_fix[1] - grid_fix[3];

	// top
	grids[0].x = outer.x;
	grids[0].y = outer.y;
	grids[0].width = grid_fix[0];
	grids[0].height = grid_fix[1];

	grids[1].x = RectRight(grids[0]);
	grids[1].y = outer.y;
	grids[1].width = center_width;
	grids[1].height = grid_fix[1];

	grids[2].x = RectRight(grids[1]);
	grids[2].y = outer.y;
	grids[2].width = grid_fix[2];
	grids[2].height = grid_fix[1];

	// center
	grids[3].x = outer.x;
	grids[3].y = RectBottom(grids[0]);
	grids[3].width = grid_fix[0];
	grids[3].height = center_height;

	grids[4].x = RectRight(grids[3]);
	grids[4].y = RectBottom(grids[0]);
	grids[4].width = center_width;
	grids[4].height = center_height;

	grids[5].x = RectRight(grids[4]);
	grids[5].y = RectBottom(grids[0]);
	grids[5].width = grid_fix[2];
	grids[5].height = center_height;

	// bottom
	grids[6].x = outer.x;
	grids[6].y = RectBottom(grids[3]);
	grids[6].width = grid_fix[0];
	grids[6].height = grid_fix[3];

	grids[7].x = RectRight(grids[6]);
	grids[7].y = RectBottom(grids[3]);
	grids[7].width = center_width;
	grids[7].height = grid_fix[3];

	grids[8].x = RectRight(grids[7]);
	grids[8].y = RectBottom(grids[3]);
	grids[8].width = grid_fix[2];
	grids[8].height = grid_fix[3];
}

bool PicOpt::Utility::Get9GridParamFromPic(const Mat &src, Vec4i &grid, Vec4i &padding, Mat *img)
{
	if (src.channels() != 4)
	{   // not contain alpha
		return false;
	}

	{
		Vec2i left_vec;
		Mat left = src.col(0);
		if (!GetBlackLine(left, left_vec))
		{
			return false;
		}

		Vec2i right_vec;
		Mat right = src.col(src.size().width - 1);
		if (!GetBlackLine(right, right_vec))
		{
			return false;
		}

		/*grid[1] = std::min(left_vec[0], right_vec[0]);
		grid[3] = std::min(left_vec[1], right_vec[1]);*/
		grid[1] = left_vec[0];
		grid[3] = left_vec[1];
		padding[1] = right_vec[0];
		padding[3] = right_vec[1];
	}
	{
		Vec2i top_vec;
		Mat top = src.row(0);
		if (!GetBlackLine(top, top_vec))
		{
			return false;
		}

		Vec2i bottom_vec;
		Mat bottom = src.row(src.size().height - 1);
		if (!GetBlackLine(bottom, bottom_vec))
		{
			return false;
		}

		/*grid[0] = std::min(top_vec[0], bottom_vec[0]);
		grid[2] = std::min(top_vec[1], bottom_vec[1]);*/
		grid[0] = top_vec[0];
		grid[2] = top_vec[1];
		padding[0] = bottom_vec[0];
		padding[2] = bottom_vec[1];
	}

	// for inner pic
	grid[0] -= 1;
	grid[1] -= 1;
	grid[2] -= 1;
	grid[3] -= 1;
	padding[0] -= 1;
	padding[1] -= 1;
	padding[2] -= 1;
	padding[3] -= 1;

    const GlobalConfig &config = GlobalConfig::GetInstance();
    if (img){
		cv::Rect rc(Point(1, 1), src.size());
		rc.width -= 2;
		rc.height -= 2;
		(*img) = src(rc);
	}
	return true;
}

Mat PicOpt::Utility::GetGrayImageHistogram(const Mat &image)
{
	if (image.channels() > 1)
	{
		return Mat();
	}

	int hist_size = 256;
	float range[] = { 0, 256 };
	const float* hist_range = { range };

	Mat hist_data;
	calcHist(&image, 1, 0, Mat(), hist_data, 1, &hist_size, &hist_range);

	cv::Size size(640, 640);
	int bin_w = cvRound((double)size.width / hist_size);
	Mat hist_img(size, CV_8UC3, Scalar(0, 0, 0));
	for (int i = 1; i < hist_size; ++i)
	{
		cv::line(hist_img,
			Point(bin_w*(i - 1), size.height - cvRound(hist_data.at<float>(i - 1))),
			Point(bin_w*(i), size.height - cvRound(hist_data.at<float>(i))),
			Scalar(255, 255, 255));
	}
	return hist_img;
}

bool PicOpt::Utility::ReadImage(ImageDesc &img){
	PngImage pngImage;
	pngImage.read_png(img.name);
	img.image = pngImage.get_Mat_from_png();
	if (img.image.data == NULL)
		return false;
	switch (img.image.channels())
	{
	case 3:
		cvtColor(img.image, img.image, COLOR_RGB2RGBA);
		break;
	}
	return true;
}

uint64_t PicOpt::Utility::WriteToPngFile(const ImageDesc &img_out){
	PngImage pngImage;
	pngImage.read_png_from_Mat(img_out.image);
	if(!pngImage.write_png(img_out.name))
		throw exception("Fail! exception converting image to PNG!");
	return boost::filesystem::file_size(boost::filesystem::path(img_out.name));
}

uint64_t PicOpt::Utility::WriteToCompliedNinePngFile(const ImageDesc &img_out){
	PngImage pngImage;
	pngImage.read_png_from_Mat(img_out.image);
	pngImage.set_npTc_info(img_out.grid, img_out.padding);
	if (!pngImage.write_png(img_out.name))
		throw exception("Fail! exception converting image to PNG!");
	return boost::filesystem::file_size(boost::filesystem::path(img_out.name));
}

uint64_t PicOpt::Utility::WriteToUnCompliedNinePngFile(const ImageDesc &img_out){
    PngImage pngImage;
    pngImage.read_png_from_Mat(img_out.image);
    pngImage.set_npTc_info(img_out.grid, img_out.padding);
    pngImage.set_de9patch();
    if (!pngImage.write_png(img_out.name))
        throw exception("Fail! exception converting image to PNG!");
    return boost::filesystem::file_size(boost::filesystem::path(img_out.name));
}

std::wstring PicOpt::Utility::Utf8ToUtf16(const std::string &str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf16conv;
	return utf16conv.from_bytes(str);
}

std::string PicOpt::Utility::Utf16ToUtf8(const std::wstring &str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf16conv;
	return utf16conv.to_bytes(str);
}

boost::filesystem::path PicOpt::Utility::MakeRelative(boost::filesystem::path path, boost::filesystem::path file)
{
	typedef std::wstring::value_type WideChar;
	std::vector<WideChar> buffer(32768);
	auto CanonicalizePath = [&buffer](const std::wstring &path)->std::wstring
	{
		::PathCanonicalizeW(buffer.data(), path.c_str());
		return std::wstring(buffer.data());
	};

	path = boost::filesystem::complete(path);
	file = boost::filesystem::complete(file);
	std::wstring path_name = CanonicalizePath(Utf8ToUtf16(path.file_string()));
	std::wstring file_name = CanonicalizePath(Utf8ToUtf16(file.file_string()));
	int ret = ::PathRelativePathToW(buffer.data(),
		path_name.c_str(),
		FILE_ATTRIBUTE_DIRECTORY,
		file_name.c_str(),
		FILE_ATTRIBUTE_NORMAL);
	return boost::filesystem::path(Utf16ToUtf8(std::wstring(buffer.data())));
}

bool PicOpt::Utility::CheckOptResult(const cv::Mat &org,
	const cv::Vec4i &org_grid,
	const cv::Mat &opt,
	const cv::Vec4i &opt_grid,
	uint32_t quality)
{
	Size input_size = org.size();
	Size stretch_size(200, 300);
	stretch_size.width = (std::max)(input_size.width, stretch_size.width) * 2;
	stretch_size.height = (std::max)(input_size.height, stretch_size.height) * 2;
	Mat stretch_img = AlphaBlendWithGray(org);
	Mat opt_stretch_img = AlphaBlendWithGray(opt);
	stretch_img = StretchPicWith9Grid(stretch_img, org_grid, stretch_size);
	opt_stretch_img = StretchPicWith9Grid(opt_stretch_img, opt_grid, stretch_size);

	if (quality < 100)
	{
		double psnr = GetPSNR(stretch_img, opt_stretch_img);
		return psnr > quality / 2.0;
	}
	else
	{
		return IsMatrixIdentical(stretch_img, opt_stretch_img);
	}
}
