#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "icutil/ic_util.hpp"
#include <boost/filesystem.hpp>

using std::string;

string get_image_type_des(int type) {
	switch (type) {
		case IMAGE_TYPE_JPEG:
			return "JPEG";
		case IMAGE_TYPE_NORMAL_PNG:
			return "PNG";
		case IMAGE_TYPE_APNG:
			return "APNG";
		case IMAGE_TYPE_COMPLIED_9PNG:
		case IMAGE_TYPE_UNCOMPLIED_9PNG:
			return "9.PNG";
		case IMAGE_TYPE_GIF:
			return "GIF";
		default:
			return "UNKNOWN";
	}
}

bool compress_image_from_file(string& inputPath, string& outputPath, int quality) {
	IMAGE_TYPE image_type = ic_get_image_type(inputPath.c_str());
	printf("===========> start to compress a %s image[%s]....\n", get_image_type_des(image_type).c_str(), inputPath.c_str());
	bool success = false;
	switch (image_type) {
		case IMAGE_TYPE_NORMAL_PNG:
		success = ic_compress_png(inputPath.c_str(), outputPath.c_str(), quality);
		break;
		case IMAGE_TYPE_APNG:
		success = ic_compress_apng(inputPath.c_str(), outputPath.c_str());
		break;
		case IMAGE_TYPE_JPEG:
		success = ic_compress_jpeg(inputPath.c_str(), outputPath.c_str(), quality);
		break;
		case IMAGE_TYPE_UNCOMPLIED_9PNG:
		success = ic_compress_nine_patch_png(inputPath.c_str(), outputPath.c_str(), quality);
		break;
	}
	if (success) {
		printf("===========> compress success and save to [%s].\n\n", outputPath.c_str());
	} else {
		printf("===========> compress fail, please check and retry.\n\n");
	}
	return success;
}

std::vector<string> get_files_from_dir(const string& inputPath) {
	std::vector<string> files;
	boost::filesystem::directory_iterator dirIterBegin(inputPath);
	boost::filesystem::directory_iterator dirIterEnd;
	for (; dirIterBegin != dirIterEnd; ++ dirIterBegin) {
		if (!boost::filesystem::is_directory(*dirIterBegin)) {
			files.push_back(string(dirIterBegin->path().c_str()));
		}
	}
	return files;
}

string combine_file_path(const string& dir, const string& postFix, const string& name, const string& extension) {
	return string().append(dir).append(name).append(postFix).append(extension);
}

bool compress_image_from_dir(string& inputPath, string& outputPath, int quality) {
	std::vector<string> files = get_files_from_dir(inputPath);
	string postFix; // = 0 != strcmp(inputPath.c_str(), outputPath.c_str()) ? "" : ".compressed";
	string extension = "";
	string baseName = "";
	string outputFilePath = "";
	for (auto iter = files.begin(); files.end() != iter; ++iter) {
		baseName = boost::filesystem::basename(*iter);
		extension = boost::filesystem::extension(*iter);
		outputFilePath = combine_file_path(outputPath, postFix, baseName, extension);
		printf("%s\n", outputPath.c_str());
		compress_image_from_file(*iter, outputFilePath, quality);
	}
	return true;
}

string fix_input_path(string& path, bool is_directory) {
	boost::filesystem::path tmpPath(path);
	if (!tmpPath.is_complete() || boost::filesystem::exists(tmpPath)) {
		try {
			tmpPath = boost::filesystem::canonical(tmpPath);
		} catch(boost::filesystem::filesystem_error e) {
			printf("error -> %s.\n", e.what());
			return string();
		}
		if (is_directory)
		{
			return string().append(tmpPath.string()).append("/");
		}
		return tmpPath.string();
	}

	return string();
}

string fix_output_path(string& path, bool is_directory) {
	boost::filesystem::path tmpPath(path);
	if (tmpPath.is_complete()) {
		return tmpPath.string();
	}

	tmpPath = boost::filesystem::system_complete(tmpPath);
	if (boost::filesystem::exists(tmpPath)) {
		tmpPath = boost::filesystem::canonical(tmpPath);
		if (is_directory)
		{
			return string().append(tmpPath.string()).append("/");
		}
		return tmpPath.string();
	}

	string fileName;
	boost::filesystem::path dirPath = tmpPath;
	if (!is_directory)
	{
		fileName = tmpPath.filename().string();
		dirPath = tmpPath.parent_path();
	}

	if (!boost::filesystem::exists(dirPath)) {
		boost::filesystem::create_directory(dirPath);
	}
	dirPath = boost::filesystem::canonical(dirPath);

	if (is_directory)
	{
		return string().append(dirPath.string()).append("/");
	}
	return string().append(dirPath.string()).append("/").append(fileName);
}

int main(int argc, char** argv) {
	string inputPath;
	string outputPath;
	string errorMessage;
	int quality = 100;
	bool isDir = false;
	if (argc < 2) { // no commands
		goto help;
	}

	for (int i = 1; i < argc; ++i) {
		if (0 == strcmp("-f", argv[i])) { // file
			isDir = false;
			if (i + 1 < argc) {
				inputPath = argv[++i];
				continue;
			} else {
				errorMessage = "you should input a file path after -f.";
				goto error;
			}
		} else if (0 == strcmp("-d", argv[i])) { // directory
			isDir = true;
			if (i + 1 < argc) {
				inputPath = argv[++i];
				continue;
			}  else {
				errorMessage = "you should input a directory path after -d.";
				goto error;
			}
		} else if (0 == strcmp("-o", argv[i])) { // output path
			if (i + 1 < argc) {
				outputPath = argv[++i];
				continue;
			} else {
				errorMessage = "you should input a output path after -o.";
				goto error;
			}
		} else if (0 == strcmp("-q", argv[i])) { // quality
			if (i + 1 < argc) {
				quality = atoi(argv[++i]);
				if (quality < 0 || quality > 100) { // shoule between 0 - 100
					errorMessage = "quality should be a numer between 0 to 100.";
					goto error;
				}
				continue;
			} else {
				errorMessage = "you should input a output path after -o.";
				goto error;
			}
		} else if (0 == strcmp("-help", argv[i])) { // help
			goto help;
		}
	}

	if (0 == inputPath.length()) {
		errorMessage = "invalid command, you can input -help to get right command.";
		goto error;
	}

	inputPath = fix_input_path(inputPath, isDir);
	if (0 == inputPath.length())
	{
		errorMessage = "invalid inputPath, please check and retry.";
		goto error;
	}
	
	if (0 == outputPath.length()) {
		outputPath = inputPath;
	} else {
		outputPath = fix_output_path(outputPath, isDir);
	}

	printf("===========> paths checking............\ninputPath>>>>%s\noutputPath>>>>%s\n", inputPath.c_str(), outputPath.c_str());

	if (isDir) {
		compress_image_from_dir(inputPath, outputPath, quality);
	} else {
		compress_image_from_file(inputPath, outputPath, quality);
	}

	return 0;

	help:
		printf("help for image_compress:\n");
		printf("-f the compress image file path.\n");
		printf("-d the compress image direcotry path.\n");
		printf("-o the output path after compress.\n");
		printf("-q compress quality, a number between 0 to 100.\n");
		printf("example of compress from file: ./image_compress -f filepath -o outputPath -q quality.\n");
		printf("example of compress from directory: ./image_compress -d directory path -o outputPath -q quality.\n");
		return 0;

	error:
		printf("error:%s\n", errorMessage.c_str());
		return -1;
}
