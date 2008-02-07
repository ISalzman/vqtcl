//
//  VqView.m
//  vq4cocoa
//
//  Created by Jean-Claude Wippler on 08/09/2007.
//  Copyright 2007 Equi 4 Software. All rights reserved.
//

#import "VqView.h"


@implementation VqView

- (int) count
{
    return [[self meta] rows];
}

- (NSObject*) at:(int) row
{
    return nil;
}

- (id) init
{
    self = [super init];
    if (self != nil)
        columns = [[NSMutableArray new] retain];
    return self;
}

- (void) dealloc
{
    [columns release];
    [super dealloc];
}

- (int) rows
{
    return 3;
}

- (VqView*) meta
{
    return nil;
}

- (NSObject*) atRow:(int) row colNum:(int) col
{
    return [[columns objectAtIndex: col] at: row];
}

@end

@implementation VqView (NSTableDataSource)

- (int) numberOfRowsInTableView:(NSTableView*) tableView
{
	NSLog(@"--- numberOfRowsInTableView:%@", tableView);
	return [self rows];
}

- (id) tableView:(NSTableView*) tableView objectValueForTableColumn:(NSTableColumn*) tableColumn row:(int) row
{
    return [self atRow: row colNum: [[tableColumn identifier] intValue]];
}

@end
