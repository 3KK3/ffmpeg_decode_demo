//
//  CommonUtil.m
//  FFmpegDecorderDemo
//
//  Created by 芝麻酱 on 2020/1/16.
//  Copyright © 2020 芝麻酱. All rights reserved.
//

#import "CommonUtil.h"
#import <UIKit/UIKit.h>

@implementation CommonUtil

+(NSString *)bundlePath:(NSString *)fileName {
    return [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:fileName];
}

+(NSString *)documentsPath:(NSString *)fileName {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    return [documentsDirectory stringByAppendingPathComponent:fileName];
}

@end
