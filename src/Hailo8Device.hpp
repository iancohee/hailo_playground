#ifndef HAILO8_DEVICE_H
#define HAILO8_DEVICE_H

#include <opencv2/core.hpp>
#include <hailo/hailort.hpp>

#include <vector>


class Hailo8Device
{
public:
    static Hailo8Device create(const std::string& hef);

    ~Hailo8Device () = default;

    hailo_status configureDefaultVStreams ();

    hailo_status write (const hailort::MemoryView& memoryView);
    hailo_status write (const cv::Mat& frame, size_t size);

    template<typename T>
    hailo_status read (std::vector<T>& out);

    const hailort::Hef& getHef () const;
    const hailo_device_identity_t& getId () const;

    size_t getInVStreamFrameSize () const;
    size_t getOutVStreamFrameSize () const;

private:
    Hailo8Device (hailort::Hef&&);

    std::unique_ptr<hailort::Device> device;
    hailort::Hef hef;
    std::shared_ptr<hailort::ConfiguredNetworkGroup> configuredNetworkGroup;
    std::unique_ptr<hailort::ActivatedNetworkGroup> activatedNetworkGroup;
    std::vector<hailort::InputVStream> inVStreams;
    std::vector<hailort::OutputVStream> outVStreams;
    hailo_device_identity_t deviceId;
};

template<typename T>
hailo_status
Hailo8Device::read (std::vector<T>& out)
{
    auto& ostream = outVStreams.at(0);
    return ostream.read(hailort::MemoryView(out.data(), out.size()));
}

#endif // HAILO8_DEVICE_H