#include "ScreenCaptureKitScreencap.h"

#include "MaaUtils/Logger.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>
#include <ScreenCaptureKit/ScreenCaptureKit.h>

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> ScreenCaptureKitScreencap::screencap()
{
    cv::Mat result;

    // 检查macOS版本
    if (@available(macOS 14.0, *)) {
        // 使用GCD信号量进行同步
        dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
        __block cv::Mat captured_image;
        __block bool capture_success = false;

        // 异步获取可共享内容
        [SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent* content, NSError* error) {
            if (error || !content) {
                LogError << "Failed to get shareable content";
                dispatch_semaphore_signal(semaphore);
                return;
            }

            // 查找目标窗口
            SCWindow* targetWindow = nil;
            if (window_id_ != 0) {
                for (SCWindow* window in content.windows) {
                    if (window.windowID == window_id_) {
                        targetWindow = window;
                        break;
                    }
                }
            }
            else {
                // 如果window_id为0，使用主显示器
                if (content.displays.count > 0) {
                    SCDisplay* mainDisplay = content.displays[0];
                    // 对于桌面截图，我们需要创建一个不同的过滤器
                    SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:mainDisplay excludingWindows:@[]];
                    // 创建截图配置
                    SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
                    config.width = mainDisplay.width;
                    config.height = mainDisplay.height;
                    config.pixelFormat = kCVPixelFormatType_32BGRA;
                    config.colorSpaceName = kCGColorSpaceSRGB;

                    // 执行截图
                    [SCScreenshotManager
                        captureSampleBufferWithFilter:filter
                                        configuration:config
                                    completionHandler:^(CMSampleBufferRef sampleBuffer, NSError* error) {
                                        if (error || !sampleBuffer) {
                                            LogError << "Screenshot failed";
                                            [filter release];
                                            [config release];
                                            dispatch_semaphore_signal(semaphore);
                                            return;
                                        }

                                        // 处理图像数据
                                        CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
                                        if (imageBuffer) {
                                            CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

                                            void* baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
                                            size_t width = CVPixelBufferGetWidth(imageBuffer);
                                            size_t height = CVPixelBufferGetHeight(imageBuffer);
                                            size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);

                                            if (baseAddress && width > 0 && height > 0) {
                                                cv::Mat raw_image = cv::Mat((int)height, (int)width, CV_8UC4, baseAddress, bytesPerRow);
                                                cv::cvtColor(raw_image, captured_image, cv::COLOR_BGRA2BGR);
                                                capture_success = true;
                                            }

                                            CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
                                        }

                                        [filter release];
                                        [config release];
                                        dispatch_semaphore_signal(semaphore);
                                    }];
                    return;
                }
            }

            if (!targetWindow) {
                LogError << "Target window not found";
                dispatch_semaphore_signal(semaphore);
                return;
            }

            // 创建内容过滤器
            SCContentFilter* filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:targetWindow];
            if (!filter) {
                LogError << "Failed to create content filter";
                dispatch_semaphore_signal(semaphore);
                return;
            }

            // 创建截图配置
            SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
            config.width = targetWindow.frame.size.width;
            config.height = targetWindow.frame.size.height;
            config.pixelFormat = kCVPixelFormatType_32BGRA;
            config.colorSpaceName = kCGColorSpaceSRGB;

            // 执行截图
            [SCScreenshotManager captureSampleBufferWithFilter:filter
                                                 configuration:config
                                             completionHandler:^(CMSampleBufferRef sampleBuffer, NSError* error) {
                                                 if (error || !sampleBuffer) {
                                                     LogError << "Screenshot failed";
                                                     [filter release];
                                                     [config release];
                                                     dispatch_semaphore_signal(semaphore);
                                                     return;
                                                 }

                                                 // 处理图像数据
                                                 CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
                                                 if (imageBuffer) {
                                                     CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

                                                     void* baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
                                                     size_t width = CVPixelBufferGetWidth(imageBuffer);
                                                     size_t height = CVPixelBufferGetHeight(imageBuffer);
                                                     size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);

                                                     if (baseAddress && width > 0 && height > 0) {
                                                         cv::Mat raw_image = cv::Mat((int)height, (int)width, CV_8UC4, baseAddress, bytesPerRow);
                                                         cv::cvtColor(raw_image, captured_image, cv::COLOR_BGRA2BGR);
                                                         capture_success = true;
                                                     }

                                                     CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
                                                 }

                                                 [filter release];
                                                 [config release];
                                                 dispatch_semaphore_signal(semaphore);
                                             }];
        }];

        // 等待完成
        if (dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC)) == 0 && capture_success) {
            result = std::move(captured_image);
            return result;
        }
    }
    else {
        LogError << "SCScreenshotManager requires macOS 14.0 or later";
    }

    return std::nullopt;
}

MAA_CTRL_UNIT_NS_END
