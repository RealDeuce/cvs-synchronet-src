/* $Id: wordwrap.c,v 1.18 2015/02/10 08:34:27 deuce Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2011 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This program is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU General Public License				*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU General Public License for more details: gpl.txt or			*
 * http://www.fsf.org/copyleft/gpl.html										*
 *																			*
 * Anonymous FTP access to the most recent released source is available at	*
 * ftp://vert.synchro.net, ftp://cvs.synchro.net and ftp://ftp.synchro.net	*
 *																			*
 * Anonymous CVS access to the development source and modification history	*
 * is available at cvs.synchro.net:/cvsroot/sbbs, example:					*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs login			*
 *     (just hit return, no password is necessary)							*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs checkout src		*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * You are encouraged to submit any modifications (preferably in Unix diff	*
 * format) via e-mail to mods@synchro.net									*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

#include <ctype.h>
#include <genwrap.h>
#include <stdlib.h>		/* realloc */
#include "wordwrap.h"

static int get_prefix(const char *text, int *bytes, int *len, int maxlen)
{
	int		tmp_prefix_bytes,tmp_prefix_len;
	int		expect;
	int		depth;

	*bytes=0;
	*len=0;
	tmp_prefix_bytes=0;
	tmp_prefix_len=0;
	depth=0;
	expect=1;
	if(text[0]!=' ')
		expect=2;
	while(expect) {
		tmp_prefix_bytes++;
		/* Skip CTRL-A codes */
		while(text[tmp_prefix_bytes-1]=='\x01') {
			tmp_prefix_bytes++;
			if(text[tmp_prefix_bytes-1]=='\x01')
				break;
			tmp_prefix_bytes++;
		}
		tmp_prefix_len++;
		if(text[tmp_prefix_bytes-1]==0 || text[tmp_prefix_bytes-1]=='\n' || text[tmp_prefix_bytes-1]=='\r')
			break;
		switch(expect) {
			case 1:		/* At start of possible quote (Next char should be space) */
				if(text[tmp_prefix_bytes-1]!=' ')
					expect=0;
				else
					expect++;
				break;
			case 2:		/* At start of nick (next char should be alphanum or '>') */
			case 3:		/* At second nick initial (next char should be alphanum or '>') */
			case 4:		/* At third nick initial (next char should be alphanum or '>') */
				if(text[tmp_prefix_bytes-1]==' ' || text[tmp_prefix_bytes-1]==0)
					expect=0;
				else
					if(text[tmp_prefix_bytes-1]=='>')
						expect=6;
					else
						expect++;
				break;
			case 5:		/* After three regular chars, next HAS to be a '>') */
				if(text[tmp_prefix_bytes-1]!='>')
					expect=0;
				else
					expect++;
				break;
			case 6:		/* At '>' next char must be a space */
				if(text[tmp_prefix_bytes-1]!=' ')
					expect=0;
				else {
					expect=1;
					*len=tmp_prefix_len;
					*bytes=tmp_prefix_bytes;
					depth++;
					/* Some editors don't put double spaces in between */
					if(text[tmp_prefix_bytes]!=' ')
						expect++;
				}
				break;
			default:
				expect=0;
				break;
		}
	}
	if(*bytes >= maxlen) {
//		lprintf(LOG_CRIT, "Prefix bytes %u is larger than buffer (%u) here: %*.*s",*bytes,maxlen,maxlen,maxlen,text);
		*bytes=maxlen-1;
	}
	return(depth);
}

static void outbuf_append(char **outbuf, char **outp, char *append, int len, int *outlen)
{
	char	*p;

	/* Terminate outbuf */
	**outp=0;
	/* Check if there's room */
	if(*outp - *outbuf + len < *outlen) {
		memcpy(*outp, append, len);
		*outp+=len;
		return;
	}
	/* Not enough room, double the size. */
	*outlen *= 2;
	p=realloc(*outbuf, *outlen);
	if(p==NULL) {
		/* Can't do it. */
		*outlen/=2;
		return;
	}
	/* Set outp for new buffer */
	*outp=p+(*outp - *outbuf);
	*outbuf=p;
	memcpy(*outp, append, len);
	*outp+=len;
	return;
}

static int compare_prefix(char *old_prefix, int old_prefix_bytes, const char *new_prefix, int new_prefix_bytes)
{
	int i;

	if(new_prefix_bytes != old_prefix_bytes) {
		if(new_prefix_bytes < old_prefix_bytes) {
			if(memcmp(old_prefix, new_prefix, new_prefix_bytes)!=0)
				return(-1);
			for(i=new_prefix_bytes; i<old_prefix_bytes; i++) {
				if(!isspace((unsigned char)old_prefix[i]))
					return(-1);
			}
		}
		else {
			if(memcmp(old_prefix, new_prefix, old_prefix_bytes)!=0)
				return(-1);
			for(i=old_prefix_bytes; i<new_prefix_bytes; i++) {
				if(!isspace((unsigned char)new_prefix[i]))
					return(-1);
			}
		}
		return(0);
	}
	if(memcmp(old_prefix,new_prefix,new_prefix_bytes)!=0)
		return(-1);

	return(0);
}

int get_word_len(char *buf, int starting_pos)
{
	int	next_len=0;
	int	pos;

	for(pos=0; ; pos++) {
		if (starting_pos + next_len < 0)
			return 0;
		if (!buf[starting_pos+pos])
			break;
		else if (isspace(buf[starting_pos+pos]))
			break;
		else if (buf[starting_pos+pos]=='\x1f')
			continue;
		else if (buf[starting_pos+pos]=='\x01') {
			pos++;
			if(buf[starting_pos+pos]!='\x01')
				continue;
		}
		else if (buf[starting_pos+pos]=='\b') {
			// This doesn't handle BS the same way... bit it's kinda BS anyway.
			next_len--;
			continue;
		}
		next_len++;
	}

	return next_len;
}

char* wordwrap(char* inbuf, int len, int oldlen, BOOL handle_quotes)
{
	int			l;
	int			crcount=0;
	long		i,k,t;
	int			ocol=1;
	int			icol=1;
	char*		outbuf;
	char*		outp;
	char*		linebuf;
	char*		prefix=NULL;
	int			prefix_len=0;
	int			prefix_bytes=0;
	int			quote_count=0;
	int			old_prefix_bytes=0;
	int			outbuf_size=0;
	int			inbuf_len=strlen(inbuf);
	unsigned	next_len;

	outbuf_size=inbuf_len*3+1;
	if((outbuf=(char*)malloc(outbuf_size))==NULL)
		return NULL;
	outp=outbuf;

	if((linebuf=(char*)malloc(inbuf_len+2))==NULL) { /* room for ^A codes */
		free(outbuf);
		return NULL;
	}

	if(handle_quotes) {
		if((prefix=(char *)malloc(inbuf_len+1))==NULL) { /* room for ^A codes */
			free(linebuf);
			free(outbuf);
			return NULL;
		}
		prefix[0]=0;
	}

	outbuf[0]=0;
	/* Get prefix from the first line (ouch) */
	l=0;
	i=0;
	if(handle_quotes && (quote_count=get_prefix(inbuf, &prefix_bytes, &prefix_len, len*2+2))!=0) {
		i+=prefix_bytes;
		if(prefix_len>len/3*2) {
			/* This prefix is insane (more than 2/3rds of the new width) hack it down to size */
			/* Since we're hacking it, we will always end up with a hardcr on this line. */
			/* ToDo: Something prettier would be nice. */
			sprintf(prefix," %d> ",quote_count);
			prefix_len=strlen(prefix);
			prefix_bytes=strlen(prefix);
		}
		else {
			memcpy(prefix,inbuf,prefix_bytes);
			/* Terminate prefix */
			prefix[prefix_bytes]=0;
		}
		memcpy(linebuf,prefix,prefix_bytes);
		l=prefix_bytes;
		ocol=prefix_len+1;
		icol=prefix_len+1;
		old_prefix_bytes=prefix_bytes;
	}
	for(; inbuf[i]; i++) {
		if(oldlen == 0)
			icol=-256;

		if(l>=len*2+2) {
			l-=4;
			linebuf[l]=0;
//			lprintf(LOG_CRIT, "Word wrap line buffer exceeded... munging line %s",linebuf);
		}
		switch(inbuf[i]) {
			case '\r':
				crcount++;
				break;
			case '\x1f':	/* Delete... meaningless... strip. */
				break;
			case '\b':		/* Backspace... handle if possible, but don't go crazy. */
				if(l>0) {
					if(l>1 && linebuf[l-2]=='\x01') {
						if(linebuf[l-1]=='\x01') {
							ocol--;
							icol--;
						}
						l-=2;
					}
					else {
						l--;
						ocol--;
						icol--;
					}
				}
				break;
			case '\t':		/* TAB */
				linebuf[l++]=inbuf[i];
				/* Can't ever wrap on whitespace remember. */
				icol++;
				ocol++;
				while(ocol%8)
					ocol++;
				while(icol%8)
					icol++;
				break;
			case '\x01':	/* CTRL-A */
				linebuf[l++]=inbuf[i++];
				if(inbuf[i]!='\x01') {
					linebuf[l++]=inbuf[i];
					break;
				}
			case '\n':
				if(handle_quotes && (quote_count=get_prefix(inbuf+i+1, &prefix_bytes, &prefix_len, len*2+2))!=0) {
					/* Move the input pointer offset to the last char of the prefix */
					i+=prefix_bytes;
				}
				if(!inbuf[i+1]) {			/* EOF */
					linebuf[l++]='\r';
					linebuf[l++]='\n';
					outbuf_append(&outbuf, &outp, linebuf, l, &outbuf_size);
					l=0;
					ocol=1;
					continue;
				}
				/* If there's a new prefix, it is a hardcr */
				else if(compare_prefix(prefix, old_prefix_bytes, inbuf+i+1-prefix_bytes, prefix_bytes)!=0) {
					if(prefix_len>len/3*2) {
						/* This prefix is insane (more than 2/3rds of the new width) hack it down to size */
						/* Since we're hacking it, we will always end up with a hardcr on this line. */
						/* ToDo: Something prettier would be nice. */
						sprintf(prefix," %d> ",quote_count);
						prefix_len=strlen(prefix);
						prefix_bytes=strlen(prefix);
					}
					else {
						memcpy(prefix,inbuf+i+1-prefix_bytes,prefix_bytes);
						/* Terminate prefix */
						prefix[prefix_bytes]=0;
					}
					linebuf[l++]='\r';
					linebuf[l++]='\n';
					outbuf_append(&outbuf, &outp, linebuf, l, &outbuf_size);
					memcpy(linebuf,prefix,prefix_bytes);
					l=prefix_bytes;
					ocol=prefix_len+1;
					old_prefix_bytes=prefix_bytes;
					icol=prefix_len+1;
					continue;
				}
				else if(isspace((unsigned char)inbuf[i+1])) {	/* Next line starts with whitespace.  This is a "hard" CR. */
					linebuf[l++]='\r';
					linebuf[l++]='\n';
					outbuf_append(&outbuf, &outp, linebuf, l, &outbuf_size);
					l=prefix_bytes;
					ocol=prefix_len+1;
					icol=prefix_len+1;
					continue;
				}
				else {
					if(icol < oldlen) {			/* If this line is overly long, It's impossible for the next word to fit */
						/* k will equal the length of the first word on the next line */
						k = get_word_len(inbuf, i+1);
						if(icol+k+1 < oldlen) {	/* The next word would have fit but isn't here.  Must be a hard CR */
							linebuf[l++]='\r';
							linebuf[l++]='\n';
							outbuf_append(&outbuf, &outp, linebuf, l, &outbuf_size);
							if(prefix)
								memcpy(linebuf,prefix,prefix_bytes);
							l=prefix_bytes;
							ocol=prefix_len+1;
							icol=prefix_len+1;
							continue;
						}
						else {		/* Not a hard CR... add space if needed */
							if(l<1 || !isspace((unsigned char)linebuf[l-1])) {
								linebuf[l++]=' ';
								ocol++;
							}
						}
					}
					else {			/* Not a hard CR... add space if needed */
						if(l<1 || !isspace((unsigned char)linebuf[l-1])) {
							linebuf[l++]=' ';
							ocol++;
						}
					}
				}
				/* Fall-through soft CRs for wrapping! */
			default:
				if (inbuf[i] != '\n') {
					linebuf[l++]=inbuf[i];
					ocol++;
					icol++;
				}
				if(ocol>len && inbuf[i+1] && !isspace(inbuf[i+1])) {		/* Need to wrap here */
					/* Find the start of the last word */
					k=l;									/* Original next char */
					l--;									/* Move back to the last char */
					while((!isspace((unsigned char)linebuf[l])) && l>0)		/* Move back to the last non-space char */
						l--;
					/*
					 * Look ahead and check how long this next "word" is.
					 * If it's longer than len, there's no point in trying
					 * to make it fit, so we can just chop it.
					 */
					next_len=(k-(l==0?0:(l+1))) + get_word_len(inbuf, i+1);
					if(next_len > len) {		/* Won't be able to wrap... may as well chop. */
						l=k;
						while(l>1 && linebuf[l-2]=='\x01' && linebuf[l-1]!='\x01')
							l-=2;
						if(l>0 && linebuf[l-1]=='\x01')
							l--;
						if(l>0)
							l--;
					}
					t=l+1;									/* Store start position of next line */
					/* Move to start of whitespace */
					while(l>0 && isspace((unsigned char)linebuf[l]))
						l--;
					outbuf_append(&outbuf, &outp, linebuf, l+1, &outbuf_size);
					outbuf_append(&outbuf, &outp, "\r\n", 2, &outbuf_size);
					/* Move trailing words to start of buffer. */
					l=prefix_bytes;
					if(k-t>0)							/* k-1 is the last char position.  t is the start of the next line position */
						memmove(linebuf+l, linebuf+t, k-t);
					l+=k-t;
					/* Find new ocol */
					for(ocol=prefix_len+1,t=prefix_bytes; t<l; t++) {
						switch(linebuf[t]) {
							case '\x01':	/* CTRL-A */
								t++;
								if(linebuf[t]!='\x01')
									break;
								/* Fall-through */
							default:
								ocol++;
						}
					}
				}
		}
	}
	/* Trailing bits. */
	if(l) {
		linebuf[l++]='\r';
		linebuf[l++]='\n';
		outbuf_append(&outbuf, &outp, linebuf, l, &outbuf_size);
	}
	*outp=0;
	/* If there were no CRs in the input, strip all CRs */
	if(!crcount) {
		for(inbuf=outbuf; *inbuf; inbuf++) {
			if(*inbuf=='\r')
				memmove(inbuf, inbuf+1, strlen(inbuf));
		}
	}
 	free(linebuf);

	if(prefix)
		free(prefix);

	return outbuf;
}
