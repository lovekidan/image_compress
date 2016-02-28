#include <exception.hpp>
#include <boost/filesystem.hpp>
#include <boost/utility.hpp>
#include <boost/regex.hpp>
#include "include/ic_image.h"
#include "nine_patch_util.hpp"
#include "nine_patch_gen_helper.hpp"
#include "nine_patch_optimize.hpp"
#include "nine_patch_global_config.hpp"


using namespace std;
using namespace cv;
using namespace PicOpt;
namespace mysys = boost::filesystem;

#define DECLARE_EXCEPTION(className) \
    class className: public exception \
    { \
    public: \
        className(const char *msg) : exception(msg) {} \
    }


namespace
{
	DECLARE_EXCEPTION(InvalidImageException);
	DECLARE_EXCEPTION(ImageSizeException);
	DECLARE_EXCEPTION(StretchException);

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
		input.size = mysys::file_size(mysys::path(src_name));
		ReadImage(input);
		if (input.image.empty()){
			throw InvalidImageException("Fail! invalid input image!");
		}
		GlobalConfig &config = GlobalConfig::GetInstance();
		input.grid = config.GetNinePathInfo();

		vector<uint32_t> divs;
		if (getMutilBlackLine(input.image.col(0), divs) || getMutilBlackLine(input.image.row(0), divs)){
			throw exception("Fail! image contain too much gird!");
		}

		if (!IsGridValid(input.grid) &&
			!Get9GridParamFromPic(input.image, input.grid, input.padding, &input.image))
		{
			throw exception("Fail! image not contain 9 grid info!");
		}

		ImageDesc output;
        if (!MainOptimizeProcess(input, output, true))
		{
			throw StretchException("Fail! optimize image stretch check error!");
		}

		if (output.image.size().area() == 0){
			throw exception("Fail! output images are empty!");
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

bool NinePatchOpt(const char *inputFile, const char *outputFile, int outputKind, int(&ninePatch)[4], int quality = 100){
	if (inputFile == NULL || strlen(inputFile) == 0 || outputFile == NULL || strlen(outputFile) == 0)
		return false;

	using namespace Utility;
	GlobalConfig &config = GlobalConfig::GetInstance();
	if (!config.SetFunctionArgs(std::string(inputFile), std::string(outputFile), outputKind, ninePatch, quality)){
		return false;
	}

	int ret = 0;
	if (!config.IsMultiFile())
	{
		auto file = mysys::path(config.GetSrcPath());
		try
		{
			auto size = ProcessSingleImage(config.GetSrcPath(), config.GetDstPath());
            cv::Vec4i resultNinePatchInfo = config.GetNinePathInfo();
            ninePatch[0] = resultNinePatchInfo[0];
            ninePatch[1] = resultNinePatchInfo[1];
            ninePatch[2] = resultNinePatchInfo[2];
            ninePatch[3] = resultNinePatchInfo[3];
			mysys::path output_name(config.GetDstPath());
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
	}
	else
	{
		auto src_path = mysys::path(config.GetSrcPath());
		auto dst_path = mysys::path(config.GetDstPath());
		if (!mysys::is_directory(src_path) || !mysys::is_directory(dst_path))
		{
			cout << "src dir not found!" << endl;
			ret = -1;
		}

		uint32_t file_count = 0;
		uint32_t success_count = 0;
		uint32_t fail_count = 0;
		uint32_t fail_size_count = 0;
		uint32_t fail_stretch_count = 0;
		uint64_t total_size = 0;
		uint64_t output_size = 0;
		for (auto it = mysys::recursive_directory_iterator(src_path);
			it != mysys::recursive_directory_iterator();
			++it)
		{
			auto &file = it->path();
			if (mysys::is_directory(file))
			{
				continue;
			}

			if (!CheckImageExt(file.extension()))
			{
				continue;
			}

			++file_count;
			try
			{
				auto output_file = Utility::MakeRelative(src_path, file);
				auto size = ProcessSingleImage(file,
					dst_path.directory_string() + output_file.file_string());

				cout << "Success with: " << file.filename();
				cout << ", reduce size: " << (int64_t)size.first - (int64_t)size.second << " bytes" << endl;
				total_size += size.first;
				output_size += size.second;
				++success_count;
			}
			catch (const InvalidImageException &e)
			{
				cout << e.what() << ", file name: " << file.filename() << endl;
				ret = -1;
				++fail_count;
			}
			catch (const ImageSizeException &e)
			{
				cout << e.what() << ", file name: " << file.filename() << endl;
				ret = -1;
				++fail_count;
				++fail_size_count;
			}
			catch (const StretchException &e)
			{
				cout << e.what() << ", file name: " << file.filename() << endl;
				ret = -1;
				++fail_count;
				++fail_stretch_count;
			}
			catch (const exception &e)
			{
				cout << e.what() << ", file name: " << file.filename() << endl;
				ret = -1;
				++fail_count;
			}
		}

		cout << endl << endl;
		cout << "*****************************************************" << endl;
		cout << "    total:        " << file_count << endl;
		cout << "    success:      " << success_count << endl;
		cout << "    fail:         " << fail_count << endl;
		cout << "    size fail:    " << fail_size_count << endl;
		cout << "    stretch fail: " << fail_stretch_count << endl;
		cout << "    total size:   " << total_size << endl;
		cout << "    opt size:     " << output_size << endl;
	}

	return ret == 0;
}
