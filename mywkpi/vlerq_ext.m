#include "vlerq.h"

#import <WebKit/WebKit.h>

//-----------------------------------------------------------------------------

@interface VqView : NSObject {
  View_p _vqView;
}

- (id) initRawView: (View_p) arg;
- (View_p) rawView;

@end

@interface VqCol : NSObject {
  Column _vqColumn;
}

- (id) initRawColumn: (Column) arg;
- (Column) rawColumn;

@end

@interface VlerqPlugin : NSView <WebPlugInViewFactory> { }
@end

//-----------------------------------------------------------------------------

typedef struct CmdDispatch {
  const char *name, *args;
  ItemTypes (*proc) (Item_p);
} CmdDispatch;

static CmdDispatch f_commands[] = {
#include "vlerq_ext.h"
  { NULL, NULL, NULL }
};

static int FindCmdIndex (const char* name) {
  int i;
  
  /* TODO: could use binary search, since the table is sorted */
  for (i = 0; f_commands[i].name != NULL; ++i)
    if (*name == *f_commands[i].name && strcmp(name, f_commands[i].name) == 0)
      return i;
      
  return -1;
}

ItemTypes OpenCmd_S (Item args[]) {
  Assert(0);
  return IT_unknown;
}

ItemTypes WriteCmd_VO (Item args[]) {
  Assert(0);
  return IT_unknown;
}

View_p ObjAsMetaView (Object_p obj) {
  Assert(0);
  return NULL;
}

void FailedAssert (const char *msg, const char *file, int line) {
  NSLog(@"Failed assertion at %s, line %d: %s", file, line, msg);
  abort();
}

int ColumnByName (View_p meta, Object_p obj) {
  if ([obj isKindOfClass: [NSNumber class]]) {
    int colnum = [obj intValue];
    if (colnum < 0)
      colnum += ViewSize(meta);
    if (colnum < 0 || colnum >= ViewSize(meta))
      return -5;
    return colnum;
  }

  return StringLookup([obj UTF8String], ViewCol(meta, MC_name));
}

struct Shared *GetShared (void) {
  static struct Shared data;
  return &data;
}

Object_p ItemAsObj (ItemTypes type, Item_p item) {
  Object_p result;
  
  switch (type) {
    case IT_int:
      result = [NSNumber numberWithInt: item->i]; break;
    case IT_wide:
      result = [NSNumber numberWithLongLong: item->w]; break;
    case IT_float:
      result = [NSNumber numberWithFloat: item->f]; break;
    case IT_double:
      result = [NSNumber numberWithDouble: item->d]; break;
    case IT_string:
      result = [NSString stringWithUTF8String: item->s]; break;
    case IT_bytes:
      result = [NSData dataWithBytes: item->u.ptr length: item->u.len]; break;
    case IT_column:
      result = [[VqCol alloc] initRawColumn: item->c]; break;
    case IT_view:
      result = [[VqView alloc] initRawView: item->v]; break;
    case IT_object:
      result = item->o; break;
    default:
      Assert(0); // unsupported type
  }
  
  return result;
}

static void ArrayObjCleaner (Seq_p seq) {
  id obj = seq->data[0].p;
  [obj release];
}

static ItemTypes ArrayObjGetter (int row, Item_p item) {
  id obj = item->c.seq->data[0].p;
  item->o = [obj objectAtIndex: row];
  return IT_object;
}

static struct SeqType ST_ArrayObj = {
  "arrayobj", ArrayObjGetter, 0, ArrayObjCleaner
};

Column ObjAsColumn (Object_p obj) {
  Seq_p seq;
  
  if ([obj isKindOfClass: [VqCol class]])
    return [obj rawColumn];
  
  Assert([obj respondsToSelector: @selector(objectAtIndex:)]);

  seq = NewSequence([obj count], &ST_ArrayObj, 0);
  seq->data[0].p = [obj retain];
  return SeqAsCol(seq);
}

View_p ObjAsView (Object_p obj) {
  if ([obj isKindOfClass: [NSNumber class]]) 
    return NoColumnView([obj intValue]);
  else
    return [obj rawView];
}

int ObjToItem (ItemTypes type, Item_p item) {
  switch (type) {
    
    case IT_int:
      item->i = [item->o intValue]; break;
    case IT_wide:
      item->w = [item->o longLongValue]; break;
    case IT_float:
      item->f = [item->o floatValue]; break;
    case IT_double:
      item->d = [item->o doubleValue]; break;
    case IT_string:
      item->s = [item->o UTF8String]; break;
    case IT_bytes:
      item->u.ptr = [item->o bytes]; item->u.len = [item->o length]; break;
    case IT_object:
      break;
    case IT_column:
      item->c = [item->o rawColumn]; break;

    case IT_view:
      if ([item->o isKindOfClass: [NSNumber class]]) 
        item->v = NoColumnView([item->o intValue]);
      else
        item->v = [item->o rawView];
      break;

    default:
      return 0;
  }
  
  return 1;
}

Object_p NeedMutable (Object_p obj) {
  return obj;
}

ItemTypes AtCmd_VIO (Item_p a) {
  int col;
  View_p view;
  
  view = a[0].v;
  col = ColumnByName(V_Meta(view), a[2].o);
  if (col < 0)
    return IT_unknown;
  
  a->c = ViewCol(view, col);
  return GetItem(a[1].i, a);
}

//-----------------------------------------------------------------------------

@implementation VqView

- (id) initRawView: (View_p) arg {
  self = [super init];
  _vqView = IncRefCount(arg);
  return self;
}

- (void) dealloc {
  DecRefCount(_vqView);
  [super dealloc];
}

- (View_p) rawView {
  return _vqView;
}

#define MAX_STACK 20

- (id) invokeUndefinedMethodFromWebScript: (NSString *) name withArguments: (NSArray *) argv {
  //NSLog(@"-invokeUndefinedMethodFromWebScript: %@ argv %@", name, argv);
  
  id result = nil;
  PUSH_KEEP_REFS
  
  int index = FindCmdIndex([name UTF8String]);
  if (index >= 0) {  
    int i, argc = [argv count] + 1;
    const char *args = f_commands[index].args + 2;
    Item stack [MAX_STACK];

    for (i = 0; args[i] != 0; ++i) {
      Assert(i < MAX_STACK);
      if (args[i] == 'X') {
        Assert(args[i+1] == 0);
        NSRange r;
        r.location = i;
        r.length = argc-i;
        stack[i].u.ptr = (const void*) [argv subarrayWithRange: r];
        stack[i].u.len = argc-i;
        break;
      }
      if ((args[i] == 0 && i != argc) || (args[i] != 0 && i >= argc))
        Assert(0); // wrong number of arguments
      stack[i].o = i == 0 ? self : [argv objectAtIndex: i-1];
      if (!CastObjToItem(args[i], stack+i))
        Assert(0); // error in arg i
    }

    ItemTypes type = f_commands[index].proc(stack);
    if (type == IT_unknown)
      Assert(0); // error in result
  
    result = ItemAsObj(type, stack);
  }
  
  POP_KEEP_REFS
  return result;
}

@end

//-----------------------------------------------------------------------------

@implementation VqCol

- (id) initRawColumn: (Column) arg {
  self = [super init];
  _vqColumn = arg;
  IncRefCount(arg.seq);
  return self;
}

- (void) dealloc {
  DecRefCount(_vqColumn.seq);
  [super dealloc];
}

- (Column) rawColumn {
  return _vqColumn;
}

@end

//-----------------------------------------------------------------------------

@implementation VlerqPlugin

//- (void) webPlugInInitialize { NSLog(@"webPlugInInitialize"); }
//- (void) webPlugInStart { NSLog(@"webPlugInStart"); }
//- (void) webPlugInStop { NSLog(@"webPlugInStop"); }

+ (NSView *) plugInViewWithArguments: (NSDictionary *) arguments {
  //NSLog(@"plugInViewWithArguments %@", arguments);
  VlerqPlugin *plugin = [[[self alloc] init] autorelease];

  // security: only expose vlerq to pages which are on the local file system
  if ([[arguments objectForKey: WebPlugInBaseURLKey] isFileURL]) {
    WebFrame *wf = [[arguments objectForKey: WebPlugInContainerKey] webFrame];
    [[[wf webView] windowScriptObject] setValue: plugin forKey: @"vlerq"];
  }

  return plugin;
}

+ (BOOL) isSelectorExcludedFromWebScript: (SEL) s {
  return s == @selector(emptyView:) || s == @selector(openFile:) ? NO : YES;
}

- (VqView *) emptyView: (int) rows {
  VqView *result;
  PUSH_KEEP_REFS

  result = [[[VqView alloc] initRawView: NoColumnView(rows)] autorelease];

  POP_KEEP_REFS
  return result;
}

- (VqView *) openFile: (NSString *) filename {
  VqView *result;
  PUSH_KEEP_REFS

  const char *fn = [[filename stringByExpandingTildeInPath] UTF8String];
  result = [[[VqView alloc] initRawView: OpenDataFile(fn)] autorelease];

  POP_KEEP_REFS
  return result;
}

@end

//-----------------------------------------------------------------------------
