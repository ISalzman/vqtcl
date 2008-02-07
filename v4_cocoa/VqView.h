//
//  VqView.h
//  vq4cocoa
//
//  Created by Jean-Claude Wippler on 08/09/2007.
//  Copyright 2007 Equi 4 Software. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@protocol VqSequence

- (int) count;
- (NSObject*) at:(int) row;

@end

@interface VqView : NSObject <VqSequence> {
    NSMutableArray* columns;
}

- (int) rows;
- (VqView*) meta;
- (NSObject*) atRow:(int) row colNum:(int) col;

@end
