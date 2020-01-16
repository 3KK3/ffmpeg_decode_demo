//
//  CommonUtil.h
//  FFmpegDecorderDemo
//
//  Created by 芝麻酱 on 2020/1/16.
//  Copyright © 2020 芝麻酱. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface CommonUtil : NSObject

+(NSString *)bundlePath:(NSString *)fileName;

+(NSString *)documentsPath:(NSString *)fileName;

@end

NS_ASSUME_NONNULL_END
