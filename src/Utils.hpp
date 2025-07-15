#ifndef UTILS_H
#define UTILS_H

#include "CocoClass.hpp"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <iostream>
#include <vector>


constexpr char DEVICE_AUTO[] = "auto";

namespace utils
{


static const char* windowName = "capture";

struct Detection {
    int classId;
    hailo_bbox_float32_t boundingBox;
};

// Find in Hailo docs for reference
template<typename T, typename A>
static
int
argmax (
    const std::vector<T, A>& vec
)
{
    return static_cast<int>(std::distance(vec.begin(), max_element(vec.begin(), vec.end())));
}

// Find in Hailo docs for reference
template <typename T, typename A>
static
std::vector<T, A>
softmax (
    std::vector<T, A> const& vec
)
{
    std::vector<T, A> result;
    float m = -INFINITY;
    float sum = 0.0;

    for (const auto& val : vec) m = (val>m) ? val : m;
    for (const auto& val : vec) sum += expf(val - m);
    for (const auto& val : vec) result.push_back(expf(val-m)/sum);
    
    return result;   
}

cv::VideoCapture
getVideoCapture (
    const cv::String& deviceAddress,
    const size_t width,
    const size_t height
)
{
    cv::VideoCapture cap;
    if (deviceAddress == DEVICE_AUTO)
        cap.open(0);
    else
        cap.open(deviceAddress);
    
    if (!cap.isOpened())
        throw std::runtime_error("failed to initialize capture device");

    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    return cap;
}

void
showFrame (
    cv::InputOutputArray& frame,
    cv::String& fpsString
)
{
    cv::Scalar fontColor(255, 255, 255);
    cv::Scalar bgColor(0, 0, 0);

    cv::Size stringSize = cv::getTextSize(fpsString, cv::FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Point p1(0, 0);
    cv::Point p2(stringSize.width, stringSize.height * 1.1);
    cv::rectangle(frame, p1, p2, bgColor, cv::FILLED);
    cv::putText(frame, fpsString, cv::Point(0, stringSize.height), cv::FONT_HERSHEY_COMPLEX, 0.5, fontColor);
    cv::imshow(windowName, frame);
}

cv::Rect
rectFromDetection (
    const Detection& detection,
    size_t frameWidth,
    size_t frameHeight
)
{
    hailo_bbox_float32_t bbox = detection.boundingBox;
    int x1 = static_cast<int>(bbox.x_min * frameWidth);
    int x2 = static_cast<int>(bbox.x_max * frameWidth);
    int y1 = static_cast<int>(bbox.y_min * frameHeight);
    int y2 = static_cast<int>(bbox.y_max * frameHeight);
    return cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2));
}

void
drawRectOnFrame (
    cv::InputOutputArray& frame,
    cv::Rect& rect,
    const cv::String& label
)
{
    cv::rectangle(frame, rect, cv::Scalar(0, 255, 0));
    cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_COMPLEX, 0.5, 1, 0);
    cv::Point p1(rect.x, rect.y + textSize.height);
    cv::Point p2(rect.x + textSize.width, rect.y + textSize.height * 1.1);
    cv::rectangle(frame, cv::Point(rect.x, rect.y), p2, cv::Scalar(0, 255, 0), cv::FILLED);
    cv::putText(
        frame,
        label,
        p1,
        cv::FONT_HERSHEY_COMPLEX,
        0.5,
        cv::Scalar(0, 0, 0));
}


} // end namespace utils

#endif // UTILS_H