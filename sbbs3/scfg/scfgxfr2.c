/* scfgxfr2.c */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2012 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#define DEFAULT_DIR_OPTIONS (DIR_FCHK|DIR_MULT|DIR_DUPES|DIR_CDTUL|DIR_CDTDL|DIR_DIZ)

static void append_dir_list(const char* parent, const char* dir, FILE* fp, int depth, int max_depth)
{
	char		path[MAX_PATH+1];
	char*		p;
	glob_t		g;
	unsigned	gi;

	SAFECOPY(path,dir);
	backslash(path);
	strcat(path,ALLFILES);

	glob(path,GLOB_MARK,NULL,&g);
   	for(gi=0;gi<g.gl_pathc;gi++) {
		if(*lastchar(g.gl_pathv[gi])=='/') {
			if(getdirsize(g.gl_pathv[gi], /* include_subdirs */ FALSE, /* subdir_only */FALSE) > 0) {
				SAFECOPY(path,g.gl_pathv[gi]+strlen(parent));
				p=lastchar(path);
				if(IS_PATH_DELIM(*p))
					*p=0;
				fprintf(fp,"%s\n",path);
			}
			if(max_depth==0 || depth+1 < max_depth) {
				append_dir_list(parent, g.gl_pathv[gi], fp, depth+1, max_depth);
			}
		}
	}
	globfree(&g);
}

BOOL create_raw_dir_list(const char* list_file)
{
	char		path[MAX_PATH+1];
	char*		p;
	int			k=0;
	FILE*		fp;

	SAFECOPY(path, list_file);
	if((p=getfname(path))!=NULL)
		*p=0;
	if(uifc.input(WIN_MID|WIN_SAV,0,0,"Parent Directory",path,sizeof(path)-1
		,K_EDIT)<1)
		return(FALSE);
	k=uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0,"Recursive",uifcYesNoOpts);
	if(k<0)
		return(FALSE);
	if((fp=fopen(list_file,"w"))==NULL) {
		SAFEPRINTF2(path,"Create Failure (%u): %s", errno, list_file);
		uifc.msg(path);
		return(FALSE); 
	}
	backslash(path);
	append_dir_list(path, path, fp, /* depth: */0, /* max_depth: */k);
	fclose(fp);
	return(TRUE);
}


void xfer_cfg()
{
	static int libs_dflt,libs_bar,dflt;
	char str[256],str2[81],done=0,*p;
	char tmp_code[32];
	int file,j,k,q;
	uint i;
	long ported,added;
	static lib_t savlib;
	dir_t tmpdir;
	FILE *stream;

while(1) {
	for(i=0;i<cfg.total_libs && i<MAX_OPTS;i++)
		sprintf(opt[i],"%-25s",cfg.lib[i]->lname);
	opt[i][0]=0;
	j=WIN_ACT|WIN_CHE|WIN_ORG;
	if(cfg.total_libs)
		j|=WIN_DEL|WIN_GET|WIN_DELACT;
	if(cfg.total_libs<MAX_OPTS)
		j|=WIN_INS|WIN_INSACT|WIN_XTR;
	if(savlib.sname[0])
		j|=WIN_PUT;
	uifc.helpbuf=
		"`File Libraries:`\n"
		"\n"
		"This is a listing of file libraries for your BBS. File Libraries are\n"
		"used to logically separate your file `directories` into groups. Every\n"
		"directory belongs to a file library.\n"
		"\n"
		"One popular use for file libraries is to separate CD-ROM and hard disk\n"
		"directories. One might have an `Uploads` file library that contains\n"
		"uploads to the hard disk directories and also have a `PC-SIG` file\n"
		"library that contains directories from a PC-SIG CD-ROM. Some sysops\n"
		"separate directories into more specific areas such as `Main`, `Graphics`,\n"
		"or `Adult`. If you have many directories that have a common subject\n"
		"denominator, you may want to have a separate file library for those\n"
		"directories for a more organized file structure.\n"
	;
	i=uifc.list(j,0,0,45,&libs_dflt,&libs_bar,"File Libraries",opt);
	if((signed)i==-1) {
		j=save_changes(WIN_MID);
		if(j==-1)
			continue;
		if(!j) {
			write_file_cfg(&cfg,backup_level);
            refresh_cfg(&cfg);
        }
		return; }
	if((i&MSK_ON)==MSK_INS) {
		i&=MSK_OFF;
		strcpy(str,"Main");
		uifc.helpbuf=
			"`Library Long Name:`\n"
			"\n"
			"This is a description of the file library which is displayed when a\n"
			"user of the system uses the `/*` command from the file transfer menu.\n"
		;
		if(uifc.input(WIN_MID|WIN_SAV,0,0,"Library Long Name",str,LEN_GLNAME
			,K_EDIT)<1)
			continue;
		sprintf(str2,"%.*s",LEN_GSNAME,str);
		uifc.helpbuf=
			"`Library Short Name:`\n"
			"\n"
			"This is a short description of the file library which is used for the\n"
			"file transfer menu prompt.\n"
		;
		if(uifc.input(WIN_MID|WIN_SAV,0,0,"Library Short Name",str2,LEN_GSNAME
			,K_EDIT)<1)
			continue;
		if((cfg.lib=(lib_t **)realloc(cfg.lib,sizeof(lib_t *)*(cfg.total_libs+1)))==NULL) {
			errormsg(WHERE,ERR_ALLOC,nulstr,cfg.total_libs+1);
			cfg.total_libs=0;
			bail(1);
            continue; }

		if(cfg.total_libs) {
			for(j=cfg.total_libs;j>i;j--)
                cfg.lib[j]=cfg.lib[j-1];
			for(j=0;j<cfg.total_dirs;j++)
				if(cfg.dir[j]->lib>=i)
					cfg.dir[j]->lib++; }
		if((cfg.lib[i]=(lib_t *)malloc(sizeof(lib_t)))==NULL) {
			errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(lib_t));
			continue; }
		memset((lib_t *)cfg.lib[i],0,sizeof(lib_t));
		strcpy(cfg.lib[i]->lname,str);
		strcpy(cfg.lib[i]->sname,str2);
		cfg.total_libs++;
		uifc.changes=1;
		continue; }
	if((i&MSK_ON)==MSK_DEL) {
		i&=MSK_OFF;
		uifc.helpbuf=
			"`Delete All Data in Library:`\n"
			"\n"
			"If you wish to delete the database files for all directories in this\n"
			"library, select `Yes`.\n"
		;
		j=1;
		strcpy(opt[0],"Yes");
		strcpy(opt[1],"No");
		opt[2][0]=0;
		j=uifc.list(WIN_MID|WIN_SAV,0,0,0,&j,0
			,"Delete All Library Data Files",opt);
		if(j==-1)
			continue;
		if(j==0)
			for(j=0;j<cfg.total_dirs;j++)
				if(cfg.dir[j]->lib==i) {
					sprintf(str,"%s%s.*"
						,cfg.lib[cfg.dir[j]->lib]->code_prefix
						,cfg.dir[j]->code_suffix);
					strlwr(str);
					if(!cfg.dir[j]->data_dir[0])
						sprintf(tmp,"%sdirs/",cfg.data_dir);
					else
						strcpy(tmp,cfg.dir[j]->data_dir);
					delfiles(tmp,str); }
		free(cfg.lib[i]);
		for(j=0;j<cfg.total_dirs;) {
			if(cfg.dir[j]->lib==i) {
				free(cfg.dir[j]);
				cfg.total_dirs--;
				k=j;
				while(k<cfg.total_dirs) {
					cfg.dir[k]=cfg.dir[k+1];
					k++; } }
			else j++; }
		for(j=0;j<cfg.total_dirs;j++)
			if(cfg.dir[j]->lib>i)
				cfg.dir[j]->lib--;
		cfg.total_libs--;
		while(i<cfg.total_libs) {
			cfg.lib[i]=cfg.lib[i+1];
			i++; }
		uifc.changes=1;
		continue; }
	if((i&MSK_ON)==MSK_GET) {
		i&=MSK_OFF;
		savlib=*cfg.lib[i];
		continue; }
	if((i&MSK_ON)==MSK_PUT) {
		i&=MSK_OFF;
		*cfg.lib[i]=savlib;
		uifc.changes=1;
        continue; }
	done=0;
	while(!done) {
		j=0;
		sprintf(opt[j++],"%-27.27s%s","Long Name",cfg.lib[i]->lname);
		sprintf(opt[j++],"%-27.27s%s","Short Name",cfg.lib[i]->sname);
		sprintf(opt[j++],"%-27.27s%s","Internal Code Prefix",cfg.lib[i]->code_prefix);
		sprintf(opt[j++],"%-27.27s%.40s","Parent Directory"
			,cfg.lib[i]->parent_path);
		sprintf(opt[j++],"%-27.27s%.40s","Access Requirements"
			,cfg.lib[i]->arstr);
		strcpy(opt[j++],"Clone Options");
		strcpy(opt[j++],"Export Areas...");
		strcpy(opt[j++],"Import Areas...");
		strcpy(opt[j++],"File Directories...");
		opt[j][0]=0;
		sprintf(str,"%s Library",cfg.lib[i]->sname);
		uifc.helpbuf=
			"`File Library Configuration:`\n"
			"\n"
			"This menu allows you to configure the security requirments for access\n"
			"to this file library. You can also add, delete, and configure the\n"
			"directories of this library by selecting the `File Directories...` option.\n"
		;
		switch(uifc.list(WIN_ACT,6,4,60,&dflt,0,str,opt)) {
			case -1:
				done=1;
				break;
			case 0:
				uifc.helpbuf=
					"`Library Long Name:`\n"
					"\n"
					"This is a description of the file library which is displayed when a\n"
					"user of the system uses the `/*` command from the file transfer menu.\n"
				;
				strcpy(str,cfg.lib[i]->lname);	/* save */
				if(!uifc.input(WIN_MID|WIN_SAV,0,0,"Name to use for Listings"
					,cfg.lib[i]->lname,LEN_GLNAME,K_EDIT))
					strcpy(cfg.lib[i]->lname,str);	/* restore */
				break;
			case 1:
				uifc.helpbuf=
					"`Library Short Name:`\n"
					"\n"
					"This is a short description of the file librarly which is used for the\n"
					"file transfer menu prompt.\n"
				;
				uifc.input(WIN_MID|WIN_SAV,0,0,"Name to use for Prompts"
					,cfg.lib[i]->sname,LEN_GSNAME,K_EDIT);
				break;
			case 2:
				uifc.helpbuf=
					"`Internal Code Prefix:`\n"
					"\n"
					"This is an `optional` code prefix that may be used to help generate unique\n"
					"internal codes for the directories in this file library. If this option\n"
					"is used, directory internal codes will be constructed from this prefix and\n"
					"the specified code suffix for each directory.\n"
				;
				uifc.input(WIN_MID|WIN_SAV,0,17,"Internal Code Prefix"
					,cfg.lib[i]->code_prefix,LEN_CODE,K_EDIT|K_UPPER);
				break;
			case 3:
				uifc.helpbuf=
					"`Parent Directory:`\n"
					"\n"
					"This an optional path to be used as the physical \"parent\" directory for \n"
					"all logical directories in this library. This parent directory will be\n"
					"used in combination with each directory's storage path to create the\n"
					"full physical storage path for files in this directory.\n"
					"\n"
					"This option is convenient for adding libraries with many directories\n"
					"that share a common parent directory (e.g. CD-ROMs) and gives you the\n"
					"option of easily changing the common parent directory location later, if\n"
					"desired.\n"
				;
				uifc.input(WIN_MID|WIN_SAV,0,0,"Parent Directory"
					,cfg.lib[i]->parent_path,sizeof(cfg.lib[i]->parent_path)-1,K_EDIT);
				break;
			case 4:
				sprintf(str,"%s Library",cfg.lib[i]->sname);
				getar(str,cfg.lib[i]->arstr);
				break;
			case 5: /* clone options */
				j=0;
				strcpy(opt[0],"Yes");
				strcpy(opt[1],"No");
				opt[2][0]=0;
				uifc.helpbuf=
					"`Clone Directory Options:`\n"
					"\n"
					"If you want to clone the options of the first directory of this library\n"
					"into all directories of this library, select `Yes`.\n"
					"\n"
					"The options cloned are upload requirments, download requirments,\n"
					"operator requirements, exempted user requirements, toggle options,\n"
					"maximum number of files, allowed file extensions, default file\n"
					"extension, and sort type.\n"
				;
				j=uifc.list(WIN_MID|WIN_SAV,0,0,0,&j,0
					,"Clone Options of First Directory into All of Library"
					,opt);
				if(j==0) {
					k=-1;
					for(j=0;j<cfg.total_dirs;j++)
						if(cfg.dir[j]->lib==i) {
							if(k==-1)
								k=j;
							else {
								uifc.changes=1;
								cfg.dir[j]->misc=cfg.dir[k]->misc;
								strcpy(cfg.dir[j]->ul_arstr,cfg.dir[k]->ul_arstr);
								strcpy(cfg.dir[j]->dl_arstr,cfg.dir[k]->dl_arstr);
								strcpy(cfg.dir[j]->op_arstr,cfg.dir[k]->op_arstr);
								strcpy(cfg.dir[j]->ex_arstr,cfg.dir[k]->ex_arstr);
								strcpy(cfg.dir[j]->exts,cfg.dir[k]->exts);
								strcpy(cfg.dir[j]->data_dir,cfg.dir[k]->data_dir);
								strcpy(cfg.dir[j]->upload_sem,cfg.dir[k]->upload_sem);
								cfg.dir[j]->maxfiles=cfg.dir[k]->maxfiles;
								cfg.dir[j]->maxage=cfg.dir[k]->maxage;
								cfg.dir[j]->up_pct=cfg.dir[k]->up_pct;
								cfg.dir[j]->dn_pct=cfg.dir[k]->dn_pct;
								cfg.dir[j]->seqdev=cfg.dir[k]->seqdev;
								cfg.dir[j]->sort=cfg.dir[k]->sort; } } }
                break;
			case 6:
				k=0;
				ported=0;
				q=uifc.changes;
				strcpy(opt[k++],"DIRS.TXT    (Synchronet)");
				strcpy(opt[k++],"FILEBONE.NA (Fido)");
				opt[k][0]=0;
				uifc.helpbuf=
					"`Export Area File Format:`\n"
					"\n"
					"This menu allows you to choose the format of the area file you wish to\n"
					"export to.\n"
				;
				k=0;
				k=uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
					,"Export Area File Format",opt);
				if(k==-1)
					break;
				if(k==0)
					sprintf(str,"%sDIRS.TXT",cfg.ctrl_dir);
				else if(k==1)
					sprintf(str,"FILEBONE.NA");
				if(uifc.input(WIN_MID|WIN_SAV,0,0,"Filename"
					,str,sizeof(str)-1,K_EDIT)<=0) {
					uifc.changes=q;
					break; }
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
					else	 j=O_WRONLY|O_APPEND; }
				else
					j=O_WRONLY|O_CREAT;
				if((stream=fnopen(&file,str,j))==NULL) {
					uifc.msg("Open Failure");
					break; }
				uifc.pop("Exporting Areas...");
				for(j=0;j<cfg.total_dirs;j++) {
					if(cfg.dir[j]->lib!=i)
						continue;
					ported++;
					if(k==1) {
						fprintf(stream,"Area %-8s  0     !      %s\r\n"
							,cfg.dir[j]->code_suffix,cfg.dir[j]->lname);
						continue; }
					fprintf(stream,"%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n"
							"%s\r\n%s\r\n"
						,cfg.dir[j]->lname
						,cfg.dir[j]->sname
						,cfg.dir[j]->code_suffix
						,cfg.dir[j]->data_dir
						,cfg.dir[j]->arstr
						,cfg.dir[j]->ul_arstr
						,cfg.dir[j]->dl_arstr
						,cfg.dir[j]->op_arstr
						);
					fprintf(stream,"%s\r\n%s\r\n%u\r\n%s\r\n%"PRIX32"\r\n%u\r\n"
							"%u\r\n"
						,cfg.dir[j]->path
						,cfg.dir[j]->upload_sem
						,cfg.dir[j]->maxfiles
						,cfg.dir[j]->exts
						,cfg.dir[j]->misc
						,cfg.dir[j]->seqdev
						,cfg.dir[j]->sort
						);
					fprintf(stream,"%s\r\n%u\r\n%u\r\n%u\r\n"
						,cfg.dir[j]->ex_arstr
						,cfg.dir[j]->maxage
						,cfg.dir[j]->up_pct
						,cfg.dir[j]->dn_pct
						);
					fprintf(stream,"***END-OF-DIR***\r\n\r\n"); }
				fclose(stream);
				uifc.pop(0);
				sprintf(str,"%lu File Areas Exported Successfully",ported);
				uifc.msg(str);
				uifc.changes=q;
				break;

			case 7:
				ported=added=0;
				k=0;
				uifc.helpbuf=
					"`Import Area File Format:`\n"
					"\n"
					"This menu allows you to choose the format of the area file you wish to\n"
					"import into the current file library.\n"
					"\n"
					"A \"raw\" directory listing can be created in DOS with the following\n"
					"command: `DIR /ON /AD /B > DIRS.RAW`\n"
				;
				strcpy(opt[k++],"DIRS.TXT    (Synchronet)");
                strcpy(opt[k++],"FILEBONE.NA (Fido)");
				strcpy(opt[k++],"DIRS.RAW    (Raw)");
				strcpy(opt[k++],"Directory Listing...");
				opt[k][0]=0;
				k=0;
				k=uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
					,"Import Area File Format",opt);
				if(k==-1)
					break;
				if(k==0)
					sprintf(str,"%sDIRS.TXT",cfg.ctrl_dir);
				else if(k==1)
					sprintf(str,"FILEBONE.NA");
				else {
					strcpy(str,cfg.lib[i]->parent_path);
					backslash(str);
					strcat(str,"dirs.raw");
				}
				if(k==3) {
					if(!create_raw_dir_list(str))
						break;
				} else {
					if(uifc.input(WIN_MID|WIN_SAV,0,0,"Filename"
						,str,sizeof(str)-1,K_EDIT)<=0)
						break;
					if(k==2 && !fexistcase(str)) {
						strcpy(opt[0],"Yes");
						strcpy(opt[1],"No");
						opt[2][0]=0;
						j=0;
						if(uifc.list(WIN_MID|WIN_SAV,0,0,0,&j,0
							,"File doesn't exist, create it?",opt)==0)
							create_raw_dir_list(str);
					}
				}
				if(!fexistcase(str))
					break;
				if((stream=fnopen(&file,str,O_RDONLY))==NULL) {
					uifc.msg("Open Failure");
                    break; 
				}
				uifc.pop("Importing Areas...");
				while(!feof(stream)) {
					if(!fgets(str,sizeof(str),stream)) break;
					truncsp(str);
					if(!str[0])
						continue;
					memset(&tmpdir,0,sizeof(dir_t));
					tmpdir.misc=DEFAULT_DIR_OPTIONS;
					tmpdir.maxfiles=MAX_FILES;
					tmpdir.up_pct=cfg.cdt_up_pct;
					tmpdir.dn_pct=cfg.cdt_dn_pct; 

					p=str;
					while(*p && *p<=' ') p++;

					if(k>=2) { /* raw */
						SAFECOPY(tmp_code,p);
						SAFECOPY(tmpdir.lname,p);
						SAFECOPY(tmpdir.sname,p);
						SAFECOPY(tmpdir.path,p);
						ported++;
					}
					else if(k==1) {
						if(strnicmp(p,"AREA ",5))
							continue;
						p+=5;
						while(*p && *p<=' ') p++;
						SAFECOPY(tmp_code,p);
						truncstr(tmp_code," \t");
						while(*p>' ') p++;			/* Skip areaname */
						while(*p && *p<=' ') p++;	/* Skip space */
						while(*p>' ') p++;			/* Skip level */
						while(*p && *p<=' ') p++;	/* Skip space */
						while(*p>' ') p++;			/* Skip flags */
						while(*p && *p<=' ') p++;	/* Skip space */
						SAFECOPY(tmpdir.sname,p); 
						SAFECOPY(tmpdir.lname,p); 
						ported++;
					}
					else {
						sprintf(tmpdir.lname,"%.*s",LEN_SLNAME,str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						sprintf(tmpdir.sname,"%.*s",LEN_SSNAME,str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						SAFECOPY(tmp_code,str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						sprintf(tmpdir.data_dir,"%.*s",LEN_DIR,str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						sprintf(tmpdir.arstr,"%.*s",LEN_ARSTR,str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						sprintf(tmpdir.ul_arstr,"%.*s",LEN_ARSTR,str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						sprintf(tmpdir.dl_arstr,"%.*s",LEN_ARSTR,str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						sprintf(tmpdir.op_arstr,"%.*s",LEN_ARSTR,str);
						if(!fgets(str,sizeof(str),stream)) break;
                        truncsp(str);
                        sprintf(tmpdir.path,"%.*s",LEN_DIR,str);
						if(!fgets(str,sizeof(str),stream)) break;
                        truncsp(str);
                        sprintf(tmpdir.upload_sem,"%.*s",LEN_DIR,str);
						if(!fgets(str,sizeof(str),stream)) break;
                        truncsp(str);
						tmpdir.maxfiles=atoi(str);
						if(!fgets(str,sizeof(str),stream)) break;
                        truncsp(str);
						sprintf(tmpdir.exts,"%.*s",40,str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						tmpdir.misc=ahtoul(str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						tmpdir.seqdev=atoi(str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						tmpdir.sort=atoi(str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						sprintf(tmpdir.ex_arstr,"%.*s",LEN_ARSTR,str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						tmpdir.maxage=atoi(str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						tmpdir.up_pct=atoi(str);
						if(!fgets(str,sizeof(str),stream)) break;
						truncsp(str);
						tmpdir.dn_pct=atoi(str);

						ported++;
						while(!feof(stream)
							&& strcmp(str,"***END-OF-DIR***")) {
							if(!fgets(str,sizeof(str),stream)) break;
							truncsp(str); 
						} 
					}

					SAFECOPY(tmpdir.code_suffix, prep_code(tmp_code,cfg.lib[i]->code_prefix));

					for(j=0;j<cfg.total_dirs;j++) {
						if(cfg.dir[j]->lib!=i)
							continue;
						if(!stricmp(cfg.dir[j]->code_suffix,tmpdir.code_suffix))
							break; }
					if(j==cfg.total_dirs) {

						if((cfg.dir=(dir_t **)realloc(cfg.dir
							,sizeof(dir_t *)*(cfg.total_dirs+1)))==NULL) {
							errormsg(WHERE,ERR_ALLOC,"dir",cfg.total_dirs+1);
							cfg.total_dirs=0;
							bail(1);
							break; }

						if((cfg.dir[j]=(dir_t *)malloc(sizeof(dir_t)))
							==NULL) {
							errormsg(WHERE,ERR_ALLOC,"dir",sizeof(dir_t));
							break; }
						memset(cfg.dir[j],0,sizeof(dir_t)); 
						added++;
					}
					if(k==1) {
						SAFECOPY(cfg.dir[j]->code_suffix,tmpdir.code_suffix);
						SAFECOPY(cfg.dir[j]->sname,tmpdir.code_suffix);
						SAFECOPY(cfg.dir[j]->lname,tmpdir.lname);
						if(j==cfg.total_dirs) {
							cfg.dir[j]->maxfiles=MAX_FILES;
							cfg.dir[j]->up_pct=cfg.cdt_up_pct;
							cfg.dir[j]->dn_pct=cfg.cdt_dn_pct; 
						}
					} else
						memcpy(cfg.dir[j],&tmpdir,sizeof(dir_t));
					cfg.dir[j]->lib=i;
					if(j==cfg.total_dirs) {
						cfg.dir[j]->misc=tmpdir.misc;
						cfg.total_dirs++; 
					}
					uifc.changes=1; 
				}
				fclose(stream);
				uifc.pop(0);
				sprintf(str,"%lu File Areas Imported Successfully (%lu added)",ported, added);
                uifc.msg(str);
                break;

			case 8:
				dir_cfg(i);
				break; } } }

}

void dir_cfg(uint libnum)
{
	static int dflt,bar,tog_dflt,tog_bar,adv_dflt,opt_dflt;
	char str[128],str2[128],code[128],path[MAX_PATH+1],done=0;
	char data_dir[MAX_PATH+1];
	int j,n;
	uint i,dirnum[MAX_OPTS+1];
	static dir_t savdir;

while(1) {
	for(i=0,j=0;i<cfg.total_dirs && j<MAX_OPTS;i++)
		if(cfg.dir[i]->lib==libnum) {
			sprintf(opt[j],"%-25s",cfg.dir[i]->lname);
			dirnum[j++]=i; }
	dirnum[j]=cfg.total_dirs;
	opt[j][0]=0;
	sprintf(str,"%s Directories",cfg.lib[libnum]->sname);
	i=WIN_SAV|WIN_ACT;
	if(j)
		i|=WIN_DEL|WIN_GET|WIN_DELACT;
	if(j<MAX_OPTS)
		i|=WIN_INS|WIN_INSACT|WIN_XTR;
	if(savdir.sname[0])
		i|=WIN_PUT;
	uifc.helpbuf=
		"`File Directories:`\n"
		"\n"
		"This is a list of file directories that have been configured for the\n"
		"selected file library.\n"
		"\n"
		"To add a directory, select the desired position with the arrow keys and\n"
		"hit ~ INS ~.\n"
		"\n"
		"To delete a directory, select it with the arrow keys and hit ~ DEL ~.\n"
		"\n"
		"To configure a directory, select it with the arrow keys and hit ~ ENTER ~.\n"
	;
	i=uifc.list(i,24,1,45,&dflt,&bar,str,opt);
	if((signed)i==-1)
		return;
	if((i&MSK_ON)==MSK_INS) {
		i&=MSK_OFF;
		strcpy(str,"My Brand-New File Directory");
		uifc.helpbuf=
			"`Directory Long Name:`\n"
			"\n"
			"This is a description of the file directory which is displayed in all\n"
			"directory listings.\n"
		;
		if(uifc.input(WIN_MID|WIN_SAV,0,0,"Directory Long Name",str,LEN_SLNAME
			,K_EDIT)<1)
			continue;
		sprintf(str2,"%.*s",LEN_SSNAME,str);
		uifc.helpbuf=
			"`Directory Short Name:`\n"
			"\n"
			"This is a short description of the file directory which is displayed at\n"
			"the file transfer prompt.\n"
		;
		if(uifc.input(WIN_MID|WIN_SAV,0,0,"Directory Short Name",str2,LEN_SSNAME
			,K_EDIT)<1)
            continue;
		SAFECOPY(code,str2);
		prep_code(code,/* prefix: */NULL);
		uifc.helpbuf=
			"`Directory Internal Code Suffix:`\n"
			"\n"
			"Every directory must have its own unique code for Synchronet to refer to\n"
			"it internally. This code should be descriptive of the directory's\n"
			"contents, usually an abreviation of the directory's name.\n"
			"\n"
			"`Note:` The internal code is onstructed from the file library's code prefix\n"
			"(if present) and the directory's code suffix.\n"
		;
		if(uifc.input(WIN_MID|WIN_SAV,0,0,"Directory Internal Code Suffix",code,LEN_CODE
			,K_EDIT|K_UPPER)<1)
            continue;
		if(!code_ok(code)) {
			uifc.helpbuf=invalid_code;
			uifc.msg("Invalid Code");
			uifc.helpbuf=0;
			continue; }
		if(cfg.lib[libnum]->parent_path[0])
			SAFECOPY(path,code);
		else
			sprintf(path,"%sdirs/%s",cfg.data_dir,code);
		uifc.helpbuf=
			"`Directory File Path:`\n"
			"\n"
			"This is the drive and directory where your uploads to and downloads from\n"
			"this directory will be stored. Example: `C:\\XFER\\GAMES`\n"
		;
		if(uifc.input(WIN_MID|WIN_SAV,0,0,"Directory File Path",path,50
			,K_EDIT)<1)
			continue;
		if((cfg.dir=(dir_t **)realloc(cfg.dir,sizeof(dir_t *)*(cfg.total_dirs+1)))==NULL) {
			errormsg(WHERE,ERR_ALLOC,nulstr,cfg.total_dirs+1);
			cfg.total_dirs=0;
			bail(1);
            continue; }

		if(j)
			for(n=cfg.total_dirs;n>dirnum[i];n--)
                cfg.dir[n]=cfg.dir[n-1];
		if((cfg.dir[dirnum[i]]=(dir_t *)malloc(sizeof(dir_t)))==NULL) {
			errormsg(WHERE,ERR_ALLOC,nulstr,sizeof(dir_t));
			continue; }
		memset((dir_t *)cfg.dir[dirnum[i]],0,sizeof(dir_t));
		cfg.dir[dirnum[i]]->lib=libnum;
		cfg.dir[dirnum[i]]->maxfiles=MAX_FILES;
		if(stricmp(str2,"OFFLINE"))
			cfg.dir[dirnum[i]]->misc=DEFAULT_DIR_OPTIONS;
		strcpy(cfg.dir[dirnum[i]]->code_suffix,code);
		strcpy(cfg.dir[dirnum[i]]->lname,str);
		strcpy(cfg.dir[dirnum[i]]->sname,str2);
		strcpy(cfg.dir[dirnum[i]]->path,path);
		cfg.dir[dirnum[i]]->up_pct=cfg.cdt_up_pct;
		cfg.dir[dirnum[i]]->dn_pct=cfg.cdt_dn_pct;
		cfg.total_dirs++;
		uifc.changes=1;
		continue; }
	if((i&MSK_ON)==MSK_DEL) {
		i&=MSK_OFF;
		uifc.helpbuf=
			"`Delete Directory Data Files:`\n"
			"\n"
			"If you want to delete all the database files for this directory,\n"
			"select `Yes`.\n"
		;
		j=1;
		strcpy(opt[0],"Yes");
		strcpy(opt[1],"No");
		opt[2][0]=0;
		SAFEPRINTF2(str,"%s%s.*"
			,cfg.lib[cfg.dir[dirnum[i]]->lib]->code_prefix
			,cfg.dir[dirnum[i]]->code_suffix);
		strlwr(str);
		if(!cfg.dir[dirnum[i]]->data_dir[0])
			SAFEPRINTF(data_dir,"%sdirs/",cfg.data_dir);
		else
			SAFECOPY(data_dir,cfg.dir[dirnum[i]]->data_dir);
		SAFEPRINTF2(path,"%s%s", data_dir, str);
		if(fexist(path)) {
			SAFEPRINTF(str2,"Delete %s",path);
			j=uifc.list(WIN_MID|WIN_SAV,0,0,0,&j,0
				,str2,opt);
			if(j==-1)
				continue;
			if(j==0)
					delfiles(data_dir,str); 
		}
		free(cfg.dir[dirnum[i]]);
		cfg.total_dirs--;
		for(j=dirnum[i];j<cfg.total_dirs;j++)
			cfg.dir[j]=cfg.dir[j+1];
		uifc.changes=1;
		continue; }
	if((i&MSK_ON)==MSK_GET) {
		i&=MSK_OFF;
		savdir=*cfg.dir[dirnum[i]];
		continue; }
	if((i&MSK_ON)==MSK_PUT) {
		i&=MSK_OFF;
		*cfg.dir[dirnum[i]]=savdir;
		cfg.dir[dirnum[i]]->lib=libnum;
		uifc.changes=1;
        continue; }
	i=dirnum[dflt];
	j=0;
	done=0;
	while(!done) {
		n=0;
		sprintf(opt[n++],"%-27.27s%s","Long Name",cfg.dir[i]->lname);
		sprintf(opt[n++],"%-27.27s%s","Short Name",cfg.dir[i]->sname);
		sprintf(opt[n++],"%-27.27s%s%s","Internal Code"
			,cfg.lib[cfg.dir[i]->lib]->code_prefix, cfg.dir[i]->code_suffix);
		sprintf(opt[n++],"%-27.27s%.40s","Access Requirements"
			,cfg.dir[i]->arstr);
		sprintf(opt[n++],"%-27.27s%.40s","Upload Requirements"
			,cfg.dir[i]->ul_arstr);
		sprintf(opt[n++],"%-27.27s%.40s","Download Requirements"
			,cfg.dir[i]->dl_arstr);
		sprintf(opt[n++],"%-27.27s%.40s","Operator Requirements"
            ,cfg.dir[i]->op_arstr);
		sprintf(opt[n++],"%-27.27s%.40s","Exemption Requirements"
			,cfg.dir[i]->ex_arstr);
		sprintf(opt[n++],"%-27.27s%.40s","Transfer File Path"
			,cfg.dir[i]->path);
		sprintf(opt[n++],"%-27.27s%u","Maximum Number of Files"
			,cfg.dir[i]->maxfiles);
		if(cfg.dir[i]->maxage)
			sprintf(str,"Enabled (%u days old)",cfg.dir[i]->maxage);
        else
            strcpy(str,"Disabled");
        sprintf(opt[n++],"%-27.27s%s","Purge by Age",str);
		sprintf(opt[n++],"%-27.27s%u%%","Credit on Upload"
			,cfg.dir[i]->up_pct);
		sprintf(opt[n++],"%-27.27s%u%%","Credit on Download"
			,cfg.dir[i]->dn_pct);
		strcpy(opt[n++],"Toggle Options...");
		strcpy(opt[n++],"Advanced Options...");
		opt[n][0]=0;
		sprintf(str,"%s Directory",cfg.dir[i]->sname);
		uifc.helpbuf=
			"`Directory Configuration:`\n"
			"\n"
			"This menu allows you to configure the individual selected directory.\n"
			"Options with a trailing `...` provide a sub-menu of more options.\n"
		;
		switch(uifc.list(WIN_SAV|WIN_ACT|WIN_RHT|WIN_BOT
			,0,0,60,&opt_dflt,0,str,opt)) {
			case -1:
				done=1;
				break;
			case 0:
				uifc.helpbuf=
					"`Directory Long Name:`\n"
					"\n"
					"This is a description of the file directory which is displayed in all\n"
					"directory listings.\n"
				;
				strcpy(str,cfg.dir[i]->lname);	/* save */
				if(!uifc.input(WIN_L2R|WIN_SAV,0,17,"Name to use for Listings"
					,cfg.dir[i]->lname,LEN_SLNAME,K_EDIT))
					strcpy(cfg.dir[i]->lname,str);
				break;
			case 1:
				uifc.helpbuf=
					"`Directory Short Name:`\n"
					"\n"
					"This is a short description of the file directory which is displayed at\n"
					"the file transfer prompt.\n"
				;
				uifc.input(WIN_L2R|WIN_SAV,0,17,"Name to use for Prompts"
					,cfg.dir[i]->sname,LEN_SSNAME,K_EDIT);
				break;
			case 2:
                uifc.helpbuf=
	                "`Directory Internal Code Suffix:`\n"
	                "\n"
	                "Every directory must have its own unique code for Synchronet to refer to\n"
	                "it internally. This code should be descriptive of the directory's\n"
	                "contents, usually an abreviation of the directory's name.\n"
	                "\n"
	                "`Note:` The internal code displayed is the complete internal code\n"
	                "constructed from the file library's code prefix and the directory's code\n"
	                "suffix.\n"
                ;
                strcpy(str,cfg.dir[i]->code_suffix);
                uifc.input(WIN_L2R|WIN_SAV,0,17,"Internal Code Suffix (unique)"
                    ,str,LEN_CODE,K_EDIT|K_UPPER);
                if(code_ok(str))
                    strcpy(cfg.dir[i]->code_suffix,str);
                else {
                    uifc.helpbuf=invalid_code;
                    uifc.msg("Invalid Code");
                    uifc.helpbuf=0; 
				}
                break;
			case 3:
				sprintf(str,"%s Access",cfg.dir[i]->sname);
				getar(str,cfg.dir[i]->arstr);
				break;
			case 4:
				sprintf(str,"%s Upload",cfg.dir[i]->sname);
				getar(str,cfg.dir[i]->ul_arstr);
				break;
			case 5:
				sprintf(str,"%s Download",cfg.dir[i]->sname);
				getar(str,cfg.dir[i]->dl_arstr);
				break;
			case 6:
				sprintf(str,"%s Operator",cfg.dir[i]->sname);
				getar(str,cfg.dir[i]->op_arstr);
                break;
			case 7:
				sprintf(str,"%s Exemption",cfg.dir[i]->sname);
				getar(str,cfg.dir[i]->ex_arstr);
                break;
			case 8:
				uifc.helpbuf=
					"`File Path:`\n"
					"\n"
					"This is the default storage path for files uploaded to this directory.\n"
					"If this path is blank, files are stored in a directory off of the\n"
					"DATA\\DIRS directory using the internal code of this directory as the\n"
					"name of the dirdirectory (i.e. DATA\\DIRS\\<CODE>).\n"
					"\n"
					"This path can be overridden on a per file basis using `Alternate File\n"
					"Paths`.\n"
				;
				uifc.input(WIN_L2R|WIN_SAV,0,17,"File Path"
					,cfg.dir[i]->path,sizeof(cfg.dir[i]->path)-1,K_EDIT);
				break;
			case 9:
				uifc.helpbuf=
					"`Maximum Number of Files:`\n"
					"\n"
					"This value is the maximum number of files allowed in this directory.\n"
				;
				sprintf(str,"%u",cfg.dir[i]->maxfiles);
				uifc.input(WIN_L2R|WIN_SAV,0,17,"Maximum Number of Files"
					,str,5,K_EDIT|K_NUMBER);
				n=atoi(str);
				if(n>MAX_FILES) {
					sprintf(str,"Maximum Files is %u",MAX_FILES);
					uifc.msg(str); }
				else
					cfg.dir[i]->maxfiles=n;
				break;
			case 10:
				sprintf(str,"%u",cfg.dir[i]->maxage);
                uifc.helpbuf=
	                "`Maximum Age of Files:`\n"
	                "\n"
	                "This value is the maximum number of days that files will be kept in\n"
	                "the directory based on the date the file was uploaded or last\n"
	                "downloaded (If the `Purge by Last Download` toggle option is used).\n"
	                "\n"
	                "The Synchronet file base maintenance program (`DELFILES`) must be used\n"
	                "to automatically remove files based on age.\n"
                ;
				uifc.input(WIN_MID|WIN_SAV,0,17,"Maximum Age of Files (in days)"
                    ,str,5,K_EDIT|K_NUMBER);
				cfg.dir[i]->maxage=atoi(str);
                break;
			case 11:
uifc.helpbuf=
	"`Percentage of Credits to Credit Uploader on Upload:`\n"
	"\n"
	"This is the percentage of a file's credit value that is given to users\n"
	"when they upload files. Most often, this value will be set to `100` to\n"
	"give full credit value (100%) for uploads.\n"
	"\n"
	"If you want uploaders to receive no credits upon upload, set this value\n"
	"to `0`.\n"
;
				uifc.input(WIN_MID|WIN_SAV,0,0
					,"Percentage of Credits to Credit Uploader on Upload"
					,ultoa(cfg.dir[i]->up_pct,tmp,10),4,K_EDIT|K_NUMBER);
				cfg.dir[i]->up_pct=atoi(tmp);
				break;
			case 12:
uifc.helpbuf=
	"`Percentage of Credits to Credit Uploader on Download:`\n"
	"\n"
	"This is the percentage of a file's credit value that is given to users\n"
	"who upload a file that is later downloaded by another user. This is an\n"
	"award type system where more popular files will generate more credits\n"
	"for the uploader.\n"
	"\n"
	"If you do not want uploaders to receive credit when files they upload\n"
	"are later downloaded, set this value to `0`.\n"
;
				uifc.input(WIN_MID|WIN_SAV,0,0
					,"Percentage of Credits to Credit Uploader on Download"
					,ultoa(cfg.dir[i]->dn_pct,tmp,10),4,K_EDIT|K_NUMBER);
				cfg.dir[i]->dn_pct=atoi(tmp);
				break;
			case 13:
				while(1) {
					n=0;
					sprintf(opt[n++],"%-27.27s%s","Check for File Existence"
						,cfg.dir[i]->misc&DIR_FCHK ? "Yes":"No");
					strcpy(str,"Slow Media Device");
					if(cfg.dir[i]->seqdev) {
						sprintf(tmp," #%u",cfg.dir[i]->seqdev);
						strcat(str,tmp); }
					sprintf(opt[n++],"%-27.27s%s",str
						,cfg.dir[i]->seqdev ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Force Content Ratings"
						,cfg.dir[i]->misc&DIR_RATE ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Upload Date in Listings"
						,cfg.dir[i]->misc&DIR_ULDATE ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Multiple File Numberings"
						,cfg.dir[i]->misc&DIR_MULT ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Search for Duplicates"
						,cfg.dir[i]->misc&DIR_DUPES ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Search for New Files"
						,cfg.dir[i]->misc&DIR_NOSCAN ? "No":"Yes");
					sprintf(opt[n++],"%-27.27s%s","Search for Auto-ADDFILES"
						,cfg.dir[i]->misc&DIR_NOAUTO ? "No":"Yes");
					sprintf(opt[n++],"%-27.27s%s","Import FILE_ID.DIZ"
						,cfg.dir[i]->misc&DIR_DIZ ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Free Downloads"
						,cfg.dir[i]->misc&DIR_FREE ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Free Download Time"
						,cfg.dir[i]->misc&DIR_TFREE ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Deduct Upload Time"
						,cfg.dir[i]->misc&DIR_ULTIME ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Credit Uploads"
						,cfg.dir[i]->misc&DIR_CDTUL ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Credit Downloads"
						,cfg.dir[i]->misc&DIR_CDTDL ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Credit with Minutes"
						,cfg.dir[i]->misc&DIR_CDTMIN ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Download Notifications"
						,cfg.dir[i]->misc&DIR_QUIET ? "No":"Yes");
					sprintf(opt[n++],"%-27.27s%s","Anonymous Uploads"
						,cfg.dir[i]->misc&DIR_ANON ? cfg.dir[i]->misc&DIR_AONLY
						? "Only":"Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Purge by Last Download"
						,cfg.dir[i]->misc&DIR_SINCEDL ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Mark Moved Files as New"
						,cfg.dir[i]->misc&DIR_MOVENEW ? "Yes":"No");
					sprintf(opt[n++],"%-27.27s%s","Include Transfers In Stats"
						,cfg.dir[i]->misc&DIR_NOSTAT ? "No":"Yes");
					opt[n][0]=0;
					uifc.helpbuf=
						"`Directory Toggle Options:`\n"
						"\n"
						"This is the toggle options menu for the selected file directory.\n"
						"\n"
						"The available options from this menu can all be toggled between two or\n"
						"more states, such as `Yes` and `No`.\n"
					;
					n=uifc.list(WIN_ACT|WIN_SAV|WIN_RHT|WIN_BOT,3,2,36,&tog_dflt
						,&tog_bar,"Toggle Options",opt);
					if(n==-1)
                        break;
					switch(n) {
						case 0:
							n=cfg.dir[i]->misc&DIR_FCHK ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Check for File Existence When Listing:`\n"
								"\n"
								"If you want the actual existence of files to be verified while listing\n"
								"directories, set this value to `Yes`.\n"
								"\n"
								"Directories with files located on CD-ROM or other slow media should have\n"
								"this option set to `No`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Check for File Existence When Listing",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_FCHK)) {
								cfg.dir[i]->misc|=DIR_FCHK;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_FCHK)) {
								cfg.dir[i]->misc&=~DIR_FCHK;
								uifc.changes=1; }
							break;
						case 1:
                            n=cfg.dir[i]->seqdev ? 0:1;
                            strcpy(opt[0],"Yes");
                            strcpy(opt[1],"No");
                            opt[2][0]=0;
							uifc.helpbuf=
								"`Slow Media Device:`\n"
								"\n"
								"If this directory contains files located on CD-ROM or other slow media\n"
								"device, you should set this option to `Yes`. Each slow media device on\n"
								"your system should have a unique `Device Number`. If you only have one\n"
								"slow media device, then this number should be set to `1`.\n"
								"\n"
								"`CD-ROM multidisk changers` are considered `one` device and all the\n"
								"directories on all the CD-ROMs in each changer should be set to the same\n"
								"device number.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Slow Media Device"
								,opt);
							if(n==0) {
								if(!cfg.dir[i]->seqdev) {
									uifc.changes=1;
									strcpy(str,"1"); }
								else
									sprintf(str,"%u",cfg.dir[i]->seqdev);
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Device Number"
									,str,2,K_EDIT|K_UPPER);
								cfg.dir[i]->seqdev=atoi(str); }
							else if(n==1 && cfg.dir[i]->seqdev) {
								cfg.dir[i]->seqdev=0;
                                uifc.changes=1; }
                            break;
						case 2:
							n=cfg.dir[i]->misc&DIR_RATE ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Force Content Ratings in Descriptions:`\n"
								"\n"
								"If you would like all uploads to this directory to be prompted for\n"
								"content rating (G, R, or X), set this value to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Force Content Ratings in Descriptions",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_RATE)) {
								cfg.dir[i]->misc|=DIR_RATE;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_RATE)) {
								cfg.dir[i]->misc&=~DIR_RATE;
								uifc.changes=1; }
							break;
						case 3:
							n=cfg.dir[i]->misc&DIR_ULDATE ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Include Upload Date in File Descriptions:`\n"
								"\n"
								"If you wish the upload date of each file in this directory to be\n"
								"automatically included in the file description, set this option to\n"
								"`Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Include Upload Date in Descriptions",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_ULDATE)) {
								cfg.dir[i]->misc|=DIR_ULDATE;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_ULDATE)) {
								cfg.dir[i]->misc&=~DIR_ULDATE;
								uifc.changes=1; }
                            break;
						case 4:
							n=cfg.dir[i]->misc&DIR_MULT ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Ask for Multiple File Numberings:`\n"
								"\n"
								"If you would like uploads to this directory to be prompted for multiple\n"
								"file (disk) numbers, set this value to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Ask for Multiple File Numberings",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_MULT)) {
								cfg.dir[i]->misc|=DIR_MULT;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_MULT)) {
								cfg.dir[i]->misc&=~DIR_MULT;
								uifc.changes=1; }
							break;
						case 5:
							n=cfg.dir[i]->misc&DIR_DUPES ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Search Directory for Duplicate Filenames:`\n"
								"\n"
								"If you would like to have this directory searched for duplicate\n"
								"filenames when a user attempts to upload a file, set this option to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Search for Duplicate Filenames",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_DUPES)) {
								cfg.dir[i]->misc|=DIR_DUPES;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_DUPES)) {
								cfg.dir[i]->misc&=~DIR_DUPES;
								uifc.changes=1; }
							break;
						case 6:
							n=cfg.dir[i]->misc&DIR_NOSCAN ? 1:0;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Search Directory for New Files:`\n"
								"\n"
								"If you would like to have this directory searched for newly uploaded\n"
								"files when a user scans `All` libraries for new files, set this option to\n"
								"`Yes`.\n"
								"\n"
								"If this directory is located on `CD-ROM` or other read only media\n"
								"(where uploads are unlikely to occur), it will improve new file scans\n"
								"if this option is set to `No`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Search for New files",opt);
							if(n==0 && cfg.dir[i]->misc&DIR_NOSCAN) {
								cfg.dir[i]->misc&=~DIR_NOSCAN;
								uifc.changes=1; }
							else if(n==1 && !(cfg.dir[i]->misc&DIR_NOSCAN)) {
								cfg.dir[i]->misc|=DIR_NOSCAN;
								uifc.changes=1; }
                            break;
						case 7:
							n=cfg.dir[i]->misc&DIR_NOAUTO ? 1:0;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Search Directory for Auto-ADDFILES:`\n"
								"\n"
								"If you would like to have this directory searched for a file list to\n"
								"import automatically when using the `ADDFILES *` (Auto-ADD) feature,\n"
								"set this option to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Search for Auto-ADDFILES",opt);
							if(n==0 && cfg.dir[i]->misc&DIR_NOAUTO) {
								cfg.dir[i]->misc&=~DIR_NOAUTO;
								uifc.changes=1; }
							else if(n==1 && !(cfg.dir[i]->misc&DIR_NOAUTO)) {
								cfg.dir[i]->misc|=DIR_NOAUTO;
								uifc.changes=1; }
                            break;
						case 8:
							n=cfg.dir[i]->misc&DIR_DIZ ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Import FILE_ID.DIZ and DESC.SDI Descriptions:`\n"
								"\n"
								"If you would like archived descriptions (`FILE_ID.DIZ` and `DESC.SDI`)\n"
								"of uploaded files to be automatically imported as the extended\n"
								"description, set this option to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Import FILE_ID.DIZ and DESC.SDI",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_DIZ)) {
								cfg.dir[i]->misc|=DIR_DIZ;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_DIZ)) {
								cfg.dir[i]->misc&=~DIR_DIZ;
								uifc.changes=1; }
                            break;
						case 9:
							n=cfg.dir[i]->misc&DIR_FREE ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Downloads are Free:`\n"
								"\n"
								"If you would like all downloads from this directory to be free (cost\n"
								"no credits), set this option to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Downloads are Free",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_FREE)) {
								cfg.dir[i]->misc|=DIR_FREE;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_FREE)) {
								cfg.dir[i]->misc&=~DIR_FREE;
								uifc.changes=1; }
                            break;
						case 10:
							n=cfg.dir[i]->misc&DIR_TFREE ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Free Download Time:`\n"
								"\n"
								"If you would like all downloads from this directory to not subtract\n"
								"time from the user, set this option to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Free Download Time",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_TFREE)) {
								cfg.dir[i]->misc|=DIR_TFREE;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_TFREE)) {
								cfg.dir[i]->misc&=~DIR_TFREE;
								uifc.changes=1; }
                            break;
						case 11:
							n=cfg.dir[i]->misc&DIR_ULTIME ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Deduct Upload Time:`\n"
								"\n"
								"If you would like all uploads to this directory to have the time spent\n"
								"uploading subtracted from their time online, set this option to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Deduct Upload Time",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_ULTIME)) {
								cfg.dir[i]->misc|=DIR_ULTIME;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_ULTIME)) {
								cfg.dir[i]->misc&=~DIR_ULTIME;
								uifc.changes=1; }
                            break;
						case 12:
							n=cfg.dir[i]->misc&DIR_CDTUL ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Give Credit for Uploads:`\n"
								"\n"
								"If you want users who upload to this directory to get credit for their\n"
								"initial upload, set this option to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Give Credit for Uploads",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_CDTUL)) {
								cfg.dir[i]->misc|=DIR_CDTUL;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_CDTUL)) {
								cfg.dir[i]->misc&=~DIR_CDTUL;
								uifc.changes=1; }
                            break;
						case 13:
							n=cfg.dir[i]->misc&DIR_CDTDL ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Give Uploader Credit for Downloads:`\n"
								"\n"
								"If you want users who upload to this directory to get credit when their\n"
								"files are downloaded, set this optin to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Give Uploader Credit for Downloads",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_CDTDL)) {
								cfg.dir[i]->misc|=DIR_CDTDL;
								uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_CDTDL)) {
								cfg.dir[i]->misc&=~DIR_CDTDL;
								uifc.changes=1; }
                            break;
						case 14:
							n=cfg.dir[i]->misc&DIR_CDTMIN ? 0:1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Credit Uploader with Minutes instead of Credits:`\n"
								"\n"
								"If you wish to give the uploader of files to this directory minutes,\n"
								"intead of credits, set this option to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Credit Uploader with Minutes",opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_CDTMIN)) {
								cfg.dir[i]->misc|=DIR_CDTMIN;
								uifc.changes=1; }
							else if(n==1 && cfg.dir[i]->misc&DIR_CDTMIN){
								cfg.dir[i]->misc&=~DIR_CDTMIN;
								uifc.changes=1; }
                            break;
						case 15:
							n=cfg.dir[i]->misc&DIR_QUIET ? 1:0;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							opt[2][0]=0;
							uifc.helpbuf=
								"`Send Download Notifications:`\n"
								"\n"
								"If you wish the BBS to send download notification messages to the\n"
								"uploader of a file to this directory, set this option to `Yes`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Send Download Notifications",opt);
							if(n==1 && !(cfg.dir[i]->misc&DIR_QUIET)) {
								cfg.dir[i]->misc|=DIR_QUIET;
								uifc.changes=1; 
							} else if(n==0 && cfg.dir[i]->misc&DIR_QUIET){
								cfg.dir[i]->misc&=~DIR_QUIET;
								uifc.changes=1; 
							}
                            break;


						case 16:
							n=cfg.dir[i]->misc&DIR_ANON ? (cfg.dir[i]->misc&DIR_AONLY ? 2:0):1;
							strcpy(opt[0],"Yes");
							strcpy(opt[1],"No");
							strcpy(opt[2],"Only");
							opt[3][0]=0;
							uifc.helpbuf=
								"`Allow Anonymous Uploads:`\n"
								"\n"
								"If you want users with the `A` exemption to be able to upload anonymously\n"
								"to this directory, set this option to `Yes`. If you want all uploads to\n"
								"this directory to be forced anonymous, set this option to `Only`.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Allow Anonymous Uploads",opt);
							if(n==0 && (cfg.dir[i]->misc&(DIR_ANON|DIR_AONLY))
								!=DIR_ANON) {
								cfg.dir[i]->misc|=DIR_ANON;
								cfg.dir[i]->misc&=~DIR_AONLY;
								uifc.changes=1; }
							else if(n==1 && cfg.dir[i]->misc&(DIR_ANON|DIR_AONLY)){
								cfg.dir[i]->misc&=~(DIR_ANON|DIR_AONLY);
								uifc.changes=1; }
							else if(n==2 && (cfg.dir[i]->misc&(DIR_ANON|DIR_AONLY))
								!=(DIR_ANON|DIR_AONLY)) {
								cfg.dir[i]->misc|=(DIR_ANON|DIR_AONLY);
								uifc.changes=1; }
							break;
						case 17:
                            n=cfg.dir[i]->misc&DIR_SINCEDL ? 0:1;
                            strcpy(opt[0],"Yes");
                            strcpy(opt[1],"No");
                            opt[2][0]=0;
                            uifc.helpbuf=
	                            "`Purge Files Based on Date of Last Download:`\n"
	                            "\n"
	                            "Using the Synchronet file base maintenance utility (`DELFILES`), you can\n"
	                            "have files removed based on the number of days since last downloaded\n"
	                            "rather than the number of days since the file was uploaded (default),\n"
	                            "by setting this option to `Yes`.\n"
                            ;
                            n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Purge Files Based on Date of Last Download"
                                ,opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_SINCEDL)) {
								cfg.dir[i]->misc|=DIR_SINCEDL;
                                uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_SINCEDL)) {
								cfg.dir[i]->misc&=~DIR_SINCEDL;
                                uifc.changes=1; }
                            break;
						case 18:
                            n=cfg.dir[i]->misc&DIR_MOVENEW ? 0:1;
                            strcpy(opt[0],"Yes");
                            strcpy(opt[1],"No");
                            opt[2][0]=0;
                            uifc.helpbuf=
	                            "`Mark Moved Files as New:`\n"
	                            "\n"
	                            "If this option is set to `Yes`, then all files moved `from` this directory\n"
	                            "will have their upload date changed to the current date so the file will\n"
	                            "appear in users' new-file scans again.\n"
                            ;
                            n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Mark Moved Files as New"
                                ,opt);
							if(n==0 && !(cfg.dir[i]->misc&DIR_MOVENEW)) {
								cfg.dir[i]->misc|=DIR_MOVENEW;
                                uifc.changes=1; }
							else if(n==1 && (cfg.dir[i]->misc&DIR_MOVENEW)) {
								cfg.dir[i]->misc&=~DIR_MOVENEW;
                                uifc.changes=1; }
                            break;
						case 19:
                            n=cfg.dir[i]->misc&DIR_NOSTAT ? 1:0;
                            strcpy(opt[0],"Yes");
                            strcpy(opt[1],"No");
                            opt[2][0]=0;
                            uifc.helpbuf=
	                            "`Include Transfers In System Statistics:`\n"
	                            "\n"
	                            "If this option is set to ~Yes~, then all files uploaded to or downloaded\n"
	                            "from this directory will be included in the system's daily and\n"
	                            "cumulative statistics.\n"
                            ;
                            n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Include Transfers In System Statistics"
                                ,opt);
							if(n==1 && !(cfg.dir[i]->misc&DIR_NOSTAT)) {
								cfg.dir[i]->misc|=DIR_NOSTAT;
								uifc.changes=1; 
							} else if(n==0 && cfg.dir[i]->misc&DIR_NOSTAT){
								cfg.dir[i]->misc&=~DIR_NOSTAT;
								uifc.changes=1; 
							}
                            break;
					} 
				}
				break;
		case 14:
			while(1) {
				n=0;
				sprintf(opt[n++],"%-27.27s%s","Extensions Allowed"
					,cfg.dir[i]->exts);
				if(!cfg.dir[i]->data_dir[0])
					sprintf(str,"%sdirs/",cfg.data_dir);
				else
					strcpy(str,cfg.dir[i]->data_dir);
				sprintf(opt[n++],"%-27.27s%.40s","Data Directory"
					,str);
				sprintf(opt[n++],"%-27.27s%.40s","Upload Semaphore File"
                    ,cfg.dir[i]->upload_sem);
				sprintf(opt[n++],"%-27.27s%s","Sort Value and Direction"
					, cfg.dir[i]->sort==SORT_NAME_A ? "Name Ascending"
					: cfg.dir[i]->sort==SORT_NAME_D ? "Name Descending"
					: cfg.dir[i]->sort==SORT_DATE_A ? "Date Ascending"
					: "Date Descending");
				opt[n][0]=0;
				uifc.helpbuf=
					"`Directory Advanced Options:`\n"
					"\n"
					"This is the advanced options menu for the selected file directory.\n"
				;
					n=uifc.list(WIN_ACT|WIN_SAV|WIN_RHT|WIN_BOT,3,4,60,&adv_dflt,0
						,"Advanced Options",opt);
					if(n==-1)
                        break;
                    switch(n) {
						case 0:
							uifc.helpbuf=
								"`File Extensions Allowed:`\n"
								"\n"
								"This option allows you to limit the types of files uploaded to this\n"
								"directory. This is a list of file extensions that are allowed, each\n"
								"separated by a comma (Example: `ZIP,EXE`). If this option is left\n"
								"blank, all file extensions will be allowed to be uploaded.\n"
							;
							uifc.input(WIN_L2R|WIN_SAV,0,17
								,"File Extensions Allowed"
								,cfg.dir[i]->exts,sizeof(cfg.dir[i]->exts)-1,K_EDIT);
							break;
						case 1:
uifc.helpbuf=
	"`Data Directory:`\n"
	"\n"
	"Use this if you wish to place the data directory for this directory\n"
	"on another drive or in another directory besides the default setting.\n"
;
							uifc.input(WIN_MID|WIN_SAV,0,17,"Data"
								,cfg.dir[i]->data_dir,sizeof(cfg.dir[i]->data_dir)-1,K_EDIT);
							break;
						case 2:
uifc.helpbuf=
	"`Upload Semaphore File:`\n"
	"\n"
	"This is a filename that will be used as a semaphore (signal) to your\n"
	"FidoNet front-end that new files are ready to be hatched for export.\n"
;
							uifc.input(WIN_MID|WIN_SAV,0,17,"Upload Semaphore"
								,cfg.dir[i]->upload_sem,sizeof(cfg.dir[i]->upload_sem)-1,K_EDIT);
							break;
						case 3:
							n=0;
							strcpy(opt[0],"Name Ascending");
							strcpy(opt[1],"Name Descending");
							strcpy(opt[2],"Date Ascending");
							strcpy(opt[3],"Date Descending");
							opt[4][0]=0;
							uifc.helpbuf=
								"`Sort Value and Direction:`\n"
								"\n"
								"This option allows you to determine the sort value and direction. Files\n"
								"that are uploaded are automatically sorted by filename or upload date,\n"
								"ascending or descending. If you change the sort value or direction after\n"
								"a directory already has files in it, use the sysop transfer menu `;RESORT`\n"
								"command to resort the directory with the new sort parameters.\n"
							;
							n=uifc.list(WIN_MID|WIN_SAV,0,0,0,&n,0
								,"Sort Value and Direction",opt);
							if(n==0 && cfg.dir[i]->sort!=SORT_NAME_A) {
								cfg.dir[i]->sort=SORT_NAME_A;
								uifc.changes=1; }
							else if(n==1 && cfg.dir[i]->sort!=SORT_NAME_D) {
								cfg.dir[i]->sort=SORT_NAME_D;
								uifc.changes=1; }
							else if(n==2 && cfg.dir[i]->sort!=SORT_DATE_A) {
								cfg.dir[i]->sort=SORT_DATE_A;
								uifc.changes=1; }
							else if(n==3 && cfg.dir[i]->sort!=SORT_DATE_D) {
								cfg.dir[i]->sort=SORT_DATE_D;
								uifc.changes=1; }
							break; 
					} 
				}
			break;
			} 
		} 
	}

}
