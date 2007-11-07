/* Vlerq extension for Objective-C */

#import <Cocoa/Cocoa.h>

@interface VQTable : NSObject {
	union vq_Item_u *table;
}

- (int) size;

@end
