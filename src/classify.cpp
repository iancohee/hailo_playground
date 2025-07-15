#include "Hailo8Device.hpp"
#include "ImageNetLabels.hpp"
#include "Utils.hpp"

#include <cstdio>
#include <iostream>

#include <hailo/hailort.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>


constexpr float confidenceThreshold = 0.50;
constexpr const std::string imageWindowName = "Classifier";


static
void
preprocessImage (
    const cv::String& inputPicture,
    cv::Mat& imageIn,
    cv::Mat& imageOut
)
{
    cv::imread(inputPicture, imageIn);
    cv::cvtColor(imageIn, imageOut, cv::COLOR_BGR2RGB);
    cv::resize(imageOut, imageOut, cv::Size(224, 224));
}

static
void
showImage (
    cv::Mat& image,
    const cv::String& windowName,
    const cv::String& label,
    const float confidence
)
{
    cv::Scalar textColor(0, 255, 0);
    cv::Scalar bgColor(0, 0, 0);

    cv::resize(image, image, cv::Size(800, 600));

    cv::Size labelTextSize = cv::getTextSize(label, cv::FONT_HERSHEY_COMPLEX, 1, 1, 0);
    cv::Point labelPos(0, labelTextSize.height);
    cv::Point rectP1(0, 0);
    cv::Point rectP2(labelTextSize.width, labelTextSize.height * 1.3);
    cv::rectangle(image, rectP1, rectP2, bgColor, cv::FILLED);
    cv::putText(image, label, labelPos, cv::FONT_HERSHEY_COMPLEX, 1, textColor, 2);

    cv::String conf = "(" + std::to_string(confidence * 100) + "%)";
    cv::Size confTextSize = cv::getTextSize(conf, cv::FONT_HERSHEY_COMPLEX, 1, 1, 0);

    int heightOffset = labelTextSize.height + (confTextSize.height * 1.5);
    rectP1 = cv::Point(0, labelTextSize.height * 1.1);
    rectP2 = cv::Point(confTextSize.width, heightOffset * 1.2);
    cv::rectangle(image, rectP1, rectP2, bgColor, cv::FILLED);
    cv::Point confPos(0, heightOffset);
    cv::putText(image, conf, confPos, cv::FONT_HERSHEY_COMPLEX, 1, textColor, 2);

    cv::namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::resizeWindow(windowName, 800, 600);
    cv::imshow(windowName, image);
}

int
main (
    int argc,
    char* argv[]
)
{
    using namespace std;
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " input.jpg" << endl;
        return 1;
    }
    std::string inputPicture(argv[1]);

    using namespace hailort;
    hailo_status status = HAILO_SUCCESS;
    auto device = Hailo8Device::create("../models/resnet_v1_50.hef");

    status = device.configureDefaultVStreams();
    if(status != HAILO_SUCCESS)
    {
        cerr << "[e] failed to configure vstreams: " << status << endl;
        return status;
    }

    cv::Mat inputImage, preprocessedImage;
    cout << "[i] preprocessing image" << endl;
    preprocessImage(inputPicture, inputImage, preprocessedImage);

    std::vector<uint8_t> outputData(device.getOutVStreamFrameSize());
    cout << "[i] writing image bytes to Hailo8" << endl;
    status = device.write(preprocessedImage, 224 * 224 * 3 * 1);
    if (status != HAILO_SUCCESS)
    {
        cerr << "[e] failed to write to hailo: "
            << hailo_get_status_message(status) << endl;
        return status;
    }

    cout << "[i] reading bytes from Hailo8" << endl;
    status = device.read(outputData);
    if (status != HAILO_SUCCESS)
    {
        cerr << "[e] failed to read from hailo device: "
            << hailo_get_status_message(status) << endl;
        return status;
    }

    // assume softmax is done on-chip based on output
    // of "hailo parse-hef resnet_v1_50.hef":
    // > Output resnet_v1_50/softmax1 UINT8, NC(1000)
    int maxIndex = utils::argmax(outputData);
    static ImageNetLabels net_labels;
    std::string label;
    float confidence = outputData[maxIndex] / 255.0;
    if (confidence < confidenceThreshold)
    {
        label = "unknown";
        cout << "[i] too low (< " 
            << confidenceThreshold << ")" << endl;
    }
    else
    {
        label = net_labels.imagenet_labelstring(maxIndex);
        cout << label << " (" << confidence << ") " << endl;
    }

    showImage(inputImage, imageWindowName, label, confidence);

    bool shouldBreak = false;
    while (true)
    {
        if (shouldBreak) break;

        try
        {
            cv::getWindowImageRect(imageWindowName);
        }
        catch (cv::Exception& e)
        {
            cerr << "[e] failed to get window properties: " << e.what() << endl;
            break;
        }

        char keyPress = (char)cv::waitKey(10);
        switch (keyPress)
        {
        case 'e': case 'q': case 27: // 27 is ESC
            shouldBreak = true;
            break;
        default:
            break;
        }
    }

    cv::destroyAllWindows();
    cout << "[i] exiting, goodbye." << endl;
    return status;
}
