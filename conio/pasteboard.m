#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include <stddef.h>
#include <stdio.h>

void OSX_copytext(const char *text)
{
	NSString *cp = [NSString stringWithCString:text encoding:CFStringConvertEncodingToNSStringEncoding(CFStringConvertWindowsCodepageToEncoding(437))];
	if (cp != nil) {
		NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
		[pasteboard clearContents];
		NSArray *copiedObjects = [NSArray arrayWithObject:cp];
		[pasteboard writeObjects:copiedObjects];
	}
}

char *OSX_getcliptext(void)
{
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSArray *classArray = [NSArray arrayWithObject:[NSString class]];
    NSDictionary *options = [NSDictionary dictionary];
	BOOL ok = [pasteboard canReadObjectForClasses:classArray options:options];
	if (ok) {
		NSArray *objectsToPaste = [pasteboard readObjectsForClasses:classArray options:options];
		NSString *ct = [objectsToPaste objectAtIndex:0];
		if (ct != nil) {
			const char *ptr = [ct cStringUsingEncoding:CFStringConvertEncodingToNSStringEncoding(CFStringConvertWindowsCodepageToEncoding(437))];
			if (ptr)
				return strdup(ptr);
		}
	}
	return NULL;
}
