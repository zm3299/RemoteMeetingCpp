#include <iostream>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavdevice/avdevice.h>
}

char* get_screen_shot() {
    avdevice_register_all();
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Error allocating AVFrame" << std::endl;
        return nullptr;
    }

    AVFormatContext *format_ctx = avformat_alloc_context();
    if (!format_ctx) {
        std::cerr << "Error allocating AVFormatContext" << std::endl;
        av_frame_free(&frame);
        return nullptr;
    }

    AVDictionary *options = NULL;
    av_dict_set(&options, "framerate", "30", 0); // 设置帧率
    
    if(av_find_input_format("gdigrab") == nullptr){
        std::cerr << "bye" << std::endl;
        return nullptr;
    }

    if (avformat_open_input(&format_ctx, "desktop", av_find_input_format("gdigrab"), &options) != 0) {
        std::cerr << "Error opening input" << std::endl;
        avformat_free_context(format_ctx);
        av_frame_free(&frame);
        return nullptr;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        std::cerr << "Error finding stream info" << std::endl;
        avformat_close_input(&format_ctx);
        av_frame_free(&frame);
        return nullptr;
    }

    const AVCodec *codec = avcodec_find_decoder(format_ctx->streams[0]->codecpar->codec_id);
    if (!codec) {
        std::cerr << "Error finding codec" << std::endl;
        avformat_close_input(&format_ctx);
        av_frame_free(&frame);
        return nullptr;
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(const_cast<AVCodec*>(codec)); // 注意这里的类型转换
    if (!codec_ctx) {
        std::cerr << "Error allocating codec context" << std::endl;
        avformat_close_input(&format_ctx);
        av_frame_free(&frame);
        return nullptr;
    }

    if (avcodec_parameters_to_context(codec_ctx, format_ctx->streams[0]->codecpar) < 0) {
        std::cerr << "Error setting codec parameters" << std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        av_frame_free(&frame);
        return nullptr;
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        std::cerr << "Error opening codec" << std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        av_frame_free(&frame);
        return nullptr;
    }

    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Error allocating AVPacket" << std::endl;
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        av_frame_free(&frame);
        return nullptr;
    }

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == 0) { // Video stream
            int ret = avcodec_send_packet(codec_ctx, packet);
            if (ret < 0) {
                std::cerr << "Error sending packet for decoding" << std::endl;
                av_packet_unref(packet);
                av_packet_free(&packet);
                avcodec_free_context(&codec_ctx);
                avformat_close_input(&format_ctx);
                av_frame_free(&frame);
                return nullptr;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    std::cerr << "Error during decoding" << std::endl;
                    av_packet_unref(packet);
                    av_packet_free(&packet);
                    avcodec_free_context(&codec_ctx);
                    avformat_close_input(&format_ctx);
                    av_frame_free(&frame);
                    return nullptr;
                }

                // Allocate buffer for storing the screenshot
                int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height, 1);
                uint8_t* buffer = (uint8_t*)av_malloc(buffer_size);
                if (!buffer) {
                    std::cerr << "Error allocating buffer for screenshot" << std::endl;
                    av_packet_unref(packet);
                    av_packet_free(&packet);
                    avcodec_free_context(&codec_ctx);
                    avformat_close_input(&format_ctx);
                    av_frame_free(&frame);
                    return nullptr;
                }

                // Fill the buffer with RGB data
                av_image_copy_to_buffer(buffer, buffer_size, frame->data, frame->linesize, AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height, 1);

                // Clean up
                av_packet_unref(packet);
                av_packet_free(&packet);
                avcodec_free_context(&codec_ctx);
                avformat_close_input(&format_ctx);
                av_frame_free(&frame);

                // Convert the buffer to a char* for returning
                char* image_data = (char*)malloc(buffer_size);
                if (!image_data) {
                    std::cerr << "Error allocating memory for image data" << std::endl;
                    av_free(buffer);
                    return nullptr;
                }
                memcpy(image_data, buffer, buffer_size);
                av_free(buffer);
                
                return image_data;
            }
        }
    }

    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    av_frame_free(&frame);

    return nullptr;
}

int main() {
    printf("hello world\n");
    char* image_data = get_screen_shot();
    printf("%p\n", image_data);
    if (image_data != nullptr) {
        printf("%s", image_data);
        free(image_data);
    } else {
        std::cerr << "Error getting screen shot" << std::endl;
    }

    return 0;
}
