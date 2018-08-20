// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "common/param_package.h"
#include "core/frontend/input.h"
#include "core/settings.h"
#include "input_common/udp/client.h"
#include "input_common/udp/udp.h"

namespace InputCommon::CemuhookUDP {

class UDPTouchDevice final : public Input::TouchDevice {
public:
    explicit UDPTouchDevice(std::shared_ptr<DeviceStatus> status_) : status(std::move(status_)) {}
    std::tuple<float, float, bool> GetStatus() const {
        std::lock_guard<std::mutex> guard(status->update_mutex);
        return status->touch_status;
    }

private:
    std::shared_ptr<DeviceStatus> status;
};

class UDPMotionDevice final : public Input::MotionDevice {
public:
    explicit UDPMotionDevice(std::shared_ptr<DeviceStatus> status_) : status(std::move(status_)) {}
    std::tuple<Math::Vec3<float>, Math::Vec3<float>> GetStatus() const {
        std::lock_guard<std::mutex> guard(status->update_mutex);
        return status->motion_status;
    }

private:
    std::shared_ptr<DeviceStatus> status;
};

class UDPTouchFactory final : public Input::Factory<Input::TouchDevice> {
public:
    explicit UDPTouchFactory(std::shared_ptr<DeviceStatus> status_) : status(std::move(status_)) {}

    std::unique_ptr<Input::TouchDevice> Create(const Common::ParamPackage& params) override {
        {
            std::lock_guard<std::mutex> guard(status->update_mutex);
            status->touch_calibration.reset({});
            // These default values work well for DS4 but probably not other touch inputs
            status->touch_calibration->min_x = params.Get("min_x", 100);
            status->touch_calibration->min_y = params.Get("min_y", 50);
            status->touch_calibration->max_x = params.Get("max_x", 1800);
            status->touch_calibration->max_y = params.Get("max_y", 850);
        }
        return std::make_unique<UDPTouchDevice>(status);
    }

private:
    std::shared_ptr<DeviceStatus> status;
};

class UDPMotionFactory final : public Input::Factory<Input::MotionDevice> {
public:
    explicit UDPMotionFactory(std::shared_ptr<DeviceStatus> status_) : status(std::move(status_)) {}

    std::unique_ptr<Input::MotionDevice> Create(const Common::ParamPackage& params) override {
        return std::make_unique<UDPMotionDevice>(status);
    }

private:
    std::shared_ptr<DeviceStatus> status;
};

State::State() {
    auto status = std::make_shared<DeviceStatus>();
    client =
        std::make_unique<Client>(status, Settings::values.udp_input_address,
                                 Settings::values.udp_input_port, Settings::values.udp_pad_index);

    Input::RegisterFactory<Input::TouchDevice>("cemuhookudp",
                                               std::make_shared<UDPTouchFactory>(status));
    Input::RegisterFactory<Input::MotionDevice>("cemuhookudp",
                                                std::make_shared<UDPMotionFactory>(status));
}

State::~State() {
    Input::UnregisterFactory<Input::TouchDevice>("cemuhookudp");
    Input::UnregisterFactory<Input::MotionDevice>("cemuhookudp");
}

std::unique_ptr<State> Init() {
    return std::make_unique<State>();
}
} // namespace InputCommon::CemuhookUDP