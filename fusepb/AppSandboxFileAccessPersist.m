//
//  AppSandboxFileAccessPersist.m
//  AppSandboxFileAccess
//
//  Created by Leigh McCulloch on 23/11/2013.
//
//  Copyright (c) 2013, Leigh McCulloch
//  All rights reserved.
//
//  BSD-2-Clause License: http://opensource.org/licenses/BSD-2-Clause
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
//  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#import "AppSandboxFileAccessPersist.h"

static NSURL *AppSandboxCanonicalFileURL( NSURL *url )
{
	if( !url || ![url isFileURL] ) return url;
	return [[url URLByStandardizingPath] URLByResolvingSymlinksInPath];
}

static NSData *AppSandboxBookmarkForURLChain( NSUserDefaults *defaults, NSURL *url,
	BOOL canonicalize )
{
	NSURL *subURL = canonicalize ? AppSandboxCanonicalFileURL( url ) : url;

	while( [subURL path].length > 1 ) {
		NSString *urlStr = [subURL absoluteString];
		NSString *key = [NSString stringWithFormat:@"bd_%@", urlStr];
		NSData *bookmark = [defaults dataForKey:key];
		if( bookmark ) return bookmark;
		subURL = [subURL URLByDeletingLastPathComponent];
	}

	return nil;
}

@implementation AppSandboxFileAccessPersist

+ (NSString *)keyForBookmarkDataForURL:(NSURL *)url {
	NSURL *canonicalURL = AppSandboxCanonicalFileURL( url );
	NSString *urlStr = [canonicalURL absoluteString];
	return [NSString stringWithFormat:@"bd_%1$@", urlStr];
}

- (NSData *)bookmarkDataForURL:(NSURL *)url {
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSData *bookmark = nil;
	NSURL *standardizedURL = [url isFileURL] ? [url URLByStandardizingPath] : url;
	NSURL *canonicalURL = AppSandboxCanonicalFileURL( url );

	bookmark = AppSandboxBookmarkForURLChain( defaults, canonicalURL, NO );
	if( bookmark ) return bookmark;

	bookmark = AppSandboxBookmarkForURLChain( defaults, standardizedURL, NO );
	if( bookmark ) return bookmark;

	bookmark = AppSandboxBookmarkForURLChain( defaults, url, NO );
	if( bookmark ) return bookmark;

	return nil;
}

- (void)setBookmarkData:(NSData *)data forURL:(NSURL *)url {
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *key = [AppSandboxFileAccessPersist keyForBookmarkDataForURL:url];
	[defaults setObject:data forKey:key];
}

- (void)clearBookmarkDataForURL:(NSURL *)url {
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *key = [AppSandboxFileAccessPersist keyForBookmarkDataForURL:url];
	[defaults removeObjectForKey:key];
}

@end
