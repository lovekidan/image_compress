#ifndef NINE_PATCH_GLOBAL_CONFIG
#define NINE_PATCH_GLOBAL_CONFIG

#include <cstdint>
#include <sstream>
#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>

class NinePatchConfig
{
public:
    static NinePatchConfig &GetInstance()
    {
        static NinePatchConfig config;
        return config;
    }

    bool SetFunctionArgs(int outputKind, int quality = 100)
    {
        output_type_ = outputKind;
        output_quality_ = quality;
        return true;
    }

    inline void setNinePatchInfo(cv::Vec4i &ninePatchInfo){ nine_path_ = ninePatchInfo; }
    const cv::Vec4i &GetNinePathInfo()const { return nine_path_; }
    uint32_t GetOutputType()const { return output_type_; }
    uint32_t GetOutputQuality()const { return output_quality_; }
    uint32_t GetZoomRatio()const { return zoom_ratio_; }
    
private:
    NinePatchConfig()
        : output_type_(1)
        , output_quality_(100)
        , zoom_ratio_(1)
    {
    }

    cv::Vec4i nine_path_;
    uint32_t output_type_;
    uint32_t output_quality_;
    uint32_t zoom_ratio_;
};

#endif