#include <stdio.h>
#include <stdlib.h>

#include <dirwrap.h>
#include <uifc.h>

#include "bbslist.h"
#include "uifcinit.h"

enum {
	 USER_BBSLIST
	,SYSTEM_BBSLIST
};

struct bbslist_file {
	char			name[LIST_NAME_MAX+1];
	char			addr[LIST_ADDR_MAX+1];
	short unsigned int port;
	time_t			added;
	time_t			connected;
	unsigned int	calls;
	char			user[MAX_USER_LEN+1];
	char			password[MAX_PASSWD_LEN+1];
};

void sort_list(struct bbslist **list)  {
	struct bbslist *tmp;
	unsigned int	i,swapped=1;

	while(swapped) {
		swapped=0;
		for(i=1;list[i-1]->name[0] && list[i]->name[0];i++) {
			int	cmp=stricmp(list[i-1]->name,list[i]->name);
			if(cmp>0) {
				tmp=list[i];
				list[i]=list[i-1];
				list[i-1]=tmp;
				swapped=1;
			}
		}
	}
}

void write_list(struct bbslist **list)
{
	char	*home;
	char	listpath[MAX_PATH+1];
	FILE	*listfile;
	int		i;
	struct bbslist_file bbs;
	char	str[MAX_PATH+1];

	home=getenv("HOME");
	if(home==NULL)
		getcwd(listpath,sizeof(listpath));
	else
		strcpy(listpath,home);
	strncat(listpath,"/",sizeof(listpath));
	strncat(listpath,"syncterm.lst",sizeof(listpath));
	if(strlen(listpath)>MAX_PATH) {
		fprintf(stderr,"Path to syncterm.lst too long");
		return;
	}
	if((listfile=fopen(listpath,"wb"))!=NULL) {
		for(i=0;list[i]->name[0];i++) {
			strcpy(bbs.name,list[i]->name);
			strcpy(bbs.addr,list[i]->addr);
			bbs.port=list[i]->port;
			bbs.added=list[i]->added;
			bbs.connected=list[i]->connected;
			bbs.calls=list[i]->calls;
			strcpy(bbs.user,list[i]->user);
			strcpy(bbs.password,list[i]->password);
			fwrite(&bbs,sizeof(bbs),1,listfile);
		}
		fclose(listfile);
	}
	else {
		sprintf(str,"Can't save list to %.*s",MAX_PATH-20,listpath);
		uifc.msg(str);
	}
}

/*
 * Reads in a BBS list from listpath using *i as the counter into bbslist
 * first BBS read goes into list[i]
 */
void read_list(char *listpath, struct bbslist **list, int *i, int type)
{
	FILE	*listfile;
	struct	bbslist_file	bbs;
	if((listfile=fopen(listpath,"r"))!=NULL) {
		while(*i<MAX_OPTS && fread(&bbs,sizeof(bbs),1,listfile)) {
			if((list[*i]=(struct bbslist *)malloc(sizeof(struct bbslist)))==NULL)
				break;
			strcpy(list[*i]->name,bbs.name);
			strcpy(list[*i]->addr,bbs.addr);
			list[*i]->port=bbs.port;
			list[*i]->added=bbs.added;
			list[*i]->connected=bbs.connected;
			list[*i]->calls=bbs.calls;
			strcpy(list[*i]->user,bbs.user);
			strcpy(list[*i]->password,bbs.password);
			list[*i]->type=type;
			list[*i]->id=(*i)++;
		}
		fclose(listfile);
	}

	/* Add terminator */
	list[*i]=(struct bbslist *)"";
}

int edit_list(struct bbslist *item)
{
	char	opt[6][80];
	char	*opts[6];
	int		changed=0;
	int		copt=0,i,j;
	char	str[6];

	for(i=0;i<6)
		opts[i]=opt[i];
	if(item->type==SYSTEM_BBSLIST) {
		uifc.msg("Cannot edit system BBS list");
		return(0);
	}
	opt[5][0]=0;
	for(;;) {
		sprintf(opt[0],"BBS Name:       %s",item->name);
		sprintf(opt[1],"RLogin Address: %s",item->addr);
		sprintf(opt[2],"RLogin Port:    %hu",item->port);
		sprintf(opt[3],"Username:       %s",item->user);
		sprintf(opt[4],"Password");
		uifc.changes=0;
		switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&copt,NULL,"Edit Entry",opts)) {
			case -1:
				return(changed);
			case 0:
				uifc.input(WIN_MID|WIN_SAV,0,0,"BBS Name",item->name,LIST_NAME_MAX,K_EDIT);
				break;
			case 1:
				uifc.input(WIN_MID|WIN_SAV,0,0,"RLogin Address",item->addr,LIST_ADDR_MAX,K_EDIT);
				break;
			case 2:
				i=item->port;
				sprintf(str,"%hu",item->port?item->port:513);
				uifc.input(WIN_MID|WIN_SAV,0,0,"RLogin Port",str,5,K_EDIT|K_NUMBER);
				j=atoi(str);
				if(j<1 || j>65535)
					j=513;
				item->port=j;
				if(i!=j)
					uifc.changes=1;
				else
					uifc.changes=0;
				break;
			case 3:
				uifc.input(WIN_MID|WIN_SAV,0,0,"Username",item->user,MAX_USER_LEN,K_EDIT);
				break;
			case 4:
				uifc.input(WIN_MID|WIN_SAV,0,0,"Password",item->password,MAX_PASSWD_LEN,K_EDIT);
				break;
		}
		if(uifc.changes)
			changed=1;
	}
}

/*
 * Displays the BBS list and allows edits to user BBS list
 * Mode is one of BBSLIST_SELECT or BBSLIST_EDIT
 */
struct bbslist *show_bbslist(int mode)
{
	char	*home;
	char	listpath[MAX_PATH+1];
	struct	bbslist	*list[MAX_OPTS+1];
	int		i,j;
	int		opt=0,bar=0;
	static struct bbslist retlist;
	int		val;
	int		listcount=0;
	char	str[6];

	if(init_uifc())
		return(NULL);

	/* User BBS list */
	home=getenv("HOME");
	if(home==NULL)
		getcwd(listpath,sizeof(listpath));
	else
		strcpy(listpath,home);
	strncat(listpath,"/",sizeof(listpath));
	strncat(listpath,"syncterm.lst",sizeof(listpath));
	if(strlen(listpath)>MAX_PATH) {
		fprintf(stderr,"Path to syncterm.lst too long");
		return(NULL);
	}
	read_list(listpath, &list[0], &listcount, USER_BBSLIST);

	/* System BBS List */
#ifdef PREFIX
	strcpy(listpath,PREFIX"/etc/syncterm.lst");

	read_list(listpath, list, &listcount, SYSTEM_BBSLIST);
#endif
	sort_list(list);

	for(;;) {
		val=uifc.list((listcount<MAX_OPTS?WIN_XTR:0)|WIN_SAV|WIN_MID|WIN_INS|WIN_DEL|WIN_EXTKEYS,0,0,0,&opt,&bar,mode==BBSLIST_SELECT?"Select BBS":"Edit BBS",(char **)list);
		if(val==listcount)
			val=listcount|MSK_INS;
		if(val<0) {
			switch(val) {
				case -7:		/* CTRL-E */
					mode=BBSLIST_EDIT;
					break;
				case -6:		/* CTRL-D */
					mode=BBSLIST_SELECT;
					break;
				case -1:		/* ESC */
					return(NULL);
			}
		}
		else if(val&MSK_ON) {
			switch(val&MSK_ON) {
				case MSK_INS:
					if(listcount>=MAX_OPTS) {
						uifc.msg("Max List size reached!");
						break;
					}
					listcount++;
					list[listcount]=list[listcount-1];
					list[listcount-1]=(struct bbslist *)malloc(sizeof(struct bbslist));
					memset(list[listcount-1],0,sizeof(struct bbslist));
					list[listcount-1]->id=listcount-1;
					uifc.changes=0;
					uifc.input(WIN_MID|WIN_SAV,0,0,"BBS Name",list[listcount-1]->name,LIST_NAME_MAX,K_EDIT);
					if(uifc.changes) {
						uifc.changes=0;
						uifc.input(WIN_MID|WIN_SAV,0,0,"RLogin Address",list[listcount-1]->addr,LIST_ADDR_MAX,K_EDIT);
					}
					if(!uifc.changes) {
						free(list[listcount-1]);
						list[listcount-1]=list[listcount];
						listcount--;
					}
					else {
						while(!list[listcount-1]->port) {
							list[listcount-1]->port=513;
							sprintf(str,"%hu",list[listcount-1]->port);
							uifc.input(WIN_MID|WIN_SAV,0,0,"RLogin Port",str,5,K_EDIT|K_NUMBER);
							j=atoi(str);
							if(j<1 || j>65535)
								j=0;
							list[listcount-1]->port=j;
						}
						uifc.input(WIN_MID|WIN_SAV,0,0,"User Name",list[listcount-1]->user,MAX_USER_LEN,K_EDIT);
						uifc.input(WIN_MID|WIN_SAV,0,0,"Password",list[listcount-1]->password,MAX_PASSWD_LEN,K_EDIT);
						sort_list(list);
						for(j=0;list[j]->name[0];j++) {
							if(list[j]->id==listcount-1)
								opt=j;
						}
						write_list(list);
					}
					break;
				case MSK_DEL:
					if(!list[opt]->name[0]) {
						uifc.msg("It's gone, calm down man!");
						break;
					}
					free(list[opt]);
					for(i=opt;list[i]->name[0];i++) {
						list[i]=list[i+1];
					}
					for(i=0;list[i]->name[0];i++) {
						list[i]->id=i;
					}
					write_list(list);
					break;
			}
		}
		else {
			if(mode==BBSLIST_EDIT) {
				i=list[opt]->id;
				if(edit_list(list[opt])) {
					sort_list(list);
					write_list(list);
					for(j=0;list[j]->name[0];j++) {
						if(list[j]->id==i)
							opt=j;
					}
				}
			}
			else {
				memcpy(&retlist,list[val],sizeof(struct bbslist));
				return(&retlist);
			}
		}
	}
}
