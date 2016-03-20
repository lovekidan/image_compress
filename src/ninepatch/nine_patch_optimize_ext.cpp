#include <boost/filesystem.hpp>
#include <boost/utility.hpp>
#include <boost/regex.hpp>
#include <regex>
#include "include/ic_image.h"
#include "include/ic_exception.h"
#include "nine_patch_util.hpp"
#include "nine_patch_gen_helper.hpp"
#include "nine_patch_optimize.hpp"
#include "nine_patch_global_config.hpp"


using namespace std;
using namespace cv;
using namespace PicOpt;
namespace bfs = boost::filesystem;

namespace
{
    enum
    {
        kOutputInvalid = 0,
        kOutputPng = 1,
        kOutputUnCompiledNinePatchPng = 2,
        kOutputCompiledNinePatchPng = 3
    };

	bool Recognize9Grid(Utility::ImageDesc &input, bool use_img_grid)
	{
		using namespace Utility;

        if (use_img_grid)
        {
            return true;
        }

		Vec4i new_grid;
		Gen9Grid generator;
		generator.Get9GirdLines(input.image, new_grid);

		ImageDesc input_opt = input;
		input_opt.grid[0] = new_grid[0];
		input_opt.grid[2] = new_grid[2];
		if (CheckOptResult(input, input_opt, 100))
		{
			input.grid[0] = new_grid[0];
			input.grid[2] = new_grid[2];
		}

		input_opt.grid = input.grid;
		input_opt.grid[1] = new_grid[1];
		input_opt.grid[3] = new_grid[3];
		if (CheckOptResult(input, input_opt, 100))
		{
			input.grid[1] = new_grid[1];
			input.grid[3] = new_grid[3];
		}

		return true;
	}

	bool OptimizePic(const Utility::ImageDesc &input,
		Utility::ImageDesc &output)
	{
		Optimize9Grid optimizator;
		optimizator.SetCenterRectWidth(2);
		bool b_ret = optimizator.Optimize(input.image, input.grid, output.image, output.grid);

		return b_ret;
	}

	bool MainOptimizeProcess(const Utility::ImageDesc &input,
		Utility::ImageDesc &output,
		bool use_image_grid)
	{
		Utility::ImageDesc out_opt = input;
		Recognize9Grid(out_opt, use_image_grid);
		if (OptimizePic(out_opt, out_opt))
		{
			output = out_opt;
			return true;
		}
		return false;
	}

	bool getMutilBlackLine(const Mat &edge, vector<uint32_t> &divs){
		Mat tmp = edge;
		if (tmp.rows > tmp.cols){
			cv::transpose(edge, tmp);
		}
		divs.clear();
		Vec4b prev(0, 0, 0, 0);
		const Vec4b kBlackColor32 = Vec4b(0, 0, 0, 255);
		for (int j = 0; j < tmp.cols; ++j){
			Vec4b value = tmp.at<Vec4b>(0, j);
			if (value[3] != 0 && value != kBlackColor32){
				return false;
			}

			if (prev[3] == 0 && value == kBlackColor32){
				divs.push_back(j);
			}
			else if (prev == kBlackColor32 && value[3] == 0)
			{
				divs.push_back(j);
			}
			prev = value;
		}
		return divs.size() > 2;
	}

	pair<uint64_t, uint64_t> ProcessSingleImage(const string &src_name,
		const string &dst_name)
	{
		using namespace Utility;

		// read the image
		ImageDesc input;
		input.name = src_name;
		input.size = bfs::file_size(bfs::path(src_name));
		ReadImage(input);
		if (input.image.empty()){
			throw InvalidImageException("Fail! invalid input image!");
		}
		GlobalConfig &config = GlobalConfig::GetInstance();
		input.grid = config.GetNinePathInfo();

		vector<uint32_t> divs;
		if (getMutilBlackLine(input.image.col(0), divs) || getMutilBlackLine(input.image.row(0), divs)){
			throw ICException("Fail! image contain too much gird!");
		}

		if (!IsGridValid(input.grid) &&
			!Get9GridParamFromPic(input.image, input.grid, input.padding, &input.image))
		{
			throw ICException("Fail! image not contain 9 grid info!");
		}

		ImageDesc output;
        if (!MainOptimizeProcess(input, output, true))
		{
			throw StretchException("Fail! optimize image stretch check error!");
		}

		if (output.image.size().area() == 0){
			throw ICException("Fail! output images are empty!");
		}

		output.name = dst_name;
        config.setNinePatchInfo(output.grid);
		switch (config.GetOutputType())
		{
		case kOutputPng:
			output.size = WriteToPngFile(output);
			break;
        case kOutputUnCompiledNinePatchPng:
            output.size = WriteToUnCompliedNinePngFile(output);
			break;
		case kOutputCompiledNinePatchPng:
            output.size = WriteToCompliedNinePngFile(output);
			break;
		}
        cout << "NinePath: ";
        cout << output.grid[0] << ',';
        cout << output.grid[1] << ',';
        cout << output.grid[2] << ',';
        cout << output.grid[3] << endl;

		return pair<uint64_t, uint64_t>(input.size, output.size);
	}

	bool CheckImageExt(const std::string &name)
	{
		const static std::regex file_ext_regx(".*\\.(gft|gif|bmp|png|jpg|jpeg)",
			std::regex_constants::ECMAScript | std::regex_constants::icase);
		return std::regex_match(name, file_ext_regx);
	}
}

bool NinePatchOpt(const char *inputFile, const char *outputFile, int outputKind, int(&ninePatch)[4], int quality){
	if (inputFile == NULL || strlen(inputFile) == 0 || outputFile == NULL || strlen(outputFile) == 0)
		return false;

	using namespace Utility;
	cv::Vec4i inNinePatchInfo;
	inNinePatchInfo[0] = ninePatch[0];
	inNinePatchInfo[1] = ninePatch[1];
	inNinePatchInfo[2] = ninePatch[2];
	inNinePatchInfo[3] = ninePatch[3];

	GlobalConfig &config = GlobalConfig::GetInstance();
	if (!config.SetFunctionArgs(std::string(inputFile), std::string(outputFile), outputKind, inNinePatchInfo, quality)){
		return false;
	}

	int ret = 0;
	auto file = bfs::path(config.GetSrcPath());
	try
	{
		auto size = ProcessSingleImage(config.GetSrcPath(), config.GetDstPath());
        cv::Vec4i resultNinePatchInfo = config.GetNinePathInfo();
        ninePatch[0] = resultNinePatchInfo[0];
        ninePatch[1] = resultNinePatchInfo[1];
        ninePatch[2] = resultNinePatchInfo[2];
        ninePatch[3] = resultNinePatchInfo[3];
		bfs::path output_name(config.GetDstPath());
		cout << "Success with: " << output_name.filename();
		cout << ", reduce size: " << (int64_t)size.first - (int64_t)size.second << " bytes" << endl;
	}
	catch (const InvalidImageException &e)
	{
		cout << e.what() << ", file name: " << file.filename() << endl;
		ret = -1;
	}
	catch (const ImageSizeException &e)
	{
		cout << e.what() << ", file name: " << file.filename() << endl;
		ret = -1;
	}
	catch (const StretchException &e)
	{
		cout << e.what() << ", file name: " << file.filename() << endl;
		ret = -1;
	}
	catch (const std::exception &e)
	{
		cout << e.what() << ", file name: " << file.filename() << endl;
		ret = -1;
	}
	return ret == 0;
}
