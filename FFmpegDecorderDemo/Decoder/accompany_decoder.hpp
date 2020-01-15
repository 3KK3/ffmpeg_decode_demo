//
//  accompany_decoder.hpp
//  FFmpegDecorderDemo
//
//  Created by 芝麻酱 on 2020/1/15.
//  Copyright © 2020 芝麻酱. All rights reserved.
//

#ifndef accompany_decoder_hpp
#define accompany_decoder_hpp

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef UINT64_C
#define UINT64_C(value) __CONCAT(value, ULL)
#endif

#ifndef INT64_MIN
#define INT64_MIN  (-9223372036854775807LL - 1)
#endif

#ifndef INT64_MAX
#define INT64_MAX    9223372036854775807LL
#endif

#define byte uint8_t
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define LOGI(...)  printf("  ");printf(__VA_ARGS__); printf("\t -  <%s> \n", LOG_TAG);


typedef struct AudioPacket {
    
    static const int AUDIO_PACKET_ACTION_PALY = 0;
    static const int AUDIO_PAKCET_ACTION_PAUSE = 100;
    static const int AUDIO_PACKET_ACTION_SEEK = 101;
    
    short *buffer;
    int size;
    float position;
    int action;
    
    float extra_param1;
    float extra_param2;
    
    AudioPacket() {
        buffer = nullptr;
        size = 0;
        position = -1;
        action = 0;
        extra_param1 = 0;
        extra_param2 = 0;
    }
    
    ~AudioPacket() {
        if (buffer != nullptr) {
            delete [] buffer;
            buffer = nullptr;
        }
    }
    
} AudioPacket;

// 记得header search path包含进  ，下面库编译需要用到。 lib文件夹链接需要用到
// 需要关闭bitcode  因为编译ffmpeg的时候没有开启bitcode
extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/avutil.h"
    #include "libavutil/samplefmt.h"
    #include "libavutil/common.h"
    #include "libavutil/channel_layout.h"
    #include "libavutil/opt.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/mathematics.h"
    #include "libswscale/swscale.h"
    #include "libswresample/swresample.h"
}

#define OUT_PUT_CHANNELS 2

class AccompanyDecoder {
private:
    // 如果使用了快进或者快退命令， 则先设置以下参数 👇
    bool seek_req;
    bool seek_resp;
    float seek_seconds;
    
    float actualSeekPosition;
    
    // 对容器或者说媒体文件层次的抽象，包含多路流（音频流、视频流、字幕流等）
    AVFormatContext *avFormatContext;
    // 描述每一路流的编码格式 包含在一个AVStream里面 ，可以根据此打开编解码器，完成AVFrame和AVPacket直接的转换（编、解码）
    AVCodecContext *avCondecContext;
    int stream_index;
    float timeBase;
    //编、解码器输入输出部分中  原始数据的抽象
    AVFrame *pAudioFrame;
    //编、解码器输入输出部分中  压缩数据的抽象
    AVPacket packet;
    
    char *accompanyFilePath;
    
    bool seek_success_read_frame_success;
    int packetBufferSize;
    
    // 每次解码出来的audioBuffer以及这个audioBuffer的时间戳 以及当前类对于这个audiioBuffer的操作情况 👇
    short *audioBuffer;
    float positon;
    int audioBufferCursor;
    int audioBufferSize;
    float duration;
    bool isNeedFirstFrameCorrectFlag;
    float firstFrameCorrectionInSecs;
    
    SwrContext *swrContext;
    void *swrBuffer;
    int swrBufferSize;
    
    int init(const char *fileString);
    int readSamples(short *samples, int size);
    int readFrame();
    bool audioCodecIsSupported();
    
    
public:
    AccompanyDecoder();
    virtual ~AccompanyDecoder();
    
    // 获取采样率以及比特率
    virtual int getMusicMeta(const char *fileString, int *metaData);
    
    // 初始化这个decoder 即打开指定的mp3文件
    virtual void init(const char *fileString, int packetBufferSizeParam);
    virtual AudioPacket *decodePacket();
    
    // 销毁decoder
    virtual void destroy();
    // 这函数写的怎么这么迷....
    void setSeekReq(bool seekReqParam) {
        seek_req = seekReqParam;
        if (seek_req) {
            seek_req = false;
        }
    }
    
    bool hasSeekReq() {
        return seek_req;
    };
    
    bool hasSeekResp() {
        return seek_resp;
    }
    
    // 设置播放到什么位置 单位是秒 但是后边3位小数 起始是精确到毫秒
    void setPosition(float seconds) {
        actualSeekPosition = -1;
        seek_seconds = seconds;
        seek_req = true;
        seek_resp = false;
    };
    
    float getActualSeekPosition() {
        float ret = actualSeekPosition;
        if (ret != -1) {
            actualSeekPosition = -1;
        }
        return ret;
    };

    virtual void seek_frame();
};


#endif /* accompany_decoder_hpp */
