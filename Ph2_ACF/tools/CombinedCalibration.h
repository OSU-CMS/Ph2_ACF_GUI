#ifndef __COMBINED_CALIBRATION__
#define __COMBINED_CALIBRATION__

#include "MetadataHandlerOT.h"
#include "tools/Tool.h"
#include <iostream>

#if __cplusplus < 201402
namespace std
{
template <size_t... Is>
struct index_sequence
{
};

template <size_t N, size_t... Is>
struct make_index_sequence_impl
{
    using type = typename make_index_sequence_impl<N - 1, N - 1, Is...>::type;
};

template <size_t N, size_t... Is>
struct make_index_sequence_impl<N, 0, Is...>
{
    using type = index_sequence<0, Is...>;
};

template <size_t N>
using make_index_sequence = typename make_index_sequence_impl<N>::type;
} // namespace std
#endif

template <class... Tools>
struct CombinedCalibration : public Tool
{
  public:
    static constexpr int size = sizeof...(Tools);

    CombinedCalibration() : current_tool(this) {}

    void Running() override { start_impl(std::make_index_sequence<size>()); }
    void Stop() override
    {
        Tool::Stop();
        Tool::CloseResultFile();
    }

  private:
    template <size_t... Is>
    void start_impl(std::index_sequence<Is...>)
    {
        __attribute__((unused)) auto _ = {(start_single(std::get<Is>(tools)), 0)...};
    }

    template <class T>
    void start_single(T& tool)
    {
        fMetadataHandler->fillSubCalibrationNameAndTimeContainer(tool.getCalibrationName());
        tool.Inherit(current_tool);
        tool.ConfigureCalibration();
        tool.Running();
        tool.Stop();
        tool.resetPointers();
        current_tool = &tool;
    }

    Tool*                current_tool;
    std::tuple<Tools...> tools;
};

#endif
