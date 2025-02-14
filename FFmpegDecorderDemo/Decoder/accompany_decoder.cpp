//
//  accompany_decoder.cpp
//  FFmpegDecorderDemo
//
//  Created by 芝麻酱 on 2020/1/15.
//  Copyright © 2020 芝麻酱. All rights reserved.
//

#include "accompany_decoder.hpp"

#define LOG_TAG "AccompanyDecoder"

AccompanyDecoder::AccompanyDecoder() {
    seek_seconds = 0.0f;
    seek_req = false;
    seek_resp = false;
    accompanyFilePath = NULL;
}

AccompanyDecoder::~AccompanyDecoder() {
    if (accompanyFilePath != nullptr) {
        delete [] accompanyFilePath;
        accompanyFilePath = nullptr;
    }
}

// 获取采样率以及比特率
int AccompanyDecoder::getMusicMeta(const char *fileString, int *metaData) {
    
    init(fileString);
    int sampleRate = avCodecContext->sample_rate;
    int bitRate = avCodecContext->bit_rate;
    destroy();
    metaData[0] = sampleRate;
    metaData[1] = bitRate;
    return 0;
}

// 初始化这个decoder 即打开指定的mp3文件
void AccompanyDecoder::init(const char *fileString, int packetBufferSizeParam) {
    
    init(fileString);
    packetBufferSize = packetBufferSizeParam;
}

int AccompanyDecoder::init(const char *fileString) {
    
    audioBuffer = nullptr;
    position = -1.0f;
    audioBufferCursor = 0;
    audioBufferSize = 0;
    swrContext = nullptr;
    swrBuffer = NULL;
    swrBufferSize = 0;
    seek_success_read_frame_success = true;
    isNeedFirstFrameCorrectFlag = true;
    firstFrameCorrectionInSecs = 0.0f;
    
    /*
     configure脚本配置，编译后会生成两个文件：config.mk和config.h
     config.mk文件会作用在编译阶段
     config.h作用在运行阶段，确定要注册哪些容器以及编解码格式到FFmpeg中
     所以avcodec_register_all会注册所有config.h中开放的编解码器
     然后会注册所有的Muxer和Demuxer（封装格式）
     最后注册所有的Protocol（协议层的东西）
     */
    avcodec_register_all();
    av_register_all();
    avFormatContext = avformat_alloc_context();
    
    if (accompanyFilePath == NULL) {
        int length = (int)strlen(fileString);
        accompanyFilePath = new char[length + 1]; // 最后一个是 \0
        memset(accompanyFilePath, 0, length + 1);
        memcpy(accompanyFilePath, fileString, length + 1);
    }
    // 会根据提供的文件路径 判断文件的格式 来决定使用哪个Demuxer
    int result = avformat_open_input(&avFormatContext, fileString, NULL, NULL);
    if (result != 0) {
        LOGI("can't open file %s result is %d", fileString, result);
        return -1;
    } else {
        LOGI("open file %s success and result is %d", fileString, result);
    }
    avFormatContext->max_analyze_duration = 50000;
    
    /*
     分析文件中的流的信息
     先查找对应的解码器，然后打开对应的解码器，紧接着会利用Demuxer中的read_packet函数读取一段数据进行解码
     读取的越多，分析的越准确，但是耗费时间就越长
     可以配置参数来控制解码数据的长度
     */
    result = avformat_find_stream_info(avFormatContext, NULL);
    if (result < 0) {
        LOGI("fail avformat_find_stream_info result is %d", result);
        return -1;
    } else {
        LOGI("sucess avformat_find_stream_info result is %d", result);
    }
    stream_index = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    LOGI("stream_index is %d", stream_index);

    // 没有音频
    if (-1 == stream_index) {
        LOGI("no audio stream");
        return -1;
    }
    
    // 音频流
    AVStream *audioStream = avFormatContext->streams[stream_index];
    if (audioStream->time_base.den && audioStream->codec->time_base.num) {
        timeBase = av_q2d(audioStream->time_base);
    } else if (audioStream->codec->time_base.den && audioStream->codec->time_base.num) {
        timeBase = av_q2d(audioStream->codec->time_base);
    }
    // 获得音频流的解码器上下文
    avCodecContext = audioStream->codec;
    // 根据解码器上下文找到解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);
    
    if (NULL == avCodec) {
        LOGI("Unsupported codec ");
        return -1;
    }
    
    // 打开解码器
    result = avcodec_open2(avCodecContext, avCodec, NULL);
    if (result < 0) {
        LOGI("fail avformat_find_stream_info result is %d", result);
        return -1;
    } else {
        LOGI("sucess avformat_find_stream_info result is %d", result);
    }
    
    // 判断是否需要resample
    if (!audioCodecIsSupported()) {
        LOGI("because of audio Codec Is Not Supported so we will init swresampler...");
        /**
         * 初始化resampler
         * @param s               Swr context, can be NULL
         * @param out_ch_layout   output channel layout (AV_CH_LAYOUT_*)
         * @param out_sample_fmt  output sample format (AV_SAMPLE_FMT_*).
         * @param out_sample_rate output sample rate (frequency in Hz)
         * @param in_ch_layout    input channel layout (AV_CH_LAYOUT_*)
         * @param in_sample_fmt   input sample format (AV_SAMPLE_FMT_*).
         * @param in_sample_rate  input sample rate (frequency in Hz)
         * @param log_offset      logging level offset
         * @param log_ctx         parent logging context, can be NULL
         */
        swrContext = swr_alloc_set_opts(NULL,
                                        av_get_default_channel_layout(OUT_PUT_CHANNELS),
                                        AV_SAMPLE_FMT_S16,
                                        avCodecContext->sample_rate,
                                        av_get_default_channel_layout(avCodecContext->channels),
                                        avCodecContext->sample_fmt,
                                        avCodecContext->sample_rate, 0, NULL);
        
        if (!swrContext || swr_init(swrContext)) {
            if (swrContext) {
                swr_free(&swrContext);
            }
            avcodec_close(avCodecContext);
            LOGI("init resampler failed...");
            return -1;
        }
    }
    
    LOGI("channels is %d sampleRate is %d", avCodecContext->channels, avCodecContext->sample_rate);
    pAudioFrame = avcodec_alloc_frame();
    //    LOGI("leave AccompanyDecoder::init");
    return 1;
}

bool AccompanyDecoder::audioCodecIsSupported() {
    if (avCodecContext->sample_fmt == AV_SAMPLE_FMT_S16) {
        return true;
    }
    return false;
}

AudioPacket * AccompanyDecoder::decodePacket() {
    short *samples = new short[packetBufferSize];
    int stereoSampleSize = readSamples(samples, packetBufferSize);
    AudioPacket *samplePacket = new AudioPacket();
    if (stereoSampleSize > 0) {
        // 构造一个packet
        samplePacket->buffer = samples;
        samplePacket->size = stereoSampleSize;
        samplePacket->position = position;
    } else {
        samplePacket->size = -1;
    }
    return samplePacket;
}

int AccompanyDecoder::readSamples(short* samples, int size) {
    if(seek_req){
        audioBufferCursor = audioBufferSize;
    }
    int sampleSize = size;
    while(size > 0){
        if(audioBufferCursor < audioBufferSize){
            int audioBufferDataSize = audioBufferSize - audioBufferCursor;
            int copySize = MIN(size, audioBufferDataSize);
            memcpy(samples + (sampleSize - size), audioBuffer + audioBufferCursor, copySize * 2);
            size -= copySize;
            audioBufferCursor += copySize;
        } else{
            if(readFrame() < 0){
                break;
            }
        }
//        LOGI("size is %d", size);
    }
    int fillSize = sampleSize - size;
    if(fillSize == 0){
        return -1;
    }
    return fillSize;
}


void AccompanyDecoder::seek_frame(){
    LOGI("\n seek frame firstFrameCorrectionInSecs is %.6f, seek_seconds=%f, position=%f \n", firstFrameCorrectionInSecs, seek_seconds, position);
    float targetPosition = seek_seconds;
    float currentPosition = position;
    float frameDuration = duration;
    if(targetPosition < currentPosition){
        this->destroy();
        this->init(accompanyFilePath);
        //TODO:这里GT的测试样本会差距25ms 不会累加
        currentPosition = 0.0;
    }
    int readFrameCode = -1;
    while (true) {
        av_init_packet(&packet);
        readFrameCode = av_read_frame(avFormatContext, &packet);
        if (readFrameCode >= 0) {
            currentPosition += frameDuration;
            if (currentPosition >= targetPosition) {
                break;
            }
        }
//        LOGI("currentPosition is %.3f", currentPosition);
        av_free_packet(&packet);
    }
    seek_resp = true;
    seek_req = false;
    seek_success_read_frame_success = false;
}

int AccompanyDecoder::readFrame() {
//    LOGI("enter AccompanyDecoder::readFrame");
    if(seek_req){
        this->seek_frame();
    }
    int ret = 1;
    av_init_packet(&packet);
    int gotframe = 0;
    int readFrameCode = -1;
    while (true) {
        readFrameCode = av_read_frame(avFormatContext, &packet);
//        LOGI(" av_read_frame ...", duration);
        if (readFrameCode >= 0) {
            if (packet.stream_index == stream_index) {
                int len = avcodec_decode_audio4(avCodecContext, pAudioFrame,
                        &gotframe, &packet);
//                LOGI(" avcodec_decode_audio4 ...", duration);
                if (len < 0) {
                    LOGI("decode audio error, skip packet");
                }
                if (gotframe) {
                    int numChannels = OUT_PUT_CHANNELS;
                    int numFrames = 0;
                    void * audioData;
                    if (swrContext) {
                        const int ratio = 2;
                        const int bufSize = av_samples_get_buffer_size(NULL,
                                numChannels, pAudioFrame->nb_samples * ratio,
                                AV_SAMPLE_FMT_S16, 1);
                        if (!swrBuffer || swrBufferSize < bufSize) {
                            swrBufferSize = bufSize;
                            swrBuffer = realloc(swrBuffer, swrBufferSize);
                        }
                        byte *outbuf[2] = { (byte*) swrBuffer, NULL };
                        numFrames = swr_convert(swrContext, outbuf,
                                pAudioFrame->nb_samples * ratio,
                                (const uint8_t **) pAudioFrame->data,
                                pAudioFrame->nb_samples);
                        if (numFrames < 0) {
                            LOGI("fail resample audio");
                            ret = -1;
                            break;
                        }
                        audioData = swrBuffer;
                    } else {
                        if (avCodecContext->sample_fmt != AV_SAMPLE_FMT_S16) {
                            LOGI("bucheck, audio format is invalid");
                            ret = -1;
                            break;
                        }
                        audioData = pAudioFrame->data[0];
                        numFrames = pAudioFrame->nb_samples;
                    }
                    if(isNeedFirstFrameCorrectFlag && position >= 0){
                        float expectedPosition = position + duration;
                        float actualPosition = av_frame_get_best_effort_timestamp(pAudioFrame) * timeBase;
                        firstFrameCorrectionInSecs = actualPosition - expectedPosition;
                        isNeedFirstFrameCorrectFlag = false;
                    }
                    duration = av_frame_get_pkt_duration(pAudioFrame) * timeBase;
                    position = av_frame_get_best_effort_timestamp(pAudioFrame) * timeBase - firstFrameCorrectionInSecs;
                    if (!seek_success_read_frame_success) {
                        LOGI("position is %.6f", position);
                        actualSeekPosition = position;
                        seek_success_read_frame_success = true;
                    }
                    audioBufferSize = numFrames * numChannels;
//                    LOGI(" \n duration is %.6f position is %.6f audioBufferSize is %d\n", duration, position, audioBufferSize);
                    audioBuffer = (short*) audioData;
                    audioBufferCursor = 0;
                    break;
                }
            }
        } else {
            ret = -1;
            break;
        }
    }
    av_free_packet(&packet);
    return ret;
}

void AccompanyDecoder::destroy() {
//    LOGI("start destroy!!!");
    if (NULL != swrBuffer) {
        free(swrBuffer);
        swrBuffer = NULL;
        swrBufferSize = 0;
    }
    if (NULL != swrContext) {
        swr_free(&swrContext);
        swrContext = NULL;
    }
    if (NULL != pAudioFrame) {
        av_free (pAudioFrame);
        pAudioFrame = NULL;
    }
    if (NULL != avCodecContext) {
        avcodec_close(avCodecContext);
        avCodecContext = NULL;
    }
    if (NULL != avFormatContext) {
        LOGI("leave LiveReceiver::destory");
        avformat_close_input(&avFormatContext);
        avFormatContext = NULL;
    }
//    LOGI("end destroy!!!");
}


