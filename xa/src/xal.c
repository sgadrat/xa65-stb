/* xa65 - 65xx/65816 cross-assembler and utility suite
 *
 * Copyright (C) 1989-1997 Andr� Fachat (a.fachat@physik.tu-chemnitz.de)
 *
 * Label management module (also see xau.c)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* structs and defs */

#include "xad.h"
#include "xah.h"
#include "xar.h"
#include "xah2.h"
#include "xap.h"
#include "xa.h"

/* externals */

#include "xam.h"
#include "xal.h"

/* exported globals */

char   *lz;

/* local prototypes */

static int b_fget(int*,int);
static int b_ltest(int,int);
static int b_get(int*);
static int b_test(int);
static int ll_def(char *s, int *n, int b);     

static int b_new(void);
static void cll_init();
static int cll_get();
static void cll_clear();
static int cll_getcur();

/* local variables */

/*
static int hashindex[256];
static Labtab *lt = NULL;
static int    lti = 0;
static int    ltm = 0;
*/

/*
static char   *ln;
static unsigned long lni;
static long   sl;
*/

static Labtab *ltp;

int l_init(void)
{
	cll_init();
	return 0;
#if 0
     int er;

     for(er=0;er<256;er++)
          hashindex[er]=0;

     /*sl=(long)sizeof(Labtab);*/
     
/*     if(!(er=m_alloc((long)(sizeof(Labtab)*ANZLAB),(char**)&lt)))
          er=m_alloc((long)LABMEM,&ln);*/

     er=m_alloc((long)(sizeof(Labtab)*ANZLAB),(char**)&lt);
          
     lti=0;
/*     lni=0L;*/

     return(er);
#endif
}

int ga_lab(void)
{
	return(afile->la.lti);
}

int gm_lab(void)
{
	return(ANZLAB);
}

long gm_labm(void)
{
	return((long)LABMEM);
}

long ga_labm(void)
{
	return(0 /*lni*/);
}

void printllist(fp)
FILE *fp;
{
     int i;
     LabOcc *p;
     char *fname = NULL;

     for(i=0;i<afile->la.lti;i++)
     {
         ltp=afile->la.lt+i;
         fprintf(fp,"%s, 0x%04x, %d, 0x%04x\n",ltp->n,ltp->val,ltp->blk, 
							ltp->afl);
	 p = ltp->occlist;
	 if(p) {
	   while(p) {
	     if(fname != p->fname) {
		if(p!=ltp->occlist) fprintf(fp,"\n");
		fprintf(fp,"    %s",p->fname);
		fname=p->fname;
	     }
	     fprintf(fp," %d", p->line);
	     p=p->next;
	   }
	   fprintf(fp,"\n");
	 }
	 fname=NULL;
     }
}

/**********************************************************************************
 * cheap local labels
 */

static int cll_current = 0;	/* the current cheap local labels block */

/**
 * init the cheap local labels
 */
void cll_init() {
	cll_current = 0;
}

/**
 * get the block number for a new cheap local label block
 */
int cll_get() {
	if (cll_current == 0) {
		cll_current = b_new();
	}
	return cll_current;
}

/**
 * clear the local labels
 */
void cll_clear() {
	cll_current = 0;
}

int cll_getcur() {
	return cll_current;
}

/**********************************************************************************/

/**
 * define a global label
 */
int lg_set(char *s ) {
	int n, er;

	er = ll_search(s,&n, 0);

	if(er==E_OK) {
	  fprintf(stderr,"Warning: global label doubly defined!\n");
	} else {
          if(!(er=ll_def(s,&n,0))) {
            ltp=afile->la.lt+n;
            ltp->fl=2;
            ltp->afl=SEG_UNDEF;
          }
      	}
	return er;
}

/**********************************************************************************/
          
int l_def(char *s, int *l, int *x, int *f)
{     
     int n,er,b,i=0;
     int cll_fl;
 
     *f=0;	/* flag (given as param) that the label is to be re-defined and the 
		   "label defined error" is to be skipped */
     b=0;	/* block level on block stack, resp. block number */
     n=0;	/* flag, when set, b is absolute block number and not being translated */
     cll_fl=0;	/* when set, clear the cheap local label block */

     if(s[0]=='-')
     {
          *f+=1;	/* label is being redefined */
          i++;
     } else
     if(s[0]=='@')
     {
	  i++;
          n++;		/* block number b is absolute */
	  b=cll_get();	/* current (possibly newly allocated) cheap label block */
	  cll_fl=1;	/* do not clear the cll block again... */
     } else
     if(s[0]=='+')
     {
          i++;
          n++;		/* block number b is absolute */
          b=0;		/* global block number */
     } 
     while(s[i]=='&')
     {
	  if (n) b=0;	/* reset block number */
          n=0; 		/* block number is relative */    
          i++;
          b++;		/* one (more) level up the block stack */
     }
     if(!n) {
	  /* translate from block stack level to absolute block number */
          b_fget(&b,b);
     }

     if(!cll_fl) {
	/* clear cheap local labels */
	cll_clear();
     }

     if(!isalpha(s[i]) && s[i]!='_' && !(ca65 && isdigit(s[i]) ) )
          er=E_SYNTAX;
     else
     {
          er=ll_search(s+i,&n, cll_fl);
               
          if(er==E_OK)
          {
	       /* we actually found an existing label in the same scope */
               ltp=afile->la.lt+n;
               
               if(*f)
               {
		    /* redefinition of label */
                    *l=ltp->len+i;
               } else
               if(ltp->fl==0)
               {
		    /* label has not been defined yet, (e.g. pass1 forward ref), so we try to set it. */
                    *l=ltp->len+i;
                    if(b_ltest(ltp->blk,b))
                         er=E_LABDEF;
                    else
                         ltp->blk=b;

               } else
                    er=E_LABDEF;
          } else
          if(er==E_NODEF)
          {
               if(!(er=ll_def(s+i,&n,b))) /* store the label in the table of labels */
               {
                    ltp=afile->la.lt+n;
                    *l=ltp->len+i;
                    ltp->fl=0;
               }
          } 

          *x=n;
     }
     return(er);
}

int l_search(char *s, int *l, int *x, int *v, int *afl)
{
     int n,er,b;
     int cll_fl;

     *afl=0;

     /* check cheap local label */
     cll_fl=0;
     if (s[0]=='@') {
	cll_fl=1;	/* also used as offset to the label length, so must be 1 */
	s++;
     }

     er=ll_search(s,&n, cll_fl);
/*printf("l_search: lab=%s(l=%d, afl=%d, er=%d, n=%d, cll_fl=%d, cll_cur=%d)\n",s,*l, *afl,er,n, cll_fl, cll_getcur());*/
     if(er==E_OK)
     {
          ltp=afile->la.lt+n;
          *l=ltp->len + cll_fl;
          if(ltp->fl == 1)
          {
               l_get(n,v,afl);/*               *v=lt[n].val;*/
               *x=n;
          } else
          {
               er=E_NODEF;
               lz=ltp->n;
               *x=n;
          }
     }
     else
     {
	  if(cll_fl) {
		b=cll_get();
	  } else {
          	b_get(&b);
	  }
	  
          er=ll_def(s,x,b); /* ll_def(...,*v); */

          ltp=afile->la.lt+(*x);
          
          *l=ltp->len + cll_fl;

          if(!er) 
          {
               er=E_NODEF;
               lz=ltp->n;   
          }
     }
     return(er);
}

int l_vget(int n, int *v, char **s)
{
     ltp=afile->la.lt+n;
     (*v)=ltp->val;
     *s=ltp->n;
     return 0;
}

void l_addocc(int n, int *v, int *afl) {
     LabOcc *p, *pp;

     (void)v;		/* quench warning */
     (void)afl;		/* quench warning */
     ltp = afile->la.lt+n;
     pp=NULL;
     p = ltp->occlist;
     while(p) {
	if (p->line == filep->fline && p->fname == filep->fname) return;
	pp = p;
	p = p->next;
     }
     p = malloc(sizeof(LabOcc));
     if(!p) {
	fprintf(stderr,"Oops, out of memory!\n");
	exit(1);
     }
     p->next = NULL;
     p->line = filep->fline;
     p->fname = filep->fname;
     if(pp) {
       pp->next = p;
     } else {
       ltp->occlist = p;
     }
}

int l_get(int n, int *v, int *afl)
{
     if(crossref) l_addocc(n,v,afl);

     ltp=afile->la.lt+n;
     (*v)=ltp->val;
     lz=ltp->n;
     *afl = ltp->afl;
/*printf("l_get('%s'(%d), v=$%04x, afl=%d, fl=%d\n",ltp->n, n, *v, *afl, ltp->fl);*/
     return( (ltp->fl==1) ? E_OK : E_NODEF);
}

void l_set(int n, int v, int afl)
{
     ltp=afile->la.lt+n;
     ltp->val = v;
     ltp->fl = 1;
     ltp->afl = afl;
/*printf("l_set('%s'(%d), v=$%04x, afl=%d\n",ltp->n, n, v, afl);*/
}

static void ll_exblk(int a, int b)
{
     int i;
     for (i=0;i<afile->la.lti;i++)
     {
          ltp=afile->la.lt+i;
          if((!ltp->fl) && (ltp->blk==a))
               ltp->blk=b;
     }
}

static int ll_def(char *s, int *n, int b)          /* definiert naechstes Label  nr->n     */     
{
     int j=0,er=E_NOMEM,hash;
     char *s2;

/*printf("ll_def: s=%s\n",s);    */

     if(!afile->la.lt) {
	afile->la.lti = 0;
	afile->la.ltm = 1000;
	afile->la.lt = malloc(afile->la.ltm * sizeof(Labtab));
     } 
     if(afile->la.lti>=afile->la.ltm) {
	afile->la.ltm *= 1.5;
	afile->la.lt = realloc(afile->la.lt, afile->la.ltm * sizeof(Labtab));
     }
     if(!afile->la.lt) {
	fprintf(stderr, "Oops: no memory!\n");
	exit(1);
     }
#if 0
     if((lti<ANZLAB) /*&&(lni<(long)(LABMEM-MAXLAB))*/)
     {
#endif
          ltp=afile->la.lt+afile->la.lti;
/*          
          s2=ltp->n=ln+lni;

          while((j<MAXLAB-1) && (s[j]!='\0') && (isalnum(s[j]) || s[j]=='_'))
          {
               s2[j]=s[j];
               j++;
          }
*/
	  while((s[j]!='\0') && (isalnum(s[j]) || (s[j]=='_'))) j++;
	  s2 = malloc(j+1);
	  if(!s2) {
		fprintf(stderr,"Oops: no memory!\n");
		exit(1);
	  }
	  strncpy(s2,s,j);
	  s2[j]=0;
/*
          if(j<MAXLAB)
          {
*/
               er=E_OK;
               ltp->len=j;
	       ltp->n = s2;
               ltp->blk=b;
               ltp->fl=0;
               ltp->afl=0;
               ltp->occlist=NULL;
               hash=hashcode(s,j); 
               ltp->nextindex=afile->la.hashindex[hash];
               afile->la.hashindex[hash]=afile->la.lti;
               *n=afile->la.lti;
               afile->la.lti++;
/*               lni+=j+1;*/
/*          }
     }
*/
/*printf("ll_def return: %d\n",er);*/
     return(er);
}

/**
 * search a label name in the label table. Return the label number
 * in "n". Finds only labels that are in a block that is in the current
 * set of blocks (in the block stack)
 *
 * If cll_fl is set, the label is also searched in the local cheap label scope
 */
int ll_search(char *s, int *n, int cll_fl)          /* search Label in Tabelle ,nr->n    */
{
     int i,j=0,k,er=E_NODEF,hash;

     while(s[j] && (isalnum(s[j])||(s[j]=='_')))  j++;

     hash=hashcode(s,j);
     i=afile->la.hashindex[hash];

     if(i>=afile->la.ltm) return E_NODEF;

     do
     {
          ltp=afile->la.lt+i;
          
          if(j==ltp->len)
          {
               for (k=0;(k<j)&&(ltp->n[k]==s[k]);k++);

	       if (cll_fl) {
			if (ltp->blk == cll_getcur()) {
				er=E_OK;
				break;
			}
	       } else {
		       /* check if the found label is in any of the blocks in the
 			  current block stack */
               		if((j==k)&&(!b_test(ltp->blk)))
               		{
		    		/* ok, label found and it is reachable (its block nr is in the current block stack */
                    		er=E_OK;
                   		break;
               		}
	       }
          }

          if(!i)
               break;          

          i=ltp->nextindex;
          
     }while(1);
     
     *n=i;
#if 0
     if(er!=E_OK && er!=E_NODEF)
     {
          fprintf(stderr, "Fehler in ll_search:er=%d\n",er);
          getchar();
     }
#endif
     return(er);
}

int ll_pdef(char *t)
{
	int n;
	
	if(ll_search(t,&n, 0)==E_OK)
	{
		ltp=afile->la.lt+n;
		if(ltp->fl)
			return(E_OK);
	}
	return(E_NODEF);
}

int l_write(FILE *fp)
{
     int i, afl, n=0;

     if(noglob) {
	fputc(0, fp);
	fputc(0, fp);
	return 0;
     }
     for (i=0;i<afile->la.lti;i++) {
        ltp=afile->la.lt+i;
	if((!ltp->blk) && (ltp->fl==1)) {
	  n++;
	}
     }
     fputc(n&255, fp);
     fputc((n>>8)&255, fp);
     for (i=0;i<afile->la.lti;i++)
     {
          ltp=afile->la.lt+i;
          if((!ltp->blk) && (ltp->fl==1)) {
	    fprintf(fp, "%s",ltp->n);
	    fputc(0,fp);
	    afl = ltp->afl;
            /* hack to switch undef and abs flag from internal to file format */
/*printf("label %s, afl=%04x, A_FMASK>>8=%04x\n", ltp->n, afl, A_FMASK>>8);*/
            if( (afl & (A_FMASK>>8)) < SEG_TEXT) afl^=1;
	    fputc(afl,fp);
	    fputc(ltp->val&255, fp);
	    fputc((ltp->val>>8)&255, fp);
	  }
     }
     /*fputc(0,fp);*/
     return 0;
}

/*******************************************************************************************
 * block management code. Here the ".(" and ".)" blocks are maintained. 
 *
 * Blocks are numbered uniquely, every time a new block is opened, the "blk" variable
 * is increased and its number used as block number.
 *
 * The currently open blocks are maintained in a stack (bt[]). The lowest entry is the outermost
 * block number, adding block numbers as blocks are opened. When a block is closed,
 * the block stack is shortened again (bi has the length of the block stack)
 *
 * Methods exist to open new blocks, close a block, and do some checks, e.g. whether
 * a specific block number is contained in the current block stack.
 */
static int bt[MAXBLK];	/* block stack */
static int bi;		/* length of the block stack (minus 1, i.e. bi[bi] has the innermost block) */
static int blk;		/* current block number for allocation */

int b_init(void)
{
     blk =0;
     bi =0;
     bt[bi]=blk;

     return(E_OK);
}     

int b_new(void) 
{
	return ++blk;
}

int b_depth(void)
{
     return bi;
}

int ga_blk(void)
{
	return(blk);
}

/**
 * open a new block scope
 */
int b_open(void)
{
     int er=E_BLKOVR;

     if(bi<MAXBLK-1)
     {
          bt[++bi]=b_new();
          
          er=E_OK;  
     }
     return(er);
}

/**
 * close a block scope
 */
int b_close(void)
{

     if(bi)
     {
          ll_exblk(bt[bi],bt[bi-1]);
          bi--;
     } else {
	  return E_BLOCK;
     }

     return(E_OK);
}

/**
 * get the block number of the current innermost block
 */
static int b_get(int *n)
{
     *n=bt[bi];

     return(E_OK);
}

/**
 * returns the block number of the block "i" levels up in the current block stack
 */
static int b_fget(int *n, int i)
{
     if((bi-i)>=0)
          *n=bt[bi-i];
     else
          *n=0;
     return(E_OK);
}

/**
 * tests whether the given block number n is in the current stack of
 * current block numbers bt[]
 */
static int b_test(int n)
{
     int i=bi;

     while( i>=0 && n!=bt[i] )
          i--;

     return( i+1 ? E_OK : E_NOBLK );
}

/**
 * tests whether the given block number "a" is in the 
 */
static int b_ltest(int a, int b)    /* testet ob bt^-1(b) in intervall [0,bt^-1(a)]   */
{
     int i=0,er=E_OK;

     if(a!=b)
     {
          er=E_OK;

          while(i<=bi && b!=bt[i])
          {
               if(bt[i]==a)
               {
                    er=E_NOBLK;
                    break;
               }
               i++;
          }
     }
     return(er);
}

