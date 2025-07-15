#include "CocoClass.hpp"
#include "EmailNotifier.hpp"
#include "Hailo8Device.hpp"
#include "Utils.hpp"

#include <hailo/hailort.h>
#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <thread>

constexpr size_t defaultCaptureHeight = 600;
constexpr size_t defaultCaptureWidth = 800;
constexpr size_t yolov8ModelInputHeight = 640;
constexpr size_t yolov8ModelInputWidth = 640;
constexpr size_t defaultDeviceId = 0;
constexpr size_t inputSize = yolov8ModelInputHeight * yolov8ModelInputHeight * 3;

struct ProgramArguments {
    std::string deviceAddress;
    std::string modelPath;
    std::string emailAccount;
    std::string emailPassword;
    std::string SMTPAddress;
    std::string emailTo;
};

EmailCode
sendEmailNotification (
   const std::string& username,
   const std::string& password,
   const std::string& url,
   const std::vector<std::string>& to,
   const std::string& body,
   const std::vector<uint8_t>& jpg
)
{
    EmailNotifier curl(
        username,
        password,
        url);

    EmailCode mailStatus = curl.connectAndSendImage(
        username,
        to,
        body,
        jpg);

    if (mailStatus != CURLE_OK)
        std::cerr << curl_easy_strerror(mailStatus) << std::endl;

    return mailStatus;
}

void
spawnEmailThread (
    const std::vector<uint8_t>& jpg,
    ProgramArguments& args
)
{
    std::cout << "[i] sending email" << std::endl;
    std::vector<std::string> to;
    to.push_back(args.emailTo);
    std::thread emailThread(
        sendEmailNotification,
        args.emailAccount,
        args.emailPassword,
        args.SMTPAddress,
        to,
        "email alert System",
        jpg);
    emailThread.detach();
}

static
int
parseArguments (
    int argc,
    const char* const* argv,
    ProgramArguments& args
)
{
    const cv::String keys = "{ h help ?   | | print this message }"
                            "{ m model hef | yolov8n.hef | path of the model to load in HEF format. Only yolov8n.hef has been tested }"
                            "{ e email    | | email account for SMTP authentication }"
                            "{ s smtp     | smtp://smtp.gmail.com:587 | SMTP address }"
                            "{ t to       | | \"MAILTO\" field for email }"
                            "{ @device    | auto | video device to open. Can be IP address or device path }"
                            ;
    cv::CommandLineParser parser(argc, argv, keys);
    if (parser.has("help"))
    {
        parser.printMessage();
        return -1;
    }

    char* SMTPPass = getenv("SMTP_PASS");
    if (SMTPPass == nullptr)
    {
        std::cout << "SMTP_PASS environment variable needs to be set" << std::endl;
        return -2;
    }

    using std::string;
    args.deviceAddress = parser.get<string>("@device");
    args.modelPath = parser.get<string>("model");
    args.emailAccount = parser.get<string>("email");
    args.emailPassword = SMTPPass;
    args.SMTPAddress = parser.get<string>("smtp");
    args.emailTo = parser.get<string>("to");

    unsetenv("SMTP_PASS");
    return 0;
}

void
preProcess (
    cv::InputArray& inputFrame,
    cv::OutputArray& processed
)
{
    cv::resize(
        inputFrame,
        processed,
        cv::Size(yolov8ModelInputHeight, yolov8ModelInputWidth));
}

std::vector<utils::Detection>
postProcess (
    const std::vector<float32_t>& inferenceOutput
)
{
    assert(sizeof(float32_t) == 4);

    std::vector<utils::Detection> detections;
    const float32_t* data = inferenceOutput.data();
    size_t offset = 0;
    
    // skip class index 0: _background_ which is not detected
    for (size_t classIndex = 1; classIndex < CocoClass::numClasses; classIndex++)
    {
        float32_t detCount = *(data + offset);
        offset++;

        for (size_t detIndex = 0; detIndex < detCount; detIndex++)
        {
            utils::Detection det;
            hailo_bbox_float32_t bbox =
                *(reinterpret_cast<const hailo_bbox_float32_t*>(data + offset));
            det.classId = classIndex;
            det.boundingBox = bbox;
            detections.push_back(det);
            offset += 5; // each bbox is 5 floats wide
        }
    }

    return detections;
}

void
drawDetections (
    cv::InputOutputArray& frame,
    const std::vector<utils::Detection>& detections,
    const std::string& fps
)
{
    cv::String fpsString, boxLabel;
    fpsString.reserve(24);
    boxLabel.reserve(32);
    for (const auto& detection : detections)
    {
        boxLabel.clear();
        boxLabel += CocoClass::nameFromIndex(detection.classId)
            + " "
            + std::to_string(detection.boundingBox.score * 100)
            + "%";
        cv::Rect rect = utils::rectFromDetection(detection,
            defaultCaptureWidth,
            defaultCaptureHeight);
        utils::drawRectOnFrame(frame, rect, boxLabel);
    }
    fpsString += "FPS: " + fps;
    utils::showFrame(frame, fpsString);
}

int
main (
    int argc,
    char *argv[]
)
{
    ProgramArguments args;
    if (parseArguments(argc, argv, args) != 0)
    {
        return -1;
    }

    using namespace std;
    using namespace hailort;

    Hailo8Device hailo = Hailo8Device::create(args.modelPath);
    hailo_status hailoStatus = hailo.configureDefaultVStreams();
    if (hailoStatus != HAILO_SUCCESS)
    {
        cerr << "failed to initialize hailo device: "
            << hailo_get_status_message(hailoStatus) << endl;
        return static_cast<int>(hailoStatus);
    }

    cv::Mat frame, processingFrame;
    cv::VideoCapture cap = utils::getVideoCapture(
        args.deviceAddress,
        defaultCaptureWidth,
        defaultCaptureHeight);

    cv::TickMeter tick;
    hailo_status status;
    size_t outFrameSize = hailo.getOutVStreamFrameSize();
    vector<float32_t> inferenceOutput(outFrameSize);

    std::vector<int> jpgFlags;
    jpgFlags.push_back(cv::IMWRITE_JPEG_PROGRESSIVE);
    jpgFlags.push_back(1U);
    jpgFlags.push_back(cv::IMWRITE_JPEG_OPTIMIZE);
    jpgFlags.push_back(1U);

    while (true)
    {
        tick.start();
        cap >> frame;

        preProcess(frame, processingFrame);

        status = hailo.write(processingFrame, inputSize);
        if (status != HAILO_SUCCESS)
        {
            cerr << "write failed: " << hailo_get_status_message(status) << endl;
            return static_cast<int>(status);
        }

        status = hailo.read(inferenceOutput);
        if (status != HAILO_SUCCESS)
        {
            cerr << "read failed: " << hailo_get_status_message(status) << endl;
            return static_cast<int>(status);
        }

        vector<utils::Detection> detections = postProcess(inferenceOutput);
        tick.stop();

        drawDetections(frame, detections, to_string(tick.getFPS()));

        char keyPress = (char)cv::waitKey(1);
        if (keyPress == 'q' || keyPress == 'e' || keyPress == (char)27)
        {
            break;
        }
        else if (keyPress == 't')
        {
            vector<uint8_t> jpg;
            if (cv::imencode(".jpg", frame, jpg, jpgFlags))
                spawnEmailThread(jpg, args);
        }

        tick.reset();
    }

    cout << "[i] exiting, goodbye." << endl;
    cv::destroyAllWindows();
    return 0;
}
