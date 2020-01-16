//
//  ViewController.m
//  FFmpegDecorderDemo
//
//  Created by 芝麻酱 on 2020/1/15.
//  Copyright © 2020 芝麻酱. All rights reserved.
//

#import "ViewController.h"
#import "accomplay_decoder_controller.hpp"
#import "CommonUtil.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

- (IBAction)startDecode:(id)sender {
    NSLog(@"decode Test...");
    const char* mp3FilePath = [[CommonUtil bundlePath:@"131.aac"] cStringUsingEncoding:NSUTF8StringEncoding];
    const char *pcmFilePath = [[CommonUtil documentsPath:@"131.pcm"] cStringUsingEncoding:NSUTF8StringEncoding];
    
    AccompanyDecoderController* decoderController = new AccompanyDecoderController();
    decoderController->Init(mp3FilePath, pcmFilePath);
    decoderController->Decode();
    decoderController->Destroy();
    delete decoderController;
    NSLog(@"After decode Test...");
    
}

@end
