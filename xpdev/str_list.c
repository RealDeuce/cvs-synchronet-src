/* str_list.c */

/* Functions to deal with NULL-terminated string lists */

/* $Id: str_list.c,v 1.46 2016/01/29 20:09:04 deuce Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html			*
 *																			*
 * This library is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU Lesser General Public License		*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU Lesser General Public License for more details: lgpl.txt or	*
 * http://www.fsf.org/copyleft/lesser.html									*
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

#include <stdlib.h>		/* malloc and qsort */
#include <string.h>		/* strtok */
#if defined(_WIN32)
 #include <malloc.h>    /* alloca() on Win32 */
#endif
#include "genwrap.h"	/* stricmp */
#include "str_list.h"

str_list_t DLLCALL strListInit(void)
{
	str_list_t list;

	if((list=(str_list_t)malloc(sizeof(char*)))==NULL)
		return(NULL);

	list[0]=NULL;	/* terminated by default */
	return(list);
}

size_t DLLCALL strListCount(const str_list_t list)
{
	size_t i;

	COUNT_LIST_ITEMS(list,i);

	return(i);
}

int DLLCALL strListIndexOf(const str_list_t list, const char* str)
{
	size_t		i;

	if(list==NULL)
		return -1;

	for(i=0; list[i]!=NULL; i++) {
		if(list[i]==str)
			return i;
	}
	
	return -1;
}

int DLLCALL strListFind(const str_list_t list, const char* str, BOOL case_sensistive)
{
	size_t		i;

	if(list==NULL)
		return -1;

	for(i=0; list[i]!=NULL; i++) {
		if(case_sensistive) {
			if(strcmp(list[i],str) == 0)
				return i;
		} else {
			if(stricmp(list[i],str) == 0)
				return i;
		}
	}
	
	return -1;
}

static char* str_list_append(str_list_t* list, char* str, size_t index)
{
	str_list_t lp;

	if((lp=(str_list_t)realloc(*list,sizeof(char*)*(index+2)))==NULL)
		return(NULL);

	*list=lp;
	lp[index++]=str;
	lp[index]=NULL;	/* terminate list */

	return(str);
}

static char* str_list_insert(str_list_t* list, char* str, size_t index)
{
	size_t	i;
	size_t	count;
	str_list_t lp;

	if(*list == NULL)
		*list = strListInit();
	count = strListCount(*list);
	if(index > count)	/* invalid index, do nothing */
		return(NULL);

	count++;
	if((lp=(str_list_t)realloc(*list,sizeof(char*)*(count+1)))==NULL)
		return(NULL);

	*list=lp;
	for(i=count; i>index; i--)
		lp[i]=lp[i-1];
	lp[index]=str;

	return(str);
}

char* DLLCALL strListRemove(str_list_t* list, size_t index)
{
	char*	str;
	size_t	i;
	size_t	count;
	str_list_t lp;

	count = strListCount(*list);

	if(index==STR_LIST_LAST_INDEX && count)
		index = count-1;

	if(index >= count)	/* invalid index, do nothing */
		return(NULL);

	count--;
	if((lp=(str_list_t)realloc(*list,sizeof(char*)*(count+1)))==NULL)
		return(NULL);

	*list=lp;
	str=lp[index];
	for(i=index; i<count; i++)
		lp[i]=lp[i+1];
	lp[count]=NULL;

	return(str);
}

BOOL DLLCALL strListDelete(str_list_t* list, size_t index)
{
	char*	str;

	if((str=strListRemove(list, index))==NULL)
		return(FALSE);

	free(str);

	return(TRUE);
}

char* DLLCALL strListReplace(const str_list_t list, size_t index, const char* str)
{
	char*	buf;
	size_t	count;

	if(str==NULL)
		return(NULL);

	count = strListCount(list);

	if(index==STR_LIST_LAST_INDEX && count)
		index = count-1;

	if(index >= count)	/* invalid index, do nothing */
		return(NULL);

	if((buf=(char*)realloc(list[index],strlen(str)+1))==NULL)
		return(NULL);

	list[index]=buf;
	strcpy(buf,str);

	return(buf);
}

BOOL DLLCALL strListSwap(const str_list_t list, size_t index1, size_t index2)
{
	char*	tmp;
	size_t	count;

	count = strListCount(list);

	if(index1==STR_LIST_LAST_INDEX && count)
		index1 = count-1;

	if(index2==STR_LIST_LAST_INDEX && count)
		index2 = count-1;

	if(index1 >= count || index2 >= count || index1 == index2)	
		return(FALSE);	/* invalid index, do nothing */

	tmp=list[index1];
	list[index1]=list[index2];
	list[index2]=tmp;

	return(TRUE);
}

char* DLLCALL strListAppend(str_list_t* list, const char* str, size_t index)
{
	char* buf;

	if(str==NULL)
		return(NULL);

	if((buf=strdup(str))==NULL)
		return(NULL);

	if(index==STR_LIST_LAST_INDEX)
		index=strListCount(*list);

	return(str_list_append(list,buf,index));
}

size_t DLLCALL	strListAppendList(str_list_t* list, const str_list_t add_list)
{
	size_t	i;
	size_t	count;

	count=strListCount(*list);
	for(i=0; add_list[i]!=NULL; i++)
		strListAppend(list,add_list[i],count++);

	return(count);
}

char* DLLCALL strListInsert(str_list_t* list, const char* str, size_t index)
{
	char* buf;

	if(str==NULL)
		return(NULL);

	if((buf=strdup(str))==NULL)
		return(NULL);

	return(str_list_insert(list,buf,index));
}

size_t DLLCALL strListInsertList(str_list_t* list, const str_list_t add_list, size_t index)
{
	size_t	i;

	for(i=0; add_list[i]!=NULL; i++)
		if(strListInsert(list,add_list[i],index++)==NULL)
			break;

	return(i);
}

str_list_t DLLCALL strListSplit(str_list_t* lp, char* str, const char* delimit)
{
	size_t	count;
	char*	token;
	char*	tmp;
	str_list_t	list;

	if(str==NULL || delimit==NULL)
		return(NULL);

	if(lp==NULL) {
		if((list = strListInit())==NULL)
			return(NULL);
		lp=&list;
		count=0;
	} else
		count=strListCount(*lp);

	for(token = strtok_r(str, delimit, &tmp); token!=NULL; token=strtok_r(NULL, delimit, &tmp))
		if(strListAppend(lp, token, count++)==NULL)
			break;

	return(*lp);
}

str_list_t DLLCALL strListSplitCopy(str_list_t* list, const char* str, const char* delimit)
{
	char*		buf;
	str_list_t	new_list;

	if(str==NULL || delimit==NULL)
		return(NULL);

	if((buf=strdup(str))==NULL)
		return(NULL);

	new_list=strListSplit(list,buf,delimit);

	free(buf);

	if(list!=NULL)
		*list = new_list;

	return(new_list);
}

size_t DLLCALL strListMerge(str_list_t* list, str_list_t add_list)
{
	size_t	i;
	size_t	count;

	count=strListCount(*list);
	for(i=0; add_list[i]!=NULL; i++)
		str_list_append(list,add_list[i],count++);

	return(i);
}

char* DLLCALL strListCombine(str_list_t list, char* buf, size_t maxlen, const char* delimit)
{
	size_t	i;
	char*	end;
	char*	ptr;

	if(maxlen<1)
		return(NULL);

	if(buf==NULL)
		if((buf=(char*)malloc(maxlen))==NULL)
			return(NULL);

	memset(buf, 0, maxlen);
	if(list==NULL)
		return buf;

	end=buf+maxlen;
	for(i=0, ptr=buf; list[i]!=NULL && buf<end; i++)
		ptr += safe_snprintf(ptr, end-ptr, "%s%s", i ? delimit:"", list[i]);

	return(buf);
}

#if defined(_WIN32)
	#define QSORT_CALLBACK_TYPE	__cdecl
#else
	#define QSORT_CALLBACK_TYPE
#endif

static int QSORT_CALLBACK_TYPE strListCompareAlpha(const void *arg1, const void *arg2)
{
   return stricmp(*(char**)arg1, *(char**)arg2);
}

static int QSORT_CALLBACK_TYPE strListCompareAlphaReverse(const void *arg1, const void *arg2)
{
   return stricmp(*(char**)arg2, *(char**)arg1);
}

static int QSORT_CALLBACK_TYPE strListCompareAlphaCase(const void *arg1, const void *arg2)
{
   return strcmp(*(char**)arg1, *(char**)arg2);
}

static int QSORT_CALLBACK_TYPE strListCompareAlphaCaseReverse(const void *arg1, const void *arg2)
{
   return strcmp(*(char**)arg2, *(char**)arg1);
}

void DLLCALL strListSortAlpha(str_list_t list)
{
	qsort(list,strListCount(list),sizeof(char*),strListCompareAlpha);
}

void DLLCALL strListSortAlphaReverse(str_list_t list)
{
	qsort(list,strListCount(list),sizeof(char*),strListCompareAlphaReverse);
}

void DLLCALL strListSortAlphaCase(str_list_t list)
{
	qsort(list,strListCount(list),sizeof(char*),strListCompareAlphaCase);
}

void DLLCALL strListSortAlphaCaseReverse(str_list_t list)
{
	qsort(list,strListCount(list),sizeof(char*),strListCompareAlphaCaseReverse);
}

str_list_t DLLCALL strListDup(str_list_t list)
{
	str_list_t	ret;
	size_t		count=0;

	ret = strListInit();
	for(; *list; list++)
		strListAppend(&ret, *list, count++);
	return ret;
}

int DLLCALL strListCmp(str_list_t list1, str_list_t list2)
{
	str_list_t	l1=strListDup(list1);
	str_list_t	l2=strListDup(list2);
	str_list_t	ol1=l1;
	str_list_t	ol2=l2;
	int			tmp;
	int			ret;

	if(*l1 == NULL && *l2 == NULL) {
		ret=0;
		goto early_return;
	}
	if(*l1 == NULL) {
		ret = -1;
		goto early_return;
	}
	if(*l2 == NULL) {
		ret = 1;
		goto early_return;
	}

	strListSortAlphaCase(l1);
	strListSortAlphaCase(l2);

	for(; *l1; l1++) {
		l2++;
		if(*l2==NULL) {
			ret=1;
			goto early_return;
		}
		tmp = strcmp(*l1, *l2);
		if(tmp != 0) {
			ret=tmp;
			goto early_return;
		}
	}
	l2++;
	if(*l2==NULL)
		ret=0;
	else
		ret=-1;

early_return:
	strListFree(&ol1);
	strListFree(&ol2);
	return ret;
}

void DLLCALL strListFreeStrings(str_list_t list)
{
	size_t i;

	FREE_LIST_ITEMS(list,i);
}

void DLLCALL strListFree(str_list_t* list)
{
	if(*list!=NULL) {
		strListFreeStrings(*list);
		FREE_AND_NULL(*list);
	}
}

static str_list_t str_list_read_file(FILE* fp, str_list_t* lp, size_t max_line_len)
{
	char*		buf=NULL;
	size_t		count;
	str_list_t	list;

	if(max_line_len<1)
		max_line_len=2048;

	if(lp==NULL) {
		if((list = strListInit())==NULL)
			return(NULL);
		lp=&list;
	}

	if(fp!=NULL) {
		count=strListCount(*lp);
		while(!feof(fp)) {
			if(buf==NULL && (buf=(char*)malloc(max_line_len+1))==NULL)
				return(NULL);
			
			if(fgets(buf,max_line_len+1,fp)==NULL)
				break;
			strListAppend(lp, buf, count++);
		}
	}
	if(buf)
		free(buf);

	return(*lp);
}

size_t DLLCALL strListInsertFile(FILE* fp, str_list_t* lp, size_t index, size_t max_line_len)
{
	str_list_t	list;
	size_t		count;

	if((list=str_list_read_file(fp, NULL, max_line_len)) == NULL)
		return(0);

	count = strListInsertList(lp, list, index);

	strListFree(&list);

	return(count);
}

str_list_t DLLCALL strListReadFile(FILE* fp, str_list_t* lp, size_t max_line_len)
{
	return str_list_read_file(fp,lp,max_line_len);
}

size_t DLLCALL strListWriteFile(FILE* fp, const str_list_t list, const char* separator)
{
	size_t		i;

	if(list==NULL)
		return(0);

	for(i=0; list[i]!=NULL; i++) {
		if(fputs(list[i],fp)==EOF)
			break;
		if(separator!=NULL && fputs(separator,fp)==EOF)
			break;
	}
	
	return(i);
}

size_t DLLCALL strListBlockLength(char* block)
{
	char*	p=block;
	size_t	str_len;
	size_t	block_len=0;

	if(block==NULL)
		return(0);

	/* calculate total block length */
	while((str_len=strlen(p))!=0) {
		block_len+=(str_len + 1);
		p+=(str_len + 1);
	}
	/* block must be double-NULL terminated */
	if(!block_len)
		block_len=1;
	block_len++;

	return(block_len);
}

char* DLLCALL strListCopyBlock(char* block)
{
	char*	p;
	size_t	block_len;
	
	if((block_len=strListBlockLength(block))==0)
		return(NULL);

	if((p=(char*)malloc(block_len))==NULL)
		return(NULL);
	memcpy(p, block, block_len);
	return(p);
}

char* DLLCALL strListAppendBlock(char* block, str_list_t list)
{
	char*	p;
	size_t	str_len;
	size_t	block_len;	
	size_t	i;

	if((block_len=strListBlockLength(block))!=0)
		block_len--;	/* Over-write existing NULL terminator */

	for(i=0; list[i]!=NULL; i++) {
		str_len=strlen(list[i]);
		if(str_len==0)
			continue;	/* can't include empty strings in block */
		if((p=(char*)realloc(block, block_len + str_len + 1))==NULL) {
			FREE_AND_NULL(block);
			return(block);
		}
		block=p;
		strcpy(block + block_len, list[i]);
		block_len += (str_len + 1);
	}

	/* block must be double-NULL terminated */
	if(!block_len)
		block_len=1;
	block_len++;
	if((p=(char*)realloc(block, block_len))==NULL) {
		FREE_AND_NULL(block);
		return(block);
	}
	block=p;
	memset(block + (block_len-2), 0, 2);

	return(block);
}

char* DLLCALL strListCreateBlock(str_list_t list)
{
	return(strListAppendBlock(NULL,list));
}

void DLLCALL strListFreeBlock(char* block)
{
	if(block!=NULL)
		free(block);	/* this must be done here for Windows-DLL reasons */
}
