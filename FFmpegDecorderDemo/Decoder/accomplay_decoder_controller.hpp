//
//  accomplay_decoder_controller.hpp
//  FFmpegDecorderDemo
//
//  Created by 芝麻酱 on 2020/1/16.
//  Copyright © 2020 芝麻酱. All rights reserved.
//

#ifndef accomplay_decoder_controller_hpp
#define accomplay_decoder_controller_hpp

#include <stdio.h>
#include "accompany_decoder.hpp"


#define CHANNEL_PER_FRAME    2
#define BITS_PER_CHANNEL        16
#define BITS_PER_BYTE        8
/** decode data to queue and queue size **/
#define QUEUE_SIZE_MAX_THRESHOLD 25
#define QUEUE_SIZE_MIN_THRESHOLD 20

class AccompanyDecoderController {
protected:
    FILE *pcmFile;
    
    // 伴奏的解码器
    AccompanyDecoder *accompanyDecoder;
    
    // 伴奏和原唱的采样率与解码伴奏和原唱的每个packet大小
    int accompanySampleRate;
    int accompanyPacketBufferSize;
    
public:
    AccompanyDecoderController();
    ~AccompanyDecoderController();
    
    /** 初始两个decoder,并且根据上一步算出的采样率，计算出伴奏和原唱的bufferSize **/
    void Init(const char* accompanyPath, const char* pcmFilePath);
    /** 解码操作 **/
    void Decode();
    /** 销毁这个controller **/
    void Destroy();
    
};


#endif /* accomplay_decoder_controller_hpp */
