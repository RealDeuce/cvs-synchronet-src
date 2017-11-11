/* $Id: scfgmsg.c,v 1.50 2017/11/11 08:13:42 rswindell Exp $ */

/* Configuring Message Options and Message Groups (but not sub-boards) */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html			*
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

#include "scfg.h"
#include "strwrap.h"	/* itoa() */
#include <stdbool.h>

#define CUT_GROUPNUM	USHRT_MAX

char *utos(char *str)
{
	static char out[128];
	int i;

	for(i=0;str[i] && i<sizeof(out)-1;i++)
		if(str[i]=='_')
			out[i]=' ';
		else
			out[i]=str[i];
	out[i]=0;
	return(out);
}

char *stou(char *str)
{
	static char out[128];
	int i;

	for(i=0;str[i];i++)
		if(str[i]==' ')
			out[i]='_';
		else
			out[i]=str[i];
	out[i]=0;
	return(out);
}

void clearptrs(int subnum)
{
	char str[256];
	ushort idx,scancfg;
	int file, i;
	size_t gi;
	long l=0L;
	glob_t g;

    uifc.pop("Clearing Pointers...");
    sprintf(str,"%suser/ptrs/*.ixb",cfg.data_dir);

	glob(str,0,NULL,&g);
   	for(gi=0;gi<g.gl_pathc;gi++) {

        if(flength(g.gl_pathv[gi])>=((long)cfg.sub[subnum]->ptridx+1L)*10L) {
            if((file=nopen(g.gl_pathv[gi],O_WRONLY))==-1) {
                errormsg(WHERE,ERR_OPEN,g.gl_pathv[gi],O_WRONLY);
                bail(1);
            }
            while(filelength(file)<(long)(cfg.sub[subnum]->ptridx)*10) {
                lseek(file,0L,SEEK_END);
                idx=(ushort)(tell(file)/10);
                for(i=0;i<cfg.total_subs;i++)
                    if(cfg.sub[i]->ptridx==idx)
                        break;
                write(file,&l,sizeof(l));
                write(file,&l,sizeof(l));
                scancfg=0xff;
                if(i<cfg.total_subs) {
                    if(!(cfg.sub[i]->misc&SUB_NSDEF))
                        scancfg&=~SUB_CFG_NSCAN;
                    if(!(cfg.sub[i]->misc&SUB_SSDEF))
                        scancfg&=~SUB_CFG_SSCAN; 
				} else	/* Default to scan OFF for unknown sub */
					scancfg&=~(SUB_CFG_NSCAN|SUB_CFG_SSCAN);
                write(file,&scancfg,sizeof(scancfg)); 
			}
            lseek(file,((long)cfg.sub[subnum]->ptridx)*10L,SEEK_SET);
            write(file,&l,sizeof(l));	/* date set to null */
            write(file,&l,sizeof(l));	/* date set to null */
            scancfg=0xff;
            if(!(cfg.sub[subnum]->misc&SUB_NSDEF))
                scancfg&=~SUB_CFG_NSCAN;
            if(!(cfg.sub[subnum]->misc&SUB_SSDEF))
                scancfg&=~SUB_CFG_SSCAN;
            write(file,&scancfg,sizeof(scancfg));
            close(file); 
		}
    }
	globfree(&g);
    uifc.pop(0);
}

static bool new_grp(unsigned new_grpnum)
{
	grp_t* new_group = malloc(sizeof(grp_t));
	if (new_group == NULL) {
		errormsg(WHERE, ERR_ALLOC, "group", sizeof(grp_t));
		return false;
	}
	memset(new_group, 0, sizeof(*new_group));

	grp_t** new_grp_list;
	if ((new_grp_list = (grp_t **)realloc(cfg.grp, sizeof(grp_t *)*(cfg.total_grps + 1))) == NULL) {
		errormsg(WHERE, ERR_ALLOC, "group list", cfg.total_grps + 1);
		return false;
	}
	cfg.grp = new_grp_list;

	for (unsigned u = cfg.total_grps; u > new_grpnum; u--)
		cfg.grp[u] = cfg.grp[u - 1];

	if (new_grpnum != cfg.total_grps) {	/* Inserting group? Renumber (higher) existing groups */
		for (unsigned j = 0; j < cfg.total_subs; j++) {
			if (cfg.sub[j]->grp >= new_grpnum && cfg.sub[j]->grp != CUT_GROUPNUM)
				cfg.sub[j]->grp++;
		}
	}
	cfg.grp[new_grpnum] = new_group;
	cfg.total_grps++;
	return true;
}

long import_msg_areas(enum import_list_type type, FILE* stream, unsigned grpnum
	, int min_confnum, int max_confnum, qhub_t* qhub
	, long* added)
{
	char		str[256];
	sub_t		tmpsub;
	char		tmp_code[128];
	long		ported = 0;
	int			total_qwk_confs = 0;
	int			read_qwk_confs = 0;
	int			qwk_confnum;
	size_t		grpname_len = strlen(cfg.grp[grpnum]->sname);

	if(added != NULL)
		*added = 0;
	if(grpnum >= cfg.total_grps)
		return -1;

	if(type == IMPORT_LIST_TYPE_QWK_CONTROL_DAT) {
		/* Skip/ignore the first 10 lines */
		for(int i = 0; i < 10; i++) {
			if(!fgets(str,sizeof(str),stream))
				break;
		}
		str[0] = 0;
		fgets(str,sizeof(str),stream);
		total_qwk_confs = atoi(str) + 1;
	}

	while(!feof(stream)) {
		if(!fgets(str,sizeof(str),stream)) break;
		truncsp(str);
		if(!str[0])
			continue;
		memset(&tmpsub,0,sizeof(tmpsub));
//		tmpsub.misc = (SUB_FIDO|SUB_NAME|SUB_TOUSER|SUB_QUOTE|SUB_HYPER);
		tmpsub.grp = grpnum;
		if(type == IMPORT_LIST_TYPE_SUBS_TXT) {
			sprintf(tmpsub.lname,"%.*s",LEN_SLNAME,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.sname,"%.*s",LEN_SSNAME,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.qwkname,"%.*s",10,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			SAFECOPY(tmp_code,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.data_dir,"%.*s",LEN_DIR,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.arstr,"%.*s",LEN_ARSTR,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.read_arstr,"%.*s",LEN_ARSTR,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.post_arstr,"%.*s",LEN_ARSTR,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.op_arstr,"%.*s",LEN_ARSTR,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			tmpsub.misc=ahtoul(str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.tagline,"%.*s",80,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.origline,"%.*s",50,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.post_sem,"%.*s",LEN_DIR,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			SAFECOPY(tmpsub.newsgroup,str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			tmpsub.faddr=atofaddr(str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			tmpsub.maxmsgs=atol(str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			tmpsub.maxcrcs=atol(str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			tmpsub.maxage=atoi(str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			tmpsub.ptridx=atoi(str);
			if(!fgets(str,128,stream)) break;
			truncsp(str);
			sprintf(tmpsub.mod_arstr,"%.*s",LEN_ARSTR,str);

			while(!feof(stream)
				&& strcmp(str,"***END-OF-SUB***")) {
				if(!fgets(str,128,stream)) break;
				truncsp(str); 
			} 
		}
		else if(type == IMPORT_LIST_TYPE_QWK_CONTROL_DAT) {
			if(read_qwk_confs >= total_qwk_confs)
				break;
			read_qwk_confs++;
			qwk_confnum = atoi(str);
			if(!fgets(str,128,stream)) break;
			if (qwk_confnum < min_confnum || qwk_confnum > max_confnum)
				continue;
			truncsp(str);
			char* p=str;
			SKIP_WHITESPACE(p);
			if(*p == 0)
				continue;
			if(strlen(p) > grpname_len && strnicmp(p, cfg.grp[grpnum]->sname, grpname_len) == 0)
				p += grpname_len;
			SKIP_WHITESPACE(p);
			SAFECOPY(tmp_code, p);
			SAFECOPY(tmpsub.lname, p);
			SAFECOPY(tmpsub.sname, p);
			SAFECOPY(tmpsub.qwkname, p);
		}
		else {
			char* p=str;
			SKIP_WHITESPACE(p);
			if(!*p || *p==';')
				continue;
			if(type == IMPORT_LIST_TYPE_GENERIC_AREAS_BBS) {		/* AREAS.BBS Generic/.MSG */
				p=str;
				SKIP_WHITESPACE(p);			/* Find path	*/
				FIND_WHITESPACE(p);			/* Skip path	*/
				SKIP_WHITESPACE(p);			/* Find tag		*/
				truncstr(p," \t");			/* Truncate tag */
				SAFECOPY(tmp_code,p);		/* Copy tag to internal code */
				SAFECOPY(tmpsub.lname,utos(p));
				SAFECOPY(tmpsub.sname,tmpsub.lname);
			}
			else if(type == IMPORT_LIST_TYPE_SBBSECHO_AREAS_BBS) { /* AREAS.BBS SBBSecho */
				p=str;
				SKIP_WHITESPACE(p);			/* Find internal code */
				char* tp=p;
				FIND_WHITESPACE(tp);
				*tp=0;						/* Truncate internal code */
				SAFECOPY(tmp_code,p);		/* Copy internal code suffix */
				p=tp+1;
				SKIP_WHITESPACE(p);			/* Find echo tag */
				truncstr(p," \t");			/* Truncate tag */
				SAFECOPY(tmpsub.lname,utos(p));
				SAFECOPY(tmpsub.sname,tmpsub.lname);
			}
			else if(type == IMPORT_LIST_TYPE_BACKBONE_NA) { /* BACKBONE.NA */
				p=str;
				SKIP_WHITESPACE(p);			/* Find echo tag */
				char* tp=p;
				FIND_WHITESPACE(tp);		/* Find end of tag */
				*tp=0;						/* Truncate echo tag */
				SAFECOPY(tmp_code,p);		/* Copy tag to internal code suffix */
				SAFECOPY(tmpsub.sname,utos(p));	/* ... to short name, converting underscores to spaces */
				p=tp+1;
				SKIP_WHITESPACE(p);			/* Find description */
				SAFECOPY(tmpsub.lname,p);	/* Copy description to long name */
			}
		}

		SAFECOPY(tmpsub.code_suffix, prep_code(tmp_code,cfg.grp[grpnum]->code_prefix));
		truncsp(tmpsub.sname);
		truncsp(tmpsub.lname);
		truncsp(tmpsub.qwkname);
		SAFECOPY(tmpsub.qwkname,tmpsub.sname);

		if(tmpsub.code_suffix[0]==0	|| tmpsub.sname[0]==0)
			continue;

		if(tmpsub.lname[0] == 0)
			SAFECOPY(tmpsub.lname, tmpsub.sname);

		uint j;
		int mod_offset = LEN_CODE-1;
		for(j=0; j < cfg.total_subs && mod_offset >= 0; j++) {
			/* same group and duplicate name/echotag or QWK confname? overwrite the sub entry */
			if(cfg.sub[j]->grp == grpnum) {
				if(stricmp(cfg.sub[j]->sname, tmpsub.sname) == 0)
					break;
			} else {
				if((cfg.grp[grpnum]->code_prefix[0] || cfg.grp[cfg.sub[j]->grp]->code_prefix[0]))
					continue;
			}
			if(stricmp(cfg.sub[j]->code_suffix, tmpsub.code_suffix) == 0) {
				if(type == IMPORT_LIST_TYPE_SUBS_TXT)	/* subs.txt import (don't modify internal code) */
					break;
				int code_len = strlen(tmpsub.code_suffix);
				if(code_len < 1)
					break;
				if(code_len < LEN_CODE)
					memset(tmpsub.code_suffix + code_len, '0', LEN_CODE - code_len);
				else if(tmpsub.code_suffix[mod_offset] == 'Z')
					tmpsub.code_suffix[mod_offset] = '0';
				else if(tmpsub.code_suffix[mod_offset] == '9')
					mod_offset--;
				else 
					tmpsub.code_suffix[mod_offset]++;
				j=0;
			}
		}
		if(mod_offset < 0)
			break;

		if(j==cfg.total_subs) {
			if(!new_sub(j, grpnum))
				break;
			if(added != NULL)
				(*added)++;
		}
		if(type == IMPORT_LIST_TYPE_SUBS_TXT) {
			uint16_t sav_ptridx=cfg.sub[j]->ptridx;	/* save original ptridx */
			memcpy(cfg.sub[j],&tmpsub,sizeof(sub_t));
			cfg.sub[j]->ptridx=sav_ptridx;	/* restore original ptridx */
		} else {
			cfg.sub[j]->grp=grpnum;
			if(cfg.total_faddrs)
				cfg.sub[j]->faddr=cfg.faddr[0];
			SAFECOPY(cfg.sub[j]->code_suffix,tmpsub.code_suffix);
			SAFECOPY(cfg.sub[j]->sname,tmpsub.sname);
			SAFECOPY(cfg.sub[j]->lname,tmpsub.lname);
			SAFECOPY(cfg.sub[j]->qwkname,tmpsub.qwkname);
			SAFECOPY(cfg.sub[j]->data_dir,tmpsub.data_dir);
			if(j==cfg.total_subs)
				cfg.sub[j]->maxmsgs=1000;
		}
		if(qhub != NULL)
			new_qhub_sub(qhub, qhub->subs, cfg.sub[j], qwk_confnum);
		ported++;
	}
	if(ported && cfg.grp[grpnum]->sort)
		sort_subs(grpnum);

	return ported;
}

unsigned subs_in_group(unsigned grpnum)
{
	unsigned total = 0;

	for(unsigned u=0; u<cfg.total_subs; u++)
		if(cfg.sub[u]->grp == grpnum)
			total++;

	return total;
}

void msgs_cfg()
{
	static int dflt,msgs_dflt,bar;
	static int import_list_type;
	static int export_list_type;
	char	str[256],str2[256],done=0;
    char	tmp[128];
	int		j,k,q,s;
	int		i,file;
	long	ported;
	static grp_t savgrp;
	FILE*	stream;

	char* grp_short_name_help =
		"`Group Short Name:`\n"
		"\n"
		"This is a short description of the message group which is used for the\n"
		"main menu and reading message prompts.\n"
		;
	char* grp_long_name_help = 
		"`Group Long Name:`\n"
		"\n"
		"This is a description of the message group which is displayed when a\n"
		"user of the system uses the `/*` command from the main menu.\n"
		;
	char* grp_code_prefix_help =
		"`Internal Code Prefix:`\n"
		"\n"
		"This is an `optional`, but highly recommended `code prefix` used to help\n"
		"generate `unique` internal codes for the sub-boards in this message group.\n"
		"When code prefixes are used, sub-board `internal codes` will be\n"
		"constructed from a combination of the prefix and the specified code \n"
		"suffix for each sub-board.\n"
		"\n"
		"Code prefixes may contain up to 8 legal filename characters.\n"
		"\n"
		"Code prefixes should be unique among the message groups on the system.\n"
		"\n"
		"Changing a group's code prefix implicitly changes all the internal codes\n"
		"of the sub-boards within the group, so change this value with caution.\n"
		;

	while(1) {
		for(i=0;i<cfg.total_grps && i<MAX_OPTS;i++)
			sprintf(opt[i],"%-*s %5u", LEN_GLNAME, cfg.grp[i]->lname, subs_in_group(i));
		opt[i][0]=0;
		int mode = WIN_ORG|WIN_ACT|WIN_CHE;
		if(cfg.total_grps)
			mode |= WIN_DEL|WIN_DELACT|WIN_COPY|WIN_CUT;
		if(cfg.total_grps<MAX_OPTS)
			mode |= WIN_INS|WIN_INSACT|WIN_XTR;
		if(savgrp.sname[0])
			mode |= WIN_PASTE | WIN_PASTEXTR;
		uifc.helpbuf=
			"`Message Groups:`\n"
			"\n"
			"This is a listing of message groups for your BBS. Message groups are\n"
			"used to logically separate your message `sub-boards` into groups. Every\n"
			"sub-board belongs to a message group. You must have at least one message\n"
			"group and one sub-board configured.\n"
			"\n"
			"One popular use for message groups is to separate local sub-boards and\n"
			"networked sub-boards. One might have a `Local` message group that contains\n"
			"non-networked sub-boards of various topics and also have a `FidoNet`\n"
			"message group that contains sub-boards that are echoed across FidoNet.\n"
			"Some sysops separate sub-boards into more specific areas such as `Main`,\n"
			"`Technical`, or `Adult`. If you have many sub-boards that have a common\n"
			"subject denominator, you may want to have a separate message group for\n"
			"those sub-boards for a more organized message structure.\n"
		;
		i=uifc.list(mode,0,0,0,&msgs_dflt,&bar,"Message Groups                      Sub-boards", opt);
		if(i==-1) {
			j=save_changes(WIN_MID);
			if(j==-1)
			   continue;
			if(!j) {
				write_msgs_cfg(&cfg,backup_level);
				refresh_cfg(&cfg);
			}
			return;
		}
		int grpnum = i & MSK_OFF;
		int msk = i & MSK_ON;
		if(msk == MSK_INS) {
			char long_name[LEN_GLNAME+1];
			uifc.helpbuf=grp_long_name_help;
			strcpy(long_name,"Main");
			if(uifc.input(WIN_MID|WIN_SAV,0,0, "Group Long Name", long_name, sizeof(long_name)-1, K_EDIT)<1)
				continue;

			char short_name[LEN_GSNAME+1];
			uifc.helpbuf=grp_short_name_help;
			SAFECOPY(short_name, long_name);
			if(uifc.input(WIN_MID|WIN_SAV,0,0, "Group Short Name", short_name, sizeof(short_name)-1 ,K_EDIT)<1)
				continue;

			char code_prefix[LEN_GSNAME+1];	/* purposely extra-long */
			SAFECOPY(code_prefix, short_name);
			prep_code(code_prefix, NULL);
			if(strlen(code_prefix) < LEN_CODE)
				strcat(code_prefix, "_");
			uifc.helpbuf=grp_code_prefix_help;
			if(uifc.input(WIN_MID|WIN_SAV,0,0,"Internal Code Prefix", code_prefix, LEN_CODE, K_EDIT|K_UPPER) < 0)
				continue;
			if (code_prefix[0] != 0 && !code_ok(code_prefix)) {
				uifc.helpbuf=invalid_code;
				uifc.msg("Invalid Code Prefix");
				continue;
			}
			if (!new_grp(grpnum))
				continue;
			SAFECOPY(cfg.grp[grpnum]->lname, long_name);
			SAFECOPY(cfg.grp[grpnum]->sname, short_name);
			SAFECOPY(cfg.grp[grpnum]->code_prefix, code_prefix);
			uifc.changes = TRUE;
			continue; 
		}
		if(msk == MSK_DEL || msk == MSK_CUT) {
			if (msk == MSK_DEL) {
				uifc.helpbuf =
					"`Delete All Data in Group:`\n"
					"\n"
					"If you wish to delete the messages in all the sub-boards in this group,\n"
					"select `Yes`.\n"
					;
				j = 1;
				strcpy(opt[0], "Yes");
				strcpy(opt[1], "No");
				opt[2][0] = 0;
				j = uifc.list(WIN_MID | WIN_SAV, 0, 0, 0, &j, 0, "Delete All Data in Group", opt);
				if (j == -1)
					continue;
				if (j == 0) {
					for (j = 0; j < cfg.total_subs; j++) {
						if (cfg.sub[j]->grp == grpnum) {
							sprintf(str, "%s%s.s*"
								, cfg.grp[cfg.sub[j]->grp]->code_prefix
								, cfg.sub[j]->code_suffix);
							strlwr(str);
							if (!cfg.sub[j]->data_dir[0])
								sprintf(tmp, "%ssubs/", cfg.data_dir);
							else
								strcpy(tmp, cfg.sub[j]->data_dir);
							delfiles(tmp, str);
							clearptrs(j);
						}
					}
				}
			}
			if(msk == MSK_CUT)
				savgrp = *cfg.grp[grpnum];
			free(cfg.grp[grpnum]);
			if (msk == MSK_DEL) {
				for (j = 0; j < cfg.total_subs;) {
					if (cfg.sub[j]->grp == grpnum) {	/* delete subs of this group */
						free(cfg.sub[j]);
						cfg.total_subs--;
						k = j;
						while (k < cfg.total_subs) {	/* move all subs down */
							cfg.sub[k] = cfg.sub[k + 1];
							for (q = 0; q < cfg.total_qhubs; q++)
								for (s = 0; s < cfg.qhub[q]->subs; s++)
									if (cfg.qhub[q]->sub[s] == cfg.sub[j])
										cfg.qhub[q]->sub[s] = NULL;
							k++;
						}
					}
					else j++;
				}
				for (j = 0; j < cfg.total_subs; j++)	/* move sub group numbers down */
					if (cfg.sub[j]->grp > grpnum)
						cfg.sub[j]->grp--;
			}
			else { /* CUT */
				for (j = 0; j < cfg.total_subs; j++) {
					if (cfg.sub[j]->grp > grpnum)
						cfg.sub[j]->grp--;
					else if (cfg.sub[j]->grp == grpnum)
						cfg.sub[j]->grp = CUT_GROUPNUM;
				}
			}
			cfg.total_grps--;
			for (i = grpnum; i < cfg.total_grps; i++)
				cfg.grp[i]=cfg.grp[i+1];
			uifc.changes=1;
			continue; 
		}
		if(msk == MSK_COPY) {
			savgrp=*cfg.grp[grpnum];
			continue; 
		}
		if (msk == MSK_PASTE) {
			if (!new_grp(grpnum))
				continue;
			/* Restore previously cut sub-boards to newly-pasted group */
			for (i = 0; i < cfg.total_subs; i++)
				if (cfg.sub[i]->grp == CUT_GROUPNUM)
					cfg.sub[i]->grp = grpnum;
			*cfg.grp[grpnum] = savgrp;
			uifc.changes = 1;
			continue;
		}
		if (msk != 0)
			continue;
		done=0;
		while(!done) {
			j=0;
			sprintf(opt[j++],"%-27.27s%s","Long Name",cfg.grp[grpnum]->lname);
			sprintf(opt[j++],"%-27.27s%s","Short Name",cfg.grp[grpnum]->sname);
			sprintf(opt[j++],"%-27.27s%s","Internal Code Prefix",cfg.grp[grpnum]->code_prefix);
			sprintf(opt[j++],"%-27.27s%.40s","Access Requirements"
				,cfg.grp[grpnum]->arstr);
			sprintf(opt[j++],"%-27.27s%s","Sort Group by Sub-board", area_sort_desc[cfg.grp[grpnum]->sort]);
			strcpy(opt[j++],"Clone Options");
			strcpy(opt[j++],"Export Areas...");
			strcpy(opt[j++],"Import Areas...");
			strcpy(opt[j++],"Message Sub-boards...");
			opt[j][0]=0;
			sprintf(str,"%s Group",cfg.grp[grpnum]->sname);
			uifc.helpbuf=
				"`Message Group Configuration:`\n"
				"\n"
				"This menu allows you to configure the security requirements for access\n"
				"to this message group. You can also add, delete, and configure the\n"
				"sub-boards of this group by selecting the `Messages Sub-boards...` option.\n"
			;
			switch(uifc.list(WIN_ACT|WIN_T2B,6,0,60,&dflt,0,str,opt)) {
				case -1:
					done=1;
					break;
				case __COUNTER__:
					uifc.helpbuf=grp_long_name_help;
					strcpy(str,cfg.grp[grpnum]->lname);	/* save incase setting to null */
					if(!uifc.input(WIN_MID|WIN_SAV,0,17,"Name to use for Listings"
						,cfg.grp[grpnum]->lname,LEN_GLNAME,K_EDIT))
						strcpy(cfg.grp[grpnum]->lname,str);
					break;
				case __COUNTER__:
					uifc.helpbuf=grp_short_name_help;
					uifc.input(WIN_MID|WIN_SAV,0,17,"Name to use for Prompts"
						,cfg.grp[grpnum]->sname,LEN_GSNAME,K_EDIT);
					break;
				case __COUNTER__:
				{
					char code_prefix[LEN_CODE+1];
					SAFECOPY(code_prefix, cfg.grp[grpnum]->code_prefix);
					uifc.helpbuf=grp_code_prefix_help;
					if(uifc.input(WIN_MID|WIN_SAV,0,17,"Internal Code Prefix"
						,code_prefix,LEN_CODE,K_EDIT|K_UPPER) < 0)
						continue;
					if(code_prefix[0] == 0 || code_ok(code_prefix)) {
						SAFECOPY(cfg.grp[grpnum]->code_prefix, code_prefix);
					} else {
						uifc.helpbuf = invalid_code;
						uifc.msg("Invalid Code Prefix");
					}
					break;
				}
				case __COUNTER__:
					sprintf(str,"%s Group",cfg.grp[grpnum]->sname);
					getar(str,cfg.grp[grpnum]->arstr);
					break;
				case __COUNTER__:
					uifc.helpbuf="`Sort Group By Sub-board:`\n"
						"\n"
						"Normally, the sub-boards within a message group are listed in the order\n"
						"that the sysop created them or a logical order determined by the sysop.\n"
						"\n"
						"Optionally, you can have the group sorted by sub-board `Long Name`,\n"
						"`Short Name`, or `Internal Code`.\n"
						"\n"
						"The group will be automatically re-sorted when new sub-boards\n"
						"are added via `SCFG` or when existing sub-boards are modified.\n"
						;
					j = cfg.grp[grpnum]->sort;
					j = uifc.list(WIN_MID|WIN_SAV, 0, 0, 0, &j, 0, "Sort Group by Sub-board", area_sort_desc);
					if(j >= 0) {
						cfg.grp[grpnum]->sort = j;
						sort_subs(grpnum);
						uifc.changes = TRUE;
					}
					break;
				case __COUNTER__: 	/* Clone Options */
					j=0;
					uifc.helpbuf=
						"`Clone Sub-board Options:`\n"
						"\n"
						"If you want to clone the options of the template sub-board of this group\n"
						"into all sub-boards of this group, select `Yes`.\n"
						"\n"
						"If no template sub-board is found, then the first sub-board in the group\n"
						"is used as the default template.  A sub-board is marked as the template\n"
						"for the group in its `Toggle Options` menu.\n"
						"\n"
						"The options cloned are: posting requirements, reading requirements,\n"
						"operator requirements, moderated user requirements, toggle options,\n"
						"network options (including EchoMail origin line, EchoMail address,\n"
						"and QWK Network tagline), maximum number of messages, maximum number\n"
						"of CRCs, maximum age of messages, storage method, and data directory.\n"
					;
					j=uifc.list(WIN_MID|WIN_SAV,0,0,0,&j,0
						,"Clone Options of Template Sub-board into All of Group",uifcYesNoOpts);
					if(j==0) {
						sub_t* template = NULL;
						for(j=0;j<cfg.total_subs;j++) {
							if(cfg.sub[j]->grp == i && cfg.sub[j]->misc&SUB_TEMPLATE) {
								template = cfg.sub[j];
								break;
							}
						}
						for(j=0;j<cfg.total_subs;j++)
							if(cfg.sub[j]->grp==i) {
								if(template == NULL)
									template = cfg.sub[j];
								else if(cfg.sub[j] != template) {
									uifc.changes=1;
									cfg.sub[j]->misc = template->misc|SUB_HDRMOD;
									cfg.sub[j]->misc &= ~SUB_TEMPLATE;
									SAFECOPY(cfg.sub[j]->post_arstr	,template->post_arstr);
									SAFECOPY(cfg.sub[j]->read_arstr	,template->read_arstr);
									SAFECOPY(cfg.sub[j]->op_arstr	,template->op_arstr);
									SAFECOPY(cfg.sub[j]->mod_arstr	,template->mod_arstr);
									SAFECOPY(cfg.sub[j]->origline	,template->origline);
									SAFECOPY(cfg.sub[j]->tagline	,template->tagline);
									SAFECOPY(cfg.sub[j]->data_dir	,template->data_dir);
									SAFECOPY(cfg.sub[j]->post_sem	,template->post_sem);
									cfg.sub[j]->maxmsgs = template->maxmsgs;
									cfg.sub[j]->maxcrcs = template->maxcrcs;
									cfg.sub[j]->maxage	= template->maxage;
									cfg.sub[j]->faddr	= template->faddr; 
								} 
							} 
					}
					break;
				case __COUNTER__:
					k=0;
					ported=0;
					q=uifc.changes;
					strcpy(opt[k++],"subs.txt       Synchronet Sub-boards");
					strcpy(opt[k++],"areas.bbs      SBBSecho Area File");
					strcpy(opt[k++],"backbone.na    FidoNet EchoList");
					opt[k][0]=0;
					uifc.helpbuf=
						"`Export Area File Format:`\n"
						"\n"
						"This menu allows you to choose the format of the area file you wish to\n"
						"export the current message group into.\n"
						"\n"
						"The supported message area list file formats to be exported are:\n"
						"\n"
						"`subs.txt`\n"
						"  Complete details of a group of `Synchronet sub-boards` in text format.\n"
						"\n"
						"`areas.bbs`\n"
						"  Area File as used by the Synchronet Fido EchoMail program, `SBBSecho`.\n"
						"\n"
						"`backbone.na` (also `fidonet.na` and `badareas.lst`)\n"
						"  FidoNet standard EchoList containing standardized echo `Area Tags`\n"
						"  and (optional) descriptions.\n"

					;
					k = uifc.list(WIN_MID|WIN_SAV,0,0,0,&export_list_type,0
						,"Export Area File Format",opt);
					if(k==-1)
						break;
					if(k==0)
						sprintf(str,"%ssubs.txt",cfg.ctrl_dir);
					else if(k==1)
						sprintf(str,"%sareas.bbs",cfg.data_dir);
					else if(k==2)
						sprintf(str,"backbone.na");
					if(k==1)
						if(uifc.input(WIN_MID|WIN_SAV,0,0,"Uplinks"
							,str2,sizeof(str2)-1,0)<=0) {
							uifc.changes=q;
							break; 
						}
					if(uifc.input(WIN_MID|WIN_SAV,0,0,"Filename"
						,str,sizeof(str)-1,K_EDIT)<=0) {
						uifc.changes=q;
						break; 
					}
					if(fexist(str)) {
						strcpy(opt[0],"Overwrite");
						strcpy(opt[1],"Append");
						opt[2][0]=0;
						j=0;
						j=uifc.list(WIN_MID|WIN_SAV,0,0,0,&j,0
							,"File Exists",opt);
						if(j==-1)
							break;
						if(j==0) j=O_WRONLY|O_TRUNC;
						else	 j=O_WRONLY|O_APPEND; 
					}
					else
						j=O_WRONLY|O_CREAT;
					if((stream=fnopen(&file,str,j))==NULL) {
						sprintf(str,"Open Failure: %d (%s)"
							,errno,strerror(errno));
						uifc.msg(str);
						uifc.changes=q;
						break; 
					}
					uifc.pop("Exporting Areas...");
					for(j=0;j<cfg.total_subs;j++) {
						if(cfg.sub[j]->grp!=i)
							continue;
						ported++;
						if(k==1) {		/* AREAS.BBS SBBSecho */
							char extcode[LEN_EXTCODE+1];
							SAFEPRINTF2(extcode,"%s%s"
								,cfg.grp[cfg.sub[j]->grp]->code_prefix
								,cfg.sub[j]->code_suffix);

							fprintf(stream,"%-*s %-*s %s\r\n"
								,LEN_EXTCODE, extcode
								,FIDO_AREATAG_LEN, stou(cfg.sub[j]->sname)
								,str2);
							continue; 
						}
						if(k==2) {		/* BACKBONE.NA */
							fprintf(stream,"%-*s %s\r\n"
								,FIDO_AREATAG_LEN, stou(cfg.sub[j]->sname),cfg.sub[j]->lname);
							continue; 
						}
						fprintf(stream,"%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n"
								"%s\r\n%s\r\n%s\r\n"
							,cfg.sub[j]->lname
							,cfg.sub[j]->sname
							,cfg.sub[j]->qwkname
							,cfg.sub[j]->code_suffix
							,cfg.sub[j]->data_dir
							,cfg.sub[j]->arstr
							,cfg.sub[j]->read_arstr
							,cfg.sub[j]->post_arstr
							,cfg.sub[j]->op_arstr
							);
						fprintf(stream,"%"PRIX32"\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n"
							,cfg.sub[j]->misc
							,cfg.sub[j]->tagline
							,cfg.sub[j]->origline
							,cfg.sub[j]->post_sem
							,cfg.sub[j]->newsgroup
							,smb_faddrtoa(&cfg.sub[j]->faddr,tmp)
							);
						fprintf(stream,"%"PRIu32"\r\n%"PRIu32"\r\n%u\r\n%u\r\n%s\r\n"
							,cfg.sub[j]->maxmsgs
							,cfg.sub[j]->maxcrcs
							,cfg.sub[j]->maxage
							,cfg.sub[j]->ptridx
							,cfg.sub[j]->mod_arstr
							);
						fprintf(stream,"***END-OF-SUB***\r\n\r\n"); 
					}
					fclose(stream);
					uifc.pop(0);
					sprintf(str,"%lu Message Areas Exported Successfully",ported);
					uifc.msg(str);
					uifc.changes=q;
					break;
				case __COUNTER__:
					ported=0;
					k=0;
					strcpy(opt[k++],"subs.txt        Synchronet Sub-boards");
					strcpy(opt[k++],"control.dat     QWK Conference List");
					strcpy(opt[k++],"areas.bbs       Generic Area File");
					strcpy(opt[k++],"areas.bbs       SBBSecho Area File");
					strcpy(opt[k++],"backbone.na     FidoNet EchoList");
					strcpy(opt[k++],"badareas.lst    SBBSecho Bad Area List");
					opt[k][0]=0;
					uifc.helpbuf=
						"`Import Area File Format:`\n"
						"\n"
						"This menu allows you to choose the format of the area file you wish to\n"
						"import into the current message group.\n"
						"\n"
						"The supported message area list file formats to be imported are:\n"
						"\n"
						"`subs.txt`\n"
						"  Complete details of a group of sub-boards as exported from `SCFG`.\n"
						"\n"
						"`areas.bbs`\n"
						"  FidoNet EchoMail Area File, in either `Generic` or `SBBSecho` flavors,\n"
						"  as used by most FidoNet EchoMail Programs or SBBSecho.\n"
						"\n"
						"`backbone.na` (also `fidonet.na` and `badareas.lst`)\n"
						"  FidoNet standard EchoList containing standardized echo `Area Tags`\n"
						"  and (optional) descriptions.\n"
					;
					k=uifc.list(WIN_MID|WIN_SAV,0,0,0,&import_list_type,0
						,"Import Area File Format",opt);
					if(k < 0)
						break;
					char filename[MAX_PATH+1];
					switch(k) {
						case IMPORT_LIST_TYPE_SUBS_TXT:
							sprintf(filename, "%ssubs.txt", cfg.ctrl_dir);
							break;
						case IMPORT_LIST_TYPE_QWK_CONTROL_DAT:
							sprintf(filename, "control.dat");
							break;
						case IMPORT_LIST_TYPE_GENERIC_AREAS_BBS:
							sprintf(filename, "areas.bbs");
							break;
						case IMPORT_LIST_TYPE_SBBSECHO_AREAS_BBS:
							sprintf(filename, "%sareas.bbs", cfg.data_dir);
							break;
						case IMPORT_LIST_TYPE_BACKBONE_NA:
							sprintf(filename, "backbone.na");
							break;
						default:
							sprintf(filename,"%sbadareas.lst", cfg.data_dir);
							k = IMPORT_LIST_TYPE_BACKBONE_NA;
							break;
					}

					/* QWK Conference number range */
					int min_confnum, max_confnum;
					if(k == IMPORT_LIST_TYPE_QWK_CONTROL_DAT) {
						strcpy(str, "1000");
						uifc.helpbuf = "`Minimum / Maximum QWK Conference Number:`\n"
							"\n"
							"This setting allows you to control the range of QWK conference numbers\n"
							"that will be imported from the `control.dat` file.";
						if(uifc.input(WIN_MID|WIN_SAV,0,0,"Minimum QWK Conference Number"
							,str,5,K_EDIT|K_NUMBER) < 1)
							break;
						min_confnum = atoi(str);
						sprintf(str, "%u", min_confnum + 999);
						if(uifc.input(WIN_MID|WIN_SAV,0,0,"Maximum QWK Conference Number"
							,str,5,K_EDIT|K_NUMBER) < 1)
							break;
						max_confnum = atoi(str);
					}
					uifc.helpbuf = "Enter the path to the Area List file to import";
					if(uifc.input(WIN_MID|WIN_SAV,0,0,"Filename"
						,filename,sizeof(filename)-1,K_EDIT)<=0)
						break;
					fexistcase(filename);
					if((stream=fnopen(&file,filename,O_RDONLY))==NULL) {
						uifc.msg("Open Failure");
						break; 
					}
					uifc.pop("Importing Areas...");
					long added = 0;
					ported = import_msg_areas(k, stream, i, min_confnum, max_confnum, /* qhub: */NULL, &added);
					fclose(stream);
					uifc.pop(0);
					if(ported < 0)
						sprintf(str, "!ERROR %ld imported message areas", ported);
					else {
						sprintf(str, "%ld Message Areas Imported Successfully (%ld added)",ported, added);
						if(ported > 0)
							uifc.changes = TRUE;
					}
					uifc.msg(str);
					break;

				case __COUNTER__:
					sub_cfg(i);
					break; 
			} 
		} 
	}
}

/* Configuring Message Options */
void msg_opts()
{
	char str[128];
	static int msg_dflt;
	int i,j,n;

	while(1) {
		i=0;
		sprintf(opt[i++],"%-33.33s%s"
			,"BBS ID for QWK Packets",cfg.sys_id);
		sprintf(opt[i++],"%-33.33s%u seconds"
			,"Maximum Retry Time",cfg.smb_retry_time);
		if(cfg.max_qwkmsgs)
			sprintf(str,"%"PRIu32,cfg.max_qwkmsgs);
		else
			sprintf(str,"Unlimited");
		sprintf(opt[i++],"%-33.33s%s"
			,"Maximum QWK Messages",str);
		if(cfg.max_qwkmsgage)
			sprintf(str,"%u days",cfg.max_qwkmsgage);
		else
			sprintf(str,"Unlimited");
		sprintf(opt[i++],"%-33.33s%s"
			,"Maximum QWK Message Age",str);
		sprintf(opt[i++],"%-33.33s%s","Pre-pack QWK Requirements",cfg.preqwk_arstr);
		if(cfg.mail_maxage)
			sprintf(str,"Enabled (%u days old)",cfg.mail_maxage);
        else
            strcpy(str,"Disabled");
		sprintf(opt[i++],"%-33.33s%s","Purge E-mail by Age",str);
		if(cfg.sys_misc&SM_DELEMAIL)
			strcpy(str,"Immediately");
		else
			strcpy(str,"Daily");
		sprintf(opt[i++],"%-33.33s%s","Purge Deleted E-mail",str);
		if(cfg.mail_maxcrcs)
			sprintf(str,"Enabled (%"PRIu32" mail CRCs)",cfg.mail_maxcrcs);
		else
			strcpy(str,"Disabled");
		sprintf(opt[i++],"%-33.33s%s","Duplicate E-mail Checking",str);
		sprintf(opt[i++],"%-33.33s%s","Allow Anonymous E-mail"
			,cfg.sys_misc&SM_ANON_EM ? "Yes" : "No");
		sprintf(opt[i++],"%-33.33s%s","Allow Quoting in E-mail"
			,cfg.sys_misc&SM_QUOTE_EM ? "Yes" : "No");
		sprintf(opt[i++],"%-33.33s%s","Allow Uploads in E-mail"
			,cfg.sys_misc&SM_FILE_EM ? "Yes" : "No");
		sprintf(opt[i++],"%-33.33s%s","Allow Forwarding to NetMail"
			,cfg.sys_misc&SM_FWDTONET ? "Yes" : "No");
		sprintf(opt[i++],"%-33.33s%s","Kill Read E-mail"
			,cfg.sys_misc&SM_DELREADM ? "Yes" : "No");
		sprintf(opt[i++],"%-33.33s%s","Receive E-mail by Real Name"
			,cfg.msg_misc&MM_REALNAME ? "Yes" : "No");
		sprintf(opt[i++],"%-33.33s%s","Include Signatures in E-mail"
			,cfg.msg_misc&MM_EMAILSIG ? "Yes" : "No");
		sprintf(opt[i++],"%-33.33s%s","Users Can View Deleted Messages"
			,cfg.sys_misc&SM_USRVDELM ? "Yes" : cfg.sys_misc&SM_SYSVDELM
				? "Sysops Only":"No");
		sprintf(opt[i++],"%-33.33s%hu","Days of New Messages for Guest", cfg.guest_msgscan_init);
		strcpy(opt[i++],"Extra Attribute Codes...");
		opt[i][0]=0;
		uifc.helpbuf=
			"`Message Options:`\n"
			"\n"
			"This is a menu of system-wide message related options. Messages include\n"
			"E-mail and public posts (on sub-boards).\n"
		;

		switch(uifc.list(WIN_ORG|WIN_ACT|WIN_MID|WIN_CHE,0,0,72,&msg_dflt,0
			,"Message Options",opt)) {
			case -1:
				i=save_changes(WIN_MID);
				if(i==-1)
				   continue;
				if(!i) {
					cfg.new_install=new_install;
					write_msgs_cfg(&cfg,backup_level);
					write_main_cfg(&cfg,backup_level);
                    refresh_cfg(&cfg);
                }
				return;
			case 0:
				strcpy(str,cfg.sys_id);
				uifc.helpbuf=
					"`BBS ID for QWK Packets:`\n"
					"\n"
					"This is a short system ID for your BBS that is used for QWK packets.\n"
					"It should be an abbreviation of your BBS name or other related string.\n"
					"This ID will be used for your outgoing and incoming QWK packets. If\n"
					"you plan on networking via QWK packets with another Synchronet BBS,\n"
					"this ID should not begin with a number. The maximum length of the ID\n"
					"is eight characters and cannot contain spaces or other invalid DOS\n"
					"filename characters. In a QWK packet network, each system must have\n"
					"a unique QWK system ID.\n"
				;

				uifc.input(WIN_MID|WIN_SAV,0,0,"BBS ID for QWK Packets"
					,str,LEN_QWKID,K_EDIT|K_UPPER);
				if(code_ok(str))
					strcpy(cfg.sys_id,str);
				else
					uifc.msg("Invalid ID");
				break;
			case 1:
				uifc.helpbuf=
					"`Maximum Message Base Retry Time:`\n"
					"\n"
					"This is the maximum number of seconds to allow while attempting to open\n"
					"or lock a message base (a value in the range of 10 to 45 seconds should\n"
					"be fine).\n"
				;
				ultoa(cfg.smb_retry_time,str,10);
				uifc.input(WIN_MID|WIN_SAV,0,0
					,"Maximum Message Base Retry Time (in seconds)"
					,str,2,K_NUMBER|K_EDIT);
				cfg.smb_retry_time=atoi(str);
				break;
			case 2:
				uifc.helpbuf=
					"`Maximum Messages Per QWK Packet:`\n"
					"\n"
					"This is the maximum number of messages (excluding E-mail), that a user\n"
					"can have in one QWK packet for download. This limit does not effect\n"
					"QWK network nodes (`Q` restriction). If set to `0`, no limit is imposed.\n"
				;

				ultoa(cfg.max_qwkmsgs,str,10);
				uifc.input(WIN_MID|WIN_SAV,0,0
					,"Maximum Messages Per QWK Packet (0=No Limit)"
					,str,6,K_NUMBER|K_EDIT);
				cfg.max_qwkmsgs=atol(str);
                break;
			case 3:
				uifc.helpbuf=
					"`Maximum Age of Messages Imported From QWK Packets:`\n"
					"\n"
					"This is the maximum age of messages (in days), allowed for messages in\n"
					"QWK packets. Messages with an age older than this value will not be\n"
					"imported. If set to `0`, no age limit is imposed.\n"
				;

				itoa(cfg.max_qwkmsgage,str,10);
				uifc.input(WIN_MID|WIN_SAV,0,0
					,"Maximum Age (in days) of QWK-imported Messages (0=No Limit)"
					,str,4,K_NUMBER|K_EDIT);
				cfg.max_qwkmsgage=atoi(str);
                break;
			case 4:
				uifc.helpbuf=
					"`Pre-pack QWK Requirements:`\n"
					"\n"
					"ALL user accounts on the BBS meeting this requirement will have a QWK\n"
					"packet automatically created for them every day after midnight\n"
					"(during the internal daily event).\n"
					"\n"
					"This is mainly intended for QWK network nodes that wish to save connect\n"
					"time by having their packets pre-packed. If a large number of users meet\n"
					"this requirement, it can take up a large amount of disk space on your\n"
					"system (in the `data/file` directory).\n"
				;
				getar("Pre-pack QWK (Use with caution!)",cfg.preqwk_arstr);
				break;
			case 5:
				sprintf(str,"%u",cfg.mail_maxage);
                uifc.helpbuf=
	                "`Maximum Age of Mail:`\n"
	                "\n"
	                "This value is the maximum number of days that mail will be kept.\n"
                ;
                uifc.input(WIN_MID|WIN_SAV,0,17,"Maximum Age of Mail "
                    "(in days)",str,5,K_EDIT|K_NUMBER);
                cfg.mail_maxage=atoi(str);
                break;
			case 6:
				strcpy(opt[0],"Daily");
				strcpy(opt[1],"Immediately");
				opt[2][0]=0;
				i=cfg.sys_misc&SM_DELEMAIL ? 0:1;
				uifc.helpbuf=
					"`Purge Deleted E-mail:`\n"
					"\n"
					"If you wish to have deleted e-mail physically (and permanently) removed\n"
					"from your e-mail database immediately after a users exits the reading\n"
					"e-mail prompt, set this option to `Immediately`.\n"
					"\n"
					"For the best system performance and to avoid delays when deleting e-mail\n"
					"from a large e-mail database, set this option to `Daily` (the default).\n"
					"Your system maintenance will automatically purge deleted e-mail once a\n"
					"day.\n"
				;

				i=uifc.list(WIN_MID|WIN_SAV,0,0,0,&i,0
					,"Purge Deleted E-mail",opt);
				if(!i && cfg.sys_misc&SM_DELEMAIL) {
					cfg.sys_misc&=~SM_DELEMAIL;
					uifc.changes=1; 
				}
				else if(i==1 && !(cfg.sys_misc&SM_DELEMAIL)) {
					cfg.sys_misc|=SM_DELEMAIL;
					uifc.changes=1; 
				}
                break;
			case 7:
				sprintf(str,"%"PRIu32,cfg.mail_maxcrcs);
                uifc.helpbuf=
	                "`Maximum Number of Mail CRCs:`\n"
	                "\n"
	                "This value is the maximum number of CRCs that will be kept for duplicate\n"
	                "mail checking. Once this maximum number of CRCs is reached, the oldest\n"
	                "CRCs will be automatically purged.\n"
                ;
                uifc.input(WIN_MID|WIN_SAV,0,17,"Maximum Number of Mail "
                    "CRCs",str,5,K_EDIT|K_NUMBER);
                cfg.mail_maxcrcs=atol(str);
                break;
			case 8:
				strcpy(opt[0],"Yes");
				strcpy(opt[1],"No");
				opt[2][0]=0;
				i=cfg.sys_misc&SM_ANON_EM ? 0:1;
				uifc.helpbuf=
					"`Allow Anonymous E-mail:`\n"
					"\n"
					"If you want users with the `A` exemption to be able to send E-mail\n"
					"anonymously, set this option to `Yes`.\n"
				;

				i=uifc.list(WIN_MID|WIN_SAV,0,0,0,&i,0
					,"Allow Anonymous E-mail",opt);
				if(!i && !(cfg.sys_misc&SM_ANON_EM)) {
					cfg.sys_misc|=SM_ANON_EM;
					uifc.changes=1; 
				}
				else if(i==1 && cfg.sys_misc&SM_ANON_EM) {
					cfg.sys_misc&=~SM_ANON_EM;
					uifc.changes=1; 
				}
				break;
			case 9:
				strcpy(opt[0],"Yes");
				strcpy(opt[1],"No");
				opt[2][0]=0;
				i=cfg.sys_misc&SM_QUOTE_EM ? 0:1;
				uifc.helpbuf=
					"`Allow Quoting in E-mail:`\n"
					"\n"
					"If you want your users to be allowed to use message quoting when\n"
					"responding in E-mail, set this option to `Yes`.\n"
				;

				i=uifc.list(WIN_MID|WIN_SAV,0,0,0,&i,0
					,"Allow Quoting in E-mail",opt);
				if(!i && !(cfg.sys_misc&SM_QUOTE_EM)) {
					cfg.sys_misc|=SM_QUOTE_EM;
					uifc.changes=1; 
				}
				else if(i==1 && cfg.sys_misc&SM_QUOTE_EM) {
					cfg.sys_misc&=~SM_QUOTE_EM;
					uifc.changes=1; 
				}
				break;
			case 10:
				strcpy(opt[0],"Yes");
				strcpy(opt[1],"No");
				opt[2][0]=0;
				i=cfg.sys_misc&SM_FILE_EM ? 0:1;
				uifc.helpbuf=
					"`Allow File Attachment Uploads in E-mail:`\n"
					"\n"
					"If you want your users to be allowed to attach an uploaded file to\n"
					"an E-mail message, set this option to `Yes`.\n"
				;

				i=uifc.list(WIN_MID|WIN_SAV,0,0,0,&i,0
					,"Allow File Attachment Uploads in E-mail",opt);
				if(!i && !(cfg.sys_misc&SM_FILE_EM)) {
					cfg.sys_misc|=SM_FILE_EM;
					uifc.changes=1; 
				}
				else if(i==1 && cfg.sys_misc&SM_FILE_EM) {
					cfg.sys_misc&=~SM_FILE_EM;
					uifc.changes=1; 
				}
				break;
			case 11:
				strcpy(opt[0],"Yes");
				strcpy(opt[1],"No");
				opt[2][0]=0;
				i=cfg.sys_misc&SM_FWDTONET ? 0:1;
				uifc.helpbuf=
					"`Allow Users to Have Their E-mail Forwarded to NetMail:`\n"
					"\n"
					"If you want your users to be able to have any e-mail sent to them\n"
					"optionally (at the sender's discretion) forwarded to a NetMail address,\n"
					"set this option to `Yes`.\n"
				;

				i=uifc.list(WIN_MID|WIN_SAV,0,0,0,&i,0
					,"Allow Forwarding of E-mail to NetMail",opt);
				if(!i && !(cfg.sys_misc&SM_FWDTONET)) {
					cfg.sys_misc|=SM_FWDTONET;
					uifc.changes=1; 
				}
				else if(i==1 && cfg.sys_misc&SM_FWDTONET) {
					cfg.sys_misc&=~SM_FWDTONET;
					uifc.changes=1; 
				}
                break;
			case 12:
				strcpy(opt[0],"Yes");
				strcpy(opt[1],"No");
				opt[2][0]=0;
				i=cfg.sys_misc&SM_DELREADM ? 0:1;
				uifc.helpbuf=
					"`Kill Read E-mail Automatically:`\n"
					"\n"
					"If this option is set to `Yes`, e-mail that has been read will be\n"
					"automatically deleted when message base maintenance is run.\n"
				;

				i=uifc.list(WIN_MID|WIN_SAV,0,0,0,&i,0
					,"Kill Read E-mail Automatically",opt);
				if(!i && !(cfg.sys_misc&SM_DELREADM)) {
					cfg.sys_misc|=SM_DELREADM;
					uifc.changes=1; 
				}
				else if(i==1 && cfg.sys_misc&SM_DELREADM) {
					cfg.sys_misc&=~SM_DELREADM;
					uifc.changes=1; 
				}
                break;
			case 13:
				strcpy(opt[0],"Yes");
				strcpy(opt[1],"No");
				opt[2][0]=0;
				i=cfg.msg_misc&MM_REALNAME ? 0:1;
				uifc.helpbuf=
					"`Receive E-mail by Real Name:`\n"
					"\n"
					"If this option is set to ~Yes~, e-mail messages may be received when\n"
					"addressed to a user's real name (rather than their alias).\n"
				;

				i=uifc.list(WIN_MID|WIN_SAV,0,0,0,&i,0
					,"Receive E-mail by Real Name",opt);
				if(!i && !(cfg.msg_misc&MM_REALNAME)) {
					cfg.msg_misc|=MM_REALNAME;
					uifc.changes=1; 
				}
				else if(i==1 && cfg.msg_misc&MM_REALNAME) {
					cfg.msg_misc&=~MM_REALNAME;
					uifc.changes=1; 
				}
                break;
			case 14:
				n=(cfg.sub[i]->misc&MM_EMAILSIG) ? 0:1;
				strcpy(opt[0],"Yes");
				strcpy(opt[1],"No");
				opt[2][0]=0;
				uifc.helpbuf=
					"`Include User Signatures in E-mail:`\n"
					"\n"
					"If you wish to have user signatures automatically appended to e-mail\n"
					"messages, set this option to ~Yes~.\n"
				;
				n=uifc.list(WIN_SAV|WIN_MID,0,0,0,&n,0
					,"Include User Signatures in E-mail",opt);
				if(n==-1)
                    break;
				if(!n && !(cfg.msg_misc&MM_EMAILSIG)) {
					uifc.changes=1;
					cfg.msg_misc|=MM_EMAILSIG;
					break; 
				}
				if(n==1 && cfg.msg_misc&MM_EMAILSIG) {
					uifc.changes=1;
					cfg.msg_misc&=~MM_EMAILSIG; 
				}
                break;
			case 15:
				strcpy(opt[0],"Yes");
				strcpy(opt[1],"No");
				strcpy(opt[2],"Sysops Only");
				opt[3][0]=0;
				i=1;
				uifc.helpbuf=
					"`Users Can View Deleted Messages:`\n"
					"\n"
					"If this option is set to `Yes`, then users will be able to view messages\n"
					"they've sent and deleted or messages sent to them and they've deleted\n"
					"with the option of un-deleting the message before the message is\n"
					"physically purged from the e-mail database.\n"
					"\n"
					"If this option is set to `No`, then when a message is deleted, it is no\n"
					"longer viewable (with SBBS) by anyone.\n"
					"\n"
					"If this option is set to `Sysops Only`, then only sysops and sub-ops (when\n"
					"appropriate) can view deleted messages.\n"
				;

				i=uifc.list(WIN_MID|WIN_SAV,0,0,0,&i,0
					,"Users Can View Deleted Messages",opt);
				if(!i && (cfg.sys_misc&(SM_USRVDELM|SM_SYSVDELM))
					!=(SM_USRVDELM|SM_SYSVDELM)) {
					cfg.sys_misc|=(SM_USRVDELM|SM_SYSVDELM);
					uifc.changes=1; 
				}
				else if(i==1 && cfg.sys_misc&(SM_USRVDELM|SM_SYSVDELM)) {
					cfg.sys_misc&=~(SM_USRVDELM|SM_SYSVDELM);
					uifc.changes=1; 
				}
				else if(i==2 && (cfg.sys_misc&(SM_USRVDELM|SM_SYSVDELM))
					!=SM_SYSVDELM) {
					cfg.sys_misc|=SM_SYSVDELM;
					cfg.sys_misc&=~SM_USRVDELM;
					uifc.changes=1; 
				}
                break;
			case 16:
				uifc.helpbuf=
					"`Days of New Messages for Guest:`\n"
					"\n"
					"This option allows you to set the number of days worth of messages\n"
					"which will be included in a Guest login's `new message scan`.\n"
					"\n"
					"The value `0` means there will be `no` new messages for the Guest account.\n"
				;
				sprintf(str,"%hu",cfg.guest_msgscan_init);
				uifc.input(WIN_SAV|WIN_MID,0,0
					,"Days of New Messages for Guest's new message scan"
					,str,4,K_EDIT|K_NUMBER);
				cfg.guest_msgscan_init=atoi(str);
                break;
			case 17:
				uifc.helpbuf=
					"`Extra Attribute Codes...`\n"
					"\n"
					"Synchronet can support the native text attribute codes of other BBS\n"
					"programs in messages (menus, posts, e-mail, etc.) To enable the extra\n"
					"attribute codes for another BBS program, set the corresponding option\n"
					"to `Yes`.\n"
				;

				j=0;
				while(1) {
					i=0;
					sprintf(opt[i++],"%-15.15s %-3.3s","WWIV"
						,cfg.sys_misc&SM_WWIV ? "Yes":"No");
					sprintf(opt[i++],"%-15.15s %-3.3s","PCBoard"
						,cfg.sys_misc&SM_PCBOARD ? "Yes":"No");
					sprintf(opt[i++],"%-15.15s %-3.3s","Wildcat"
						,cfg.sys_misc&SM_WILDCAT ? "Yes":"No");
					sprintf(opt[i++],"%-15.15s %-3.3s","Celerity"
						,cfg.sys_misc&SM_CELERITY ? "Yes":"No");
					sprintf(opt[i++],"%-15.15s %-3.3s","Renegade"
						,cfg.sys_misc&SM_RENEGADE ? "Yes":"No");
					opt[i][0]=0;
					j=uifc.list(WIN_BOT|WIN_RHT|WIN_SAV,2,2,0,&j,0
						,"Extra Attribute Codes",opt);
					if(j==-1)
						break;

					uifc.changes=1;
					switch(j) {
						case 0:
							cfg.sys_misc^=SM_WWIV;
							break;
						case 1:
							cfg.sys_misc^=SM_PCBOARD;
							break;
						case 2:
							cfg.sys_misc^=SM_WILDCAT;
							break;
						case 3:
							cfg.sys_misc^=SM_CELERITY;
							break;
						case 4:
							cfg.sys_misc^=SM_RENEGADE;
							break; 
				} 
			} 
		} 
	}
}
