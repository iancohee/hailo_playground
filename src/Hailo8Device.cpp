#include "Hailo8Device.hpp"

#include <hailo/hailort.hpp>

#include <iostream>
#include <stdexcept>

Hailo8Device
Hailo8Device::create (
    const std::string& path
)
{
    auto hef_result = hailort::Hef::create(path);
    if (!hef_result)
    {
        throw std::runtime_error("failed to create device object from hef");
    }
    auto hef = hef_result.release(); 
    return Hailo8Device(std::move(hef));
}

Hailo8Device::Hailo8Device (
    hailort::Hef&& inHef
)
: hef(std::move(inHef))
{
    auto dev_exp = hailort::Device::create();
    if (!dev_exp)
    {
        throw std::runtime_error("failed to create hailo8 device");
    }
    device = dev_exp.release();

    auto device_identity_result = device->identify();
    if (!device_identity_result)
    {
        throw std::runtime_error("failed to get device identity");
    }
    deviceId = device_identity_result.release();
}

const hailort::Hef& Hailo8Device::getHef (
    void
) const
{
    return hef;
}

hailo_status
Hailo8Device::configureDefaultVStreams (
    void
)
{
    auto config_params_result = hef.create_configure_params(HAILO_STREAM_INTERFACE_PCIE);
    if (!config_params_result)
        return config_params_result.status();

    auto config_params = config_params_result.release();

    auto network_groups_result = device->configure(hef, config_params);
    if (!network_groups_result)
        return network_groups_result.status();

    auto network_groups = network_groups_result.release();
    if (network_groups.size() != 1)
    {
        std::cerr << "wrong number of network groups: " << network_groups.size() << std::endl;
        return HAILO_INVALID_ARGUMENT;
    }
    assert(network_groups.size() == 1);
    configuredNetworkGroup = network_groups.at(0);

    auto activated_network_groups_res = configuredNetworkGroup->activate();
    if (!activated_network_groups_res)
        return activated_network_groups_res.status();

    activatedNetworkGroup = activated_network_groups_res.release();

    auto input_params_res = configuredNetworkGroup->make_input_vstream_params(
        true,
        HAILO_FORMAT_TYPE_AUTO,
        HAILO_DEFAULT_VSTREAM_TIMEOUT_MS,
        HAILO_DEFAULT_VSTREAM_QUEUE_SIZE);
    if (!input_params_res)
        return input_params_res.status();

    auto input_params = input_params_res.value();
    auto output_params_res = configuredNetworkGroup->make_output_vstream_params(
        false,
        HAILO_FORMAT_TYPE_AUTO,
        HAILO_DEFAULT_VSTREAM_TIMEOUT_MS,
        HAILO_DEFAULT_VSTREAM_QUEUE_SIZE);
    if (!output_params_res)
        return output_params_res.status();

    auto output_params = output_params_res.value();

    auto input_vstreams_res = hailort::VStreamsBuilder::create_input_vstreams(
        *configuredNetworkGroup,
        input_params);

    if (!input_vstreams_res)
        return input_vstreams_res.status();

    inVStreams = input_vstreams_res.release();

    auto output_vstreams_res = hailort::VStreamsBuilder::create_output_vstreams(
        *configuredNetworkGroup,
        output_params);

    if (!output_vstreams_res)
        return output_vstreams_res.status(); 

    outVStreams = output_vstreams_res.release();

    return HAILO_SUCCESS;
}

hailo_status
Hailo8Device::write (
    const hailort::MemoryView& memoryView
)
{
    auto& istream = inVStreams.at(0);
    return istream.write(memoryView);
}

hailo_status
Hailo8Device::write (
    const cv::Mat& frame,
    size_t inputSize
)
{
    return write(hailort::MemoryView(frame.data, inputSize));
}

size_t
Hailo8Device::getInVStreamFrameSize (
    void
) const
{
    return inVStreams.at(0).get_frame_size();
}

size_t
Hailo8Device::getOutVStreamFrameSize (
    void
) const
{
    return outVStreams.at(0).get_frame_size();
}

const hailo_device_identity_t&
Hailo8Device::getId (
    void
) const
{
    return deviceId;
}
