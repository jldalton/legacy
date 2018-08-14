
/*   CHARMS by JOHN L. DALTON (c) 1990, 1992
 *   Modelled after the 1990 Sega arcade game Columns
 *   Written using Turbo C, v.2.0
 *   Dates worked on: 901108,1109,1112,1118
 */

/*  UPDATES:
 *  910311      CR001     Allow sound to be turned off by -s option
 *  921114      CR002     Don't reset time-of-day clock for timing!
 *                        version 1.02
 */


/* TERMS:
     fallee - any vertical set of any number of characters that are suspended
              in air, and hence on their way down.

     magic fallee - whatever color it lands on, that color is erased from
                    the entire board.
 */

/* INSTRUCTIONS:
     Charms are obtained by matching three or more in a row (vertical,
     horizontal, or diagonal).  If the pile of charms reaches the gray
     level, the game is over.

     Scoring: Each charm obtained is worth:  bonus*level points.
       The bonus is 10, but if level>10, the bonus equals the level.
*/

#include <conio.h>  /* kbhit,getch et.al. */
#include <stdlib.h> /* random */
#include <bios.h>   /* biostime */
#include <dos.h>    /* sound */

#define YMAX          (13)
#define XMAX          (10)
#define LEFTX         (17)
#define RIGHTX        (22)
#define BOTY          (23)
#define TOPY          (9)
#define STARTX        (20)
#define STARTY        (11)
#define SCOREX        (7)
#define SCOREY        (15)
#define LEVELX        (7)
#define LEVELY        (18)
#define INSTRUCTX     (27)
#define INSTRUCTY     (15)
#define CHARMSX       (7)
#define CHARMSY       (21)
#define NEXTX         (24)
#define NEXTY         (11)
#define CLEFT         (75)
#define CRIGHT        (77)
#define CDOWN         (80)
#define CUP           (72)
#define TRUE          (BOOLEAN)(1)
#define FALSE         (BOOLEAN)(0)
#define EMPTY         (0)
#define DRAW          (0)
#define ERASE         (1)
#define xytoscr(x,y)  ((y)*(monoc?160:80)+(x)*2)
#define hidecursor()  gotoxy(1,25)
#define AIR           (32)
#define FACE          (1)
#define DOT           (9)
#define USER          (1)
#define NEXT          (0)
#define BLACK   0
#define BLUE    1
#define GREEN   2
#define CYAN    3
#define RED     4
#define MAGENTA 5
#define ORANGE  6
#define LTGRAY  7
#define DKGRAY  8
#define LTBLUE  9
#define LTGREEN 10
#define LTCYAN  11
#define LTRED   12
#define PURPLE  13
#define YELLOW  14
#define WHITE   15

typedef unsigned char BYTE;
typedef unsigned char BOOLEAN;

typedef struct SPOS
{
  BYTE x,y;
} SPOS;

typedef struct FALLEE
{
  SPOS pos;
  BYTE ccolor[YMAX];
} FALLEE;

/* DIRECTION */
typedef struct Dir
{
  int dx,dy;
} Dir;

/*---------------------*/
/* FUNCTION PROTOTYPES */
/*---------------------*/

void fill_smem(BYTE x, BYTE c);
int fall(int whichf);
void updatef_xy(int whichf,BYTE ux,BYTE uy);
void out_fallee(int whichf,BOOLEAN clear);
void out_thing(BYTE jx,BYTE jy, BYTE thing, BYTE jc);
void init_userf(void);
BYTE look_above (BYTE x,BYTE y);
BYTE look_at (BYTE x,BYTE y);
BYTE look_below (BYTE x,BYTE y);
int identify_all_matches(int ftotal,BOOLEAN useronly);
void initialization(void);
void out_instruct(BOOLEAN also_begin);
int end_of_game(void);
void flash_and_rid();
void drawline(BYTE x1,BYTE y1, BYTE x2, BYTE y2,BYTE ch,BYTE cl);
void up_score(float amount);
void up_charms(int amount);
void up_level(int amount);
int zapmagic(BYTE magicc);
void pick_next_fallee(BOOLEAN magic);
void seffect(int which);
void exit_program(void);

/*-------------*/
/* GLOBAL DATA */
/*-------------*/

/* DIRECTIONS FOR CHECKING EACH OF FOUR ROWTYPES */
/* USED IN IDENTIFY_ALL_MATCHES */
Dir chk[4];

FALLEE flist[YMAX*XMAX];  /* 1st is user's, 0th is user's next */
BYTE _colors[7] = {WHITE,RED,YELLOW,GREEN,MAGENTA,LTCYAN,LTBLUE};
BYTE _charms[7] = {2,    3,  21,    5,    6,      15,     4};

int level;
int turns;
int bonus;
float score;
int charms;

BOOLEAN dropflag;
BOOLEAN gameover;
BOOLEAN monoc;
BOOLEAN soundflag;

unsigned SCRMEM;

long timecheck; // 921114 JLD CR002

/*===============================================*/
/* MAIN:                                         */
/*===============================================*/
main(argc, argv)
   int argc;
   char *argv[];
{
   BYTE mx,my;
   BYTE ch;
   int mtotal,ftotal;
   BYTE magicolor;
   unsigned scrlocs[3] = {0xB800,0xA000,0xB000};
   int i;

   /* CHECK IF NO SOUND */ // { CR001
   soundflag = TRUE;
   if (argc>1)
   {
     if (strcmp(*++argv,"-s")==0 || strcmp(*argv,"-S")==0)
       soundflag = FALSE;
   }
   // } CR001

   /* CHECK IF MONOCHROME */
   monoc = (peekb(0,0x0449)==7);

   /* DETERMINE WHERE SCREEN MEMORY IS STORED */
   gotoxy(1,1);
   cprintf("á"); /* beta - ascii 225 */
   for (i=0; i<=2; i++)
   {
     SCRMEM = scrlocs[i];
     if (peekb(SCRMEM,0)=='á')
       break;
   }

   /* COULDN'T FIND SCREEN MEMORY */
   if (i==3)
     exit(0);

   delay(20);
   if (!monoc)
     textmode(C40); /* 40-column mode */

 Begin:
   turns=0;
   gameover = FALSE;
   randomize();

   initialization();

   pick_next_fallee(FALSE);
   out_fallee(NEXT,FALSE);

   do{
     /* PUT FALLEE AT TOP OF SCREEN */
     init_userf();

     /* LET FALLEE FALL */
     magicolor = fall(USER); /* if was magic fallee, what color land on? */

     /* CHECK IF GAME IS OVER */
     if (gameover)
       if (end_of_game())
         exit_program();
       else
         goto Begin;

     /* DETERMINE IF THERE WERE ANY 3+ IN A ROW */
     mtotal = (magicolor) ? zapmagic(magicolor) :
                            identify_all_matches(1,TRUE);
     if (mtotal>0)
       do
       {
         flash_and_rid();
         ftotal = form_and_drop_new_fallees();
       } while ((mtotal=identify_all_matches(ftotal,FALSE)) > 0);

   } while (TRUE==TRUE);

}

/*************************************************/
/* DRAWLINE:                                     */
/* DRAW LINE UP TO BUT NOT INCL. END POINT       */
/*************************************************/
void drawline(BYTE x1,BYTE y1, BYTE x2, BYTE y2,BYTE ch,BYTE cl)
{
  BYTE h,v;

  /* DRAW HORIZ OR VERT LINE */
  if (x1==x2)
  {
    /* VERT */
    for (v=y1; v!=y2; v+=((y1<y2)?1:-1))
      out_thing(x1,v,ch,cl);
  }
  else
  {
    /* HORIZ */
    for (h=x1; h!=x2; h+=((x1<x2)?1:-1))
      out_thing(h,y1,ch,cl);
  }
}
/*************************************************/
/* END_OF_GAME:                                  */
/* RETURNS TRUE IF USER WANTS TO QUIT            */
/*************************************************/
int end_of_game(void)
{
  BYTE mx,my;
  char ch;

  /* CLEAR THE BOARD */
  delay(300);
  for (my=BOTY; my >= TOPY-2; my--)
  {
    if (my>=TOPY)
      drawline(LEFTX,my,RIGHTX+1,my,FACE,WHITE);
    if (my+2<=BOTY)
      drawline(LEFTX,my+2,RIGHTX+1,my+2,AIR,BLACK);
    delay(45);
  }

  /* TELL USER THAT <ENTER> RESTARTS GAME */
  out_instruct(TRUE);

  /* FLASH GAME OVER, WAIT FOR A KEY */
  textattr(CYAN | 128);
  gotoxy(12,5);
  cprintf ("G A M E    O V E R");
  while((ch=getch())!=13)
    if (ch==27)
      return(1);

  return(0);
}
/*************************************************/
/* EXIT_PROGRAM:                                 */
/*************************************************/
void exit_program(void)
{
  fill_smem(AIR,WHITE);  /* clear screen */
  textmode(C80);         /* set back to 80-column mode */
  gotoxy(1,1);           /* cursor at top of screen */
  exit(0);
}

/*************************************************/
/* FALL:                                         */
/* SEND FALLEE NUMBER.                           */
/* FALLEE FALLS UNTIL IT LANDS ON SOMETHING.     */
/* IF FALLEE IS MAGIC (WHITE), THIS RETURNS      */
/* THE COLOR THAT WAS LANDED UPON.               */
/*************************************************/
int fall(int whichf)
{
  BOOLEAN user = (whichf==USER);
  BYTE uin;
  BYTE fx,fy;
  BYTE temp;
  int i;
  long timzup;


  fx = flist[whichf].pos.x;
  fy = flist[whichf].pos.y;
  dropflag = FALSE;

  /* IF CHARM(S) PILE UP PAST TOP LINE, GAME IS OVER! */
  if (user)
    for (i=LEFTX; i<=RIGHTX; i++)
      if (look_at(i,STARTY)!=BLACK)
        gameover = TRUE;

  if (!gameover)
    do
    {
      out_fallee(whichf,DRAW);

      if (kbhit() && user)
      {
        /* CHECK FOR USER MOVEMENT OF FALLEE */
        uin = getch();
        switch(uin)
        {
          case 27:     /* ESCAPE = ABRUPT QUIT */
            exit_program();

          case ',':
          case CLEFT:
          case '<': /* THE COMMA,LESS-THAN KEY MOVES FALLEE LEFT */
          {
            if (!look_at(fx-1,fy) &&    /* make sure room to move */
                !look_at(fx-1,fy-1) &&
                !look_at(fx-1,fy-2))
            {
              fx --;
              updatef_xy(whichf,fx,fy);
            }
            break;
          }
          case '>':
          case CRIGHT:
          case '.': /* THE PERIOD,GREATER-THAN KEY MOVES FALLEE RIGHT */
          {
            if (!look_at(fx+1,fy) &&
                !look_at(fx+1,fy-1) &&
                !look_at(fx+1,fy-2))
            {
              fx ++;
              updatef_xy(whichf,fx,fy);
            }
            break;
          }
          case ' ': /* THE SPACE BAR ROTATES USER'S FALLEE */
          {
            temp = flist[whichf].ccolor[0];
            flist[whichf].ccolor[0] = flist[whichf].ccolor[1];
            flist[whichf].ccolor[1] = flist[whichf].ccolor[2];
            flist[whichf].ccolor[2] = temp;
            break;
          }
          case CDOWN: /* cursor down key */
          {
            dropflag = TRUE;
            break;
          }
          case CUP:
          {
            dropflag = FALSE;
            break;
          }
        } /* end switch */
      } /* end if */

      /* CONTROL SPEED OF FALL */
      timzup = (long)( (level<10)  ? (11-level)*2 : 4);
      if (!user || dropflag)
        timzup = 1;

      if (abs(biostime(0,0L)-timecheck)>=timzup) // 921114 JLD CR002
      {
        timecheck = biostime(0,0L); // 921114 JLD CR002
        if (look_below(fx,fy) == BLACK)
        {
          /* FALL ONE STEP DOWN */
          updatef_xy(whichf,fx,++fy);

          /* SOUND FOR LANDING */
          if (look_below(fx,fy)!=BLACK && user)
            seffect(0);

          /* SHOW WHAT CHARMS ARE "ON DECK" (NEXT) */
          if (user && fy==STARTY+1)
            out_fallee(NEXT,FALSE);

        }
        else
          /* FALLEE HAS LANDED */
          return( (look_at(fx,fy)==WHITE) ? look_below(fx,fy) : 0);
      } /* end if */

  } while(TRUE==TRUE);

}

/*************************************************/
/* FILL_SMEM:                                    */
/* FILL SCREEN MEMORY WITH CHARACTER X, COLOUR C */
/*************************************************/
void fill_smem(BYTE x, BYTE c)
{
  int i;

  for (i=0; i<=4000; i++)
  {
    pokeb(SCRMEM,i,x);
    pokeb(SCRMEM,++i,c);
  }
}
/*************************************************/
/* FLASH_AND_RID:                                */
/* FLASH ALL MATCHED CHARMS AND TAKE THEM OFF SCR*/
/*************************************************/
void flash_and_rid()
{
  int flash,amatch;
  BYTE x0,y0;
  int which;
  int countj=0;
  BYTE colour[YMAX*XMAX];

  /* FLASH FOUR TIMES */
  for (flash=0; flash <= 7; flash++)
  {
    x0 = LEFTX;
    y0 = 23;
    which = 0;

    do
    {
      if ((peekb(SCRMEM,xytoscr(x0,y0))&255)==DOT)
      {
        if (!flash)
        {
          countj++;
          colour[which] = look_at(x0,y0);
        }

        out_thing(x0,y0,(flash==7)? AIR : DOT,
                        ((flash % 2) ? BLACK : colour[which]));
        which++;
        delay(10);
        if (flash % 2)
          seffect(1);
      }

      x0++;
      if (x0>RIGHTX)
      {
        x0 = LEFTX;
        y0--;
      }
    } while (y0 >= STARTY-3);
  }

  up_charms(charms+countj);
  up_score(score+countj*(bonus*level));
}
/*************************************************/
/* FORM_AND_DROP_NEW_FALLEES:                    */
/* AFTER RIDDING MATCHED CHARMS, THERE WILL BE   */
/* BLANK SPOTS ON SCREEN.  THIS ROUTINE WILL     */
/* CAUSE ALL CHARM PILES SUSPENDED IN MID AIR TO */
/* FALL DOWN.                                    */
/* RETURNS NUMBER OF PILES THAT FELL             */
/*************************************************/
int form_and_drop_new_fallees()
{
  int new_total=0;
  BYTE px,py;
  BYTE x1,y1;
  int i;
  BYTE bc;

  px = LEFTX;
  py = 23;

  do
  {
    /* IS THIS A BLANK SPOT WITH STUFF ABOVE IT? */
    if (look_at(px,py)==BLACK && look_above(px,py)!=BLACK)
    {
      /* FORM A NEW FALLEE */
      flist[++new_total + 1].pos.x = px;
      flist[new_total+1].pos.y = py-1;
      i=0;
      x1=px; y1=py-1;
      do
      {
        bc = look_at(x1,y1);
        flist[new_total+1].ccolor[i++] = bc;
        y1--;
      } while (bc!=BLACK);
    }

    /* SCAN ENTIRE BOARD */
    px++;
    if (px>RIGHTX)
    {
      px = LEFTX;
      py--;
    }
  } while (py >= STARTY-3);

  /* CAUSE ALL AIR-SUSPENDED FALLEES TO DROP */
  for (i=1; i<=(new_total); i++)
  {
    fall(i+1);
  }
  return(new_total);
}


/*************************************************/
/* IDENTIFY_ALL_MATCHES:                         */
/* SEND TOTAL # OF FALLEES THAT RECENTLY LANDED  */
/* SEND useronly=TRUE IF JUST USER FALLEE LANDED */
/* RETURNS NUMBER OF WAYS THERE WERE 3+ IN A ROW */
/*************************************************/
int identify_all_matches(int ftotal,BOOLEAN useronly)
{
  int whichf;
  BYTE bcolor,bx,by,x0,y0,xt,yt;
  int dir;
  int i,k;
  int kount;     /* number of charms in a row */
  int mcount=0;  /* number of matches (3+ in a row) */
  BYTE rowtype;  /* [0-3] == [- / | \] */

  SPOS goodrow[YMAX+4];

  /* FOR EACH FALLEE DO */
  for (whichf=(useronly)?1:2;
       whichf<=((useronly)?1:ftotal+1);
       whichf++)
  {
    /* GET LOCATION OF FALLEE */
    bx = flist[whichf].pos.x;
    by = flist[whichf].pos.y;

    /* FOR EACH CHARM OF FALLEE DO */
    for (i=0; flist[whichf].ccolor[i] != BLACK; i++,by--)
    {
      /* WHAT COLOR IS THE CHARM */
      bcolor = look_at(bx,by);

      /* FOR EACH ROW TYPE CONTAINING THIS CHARM */
      for (rowtype=0; rowtype<=3; rowtype++) /* / \ | - */
      {
        kount = 1;

        /* FOR EACH DIRECTION FROM CHARM ALONG THIS ROW */
        for (dir=0; dir<=1; dir++)
        {
          x0 = bx; y0 = by;
          goodrow[1].x = x0;
          goodrow[1].y = y0;

          /* COUNT ALL IDENTICAL CHARMS IN A ROW */
          while (look_at( (xt=x0+(dir*2-1)*chk[rowtype].dx),
                          (yt=y0+(dir*2-1)*chk[rowtype].dy))
                 == bcolor)
          {
            kount++;
            x0 = xt; y0 = yt;
            goodrow[kount].x = x0;
            goodrow[kount].y = y0;
          }

        } /* next dir */

        /* KEEP TRACK OF ALL 3+ IN A ROW */
        if (kount>=3)
        {
          /* CHANGE CHARM TO A DOT, SO WE KNOW TO RID IT FROM BOARD */
          for (k=1; k<=kount; k++)
            out_thing (goodrow[k].x,goodrow[k].y,DOT,bcolor);

          mcount++;
        }

      } /* next row */
    } /* next charm of fallee */
  } /* next fallee */

  return (mcount);
}
/*************************************************/
/* INITIALIZATION:                               */
/* FOR BEGINNING OF GAME                         */
/*************************************************/
void initialization(void)
{
  BYTE mx,my;

   /* THE FOUR ROW TYPE DIRECTIONS */
   chk[0].dx = +1; chk[0].dy = -1;  /*  /  */
   chk[1].dx = +1; chk[1].dy = +1;  /*  \  */
   chk[2].dx =  0; chk[2].dy = +1;  /*  |  */
   chk[3].dx = +1; chk[3].dy =  0;  /*  -  */

   /* CLEAR SCREEN */
   fill_smem(AIR,0);

   /* BOTTOM LINE */
   drawline(1,24,40,24,177,DKGRAY);

   /* LEFT WALL */
   for (mx=LEFTX-1; mx >=1; mx--)
   {
     drawline(mx,23,mx,11,177,ORANGE);
     drawline(mx,11,mx,5,177,LTGRAY);
   }

   /* RIGHT WALL */
   for (mx=RIGHTX+1; mx<=39; mx++)
   {
     drawline(mx,23,mx,11,177,ORANGE);
     drawline(mx,11,mx,5,177,LTGRAY);
   }

   /* TITLE */
   textattr(16*LTGRAY+BLUE);
   gotoxy (10,1);   cprintf ("      C H A R M S     ");
   gotoxy (10,2);   cprintf ("                      ");
   gotoxy (10,3);   cprintf (" (c) 1992 John Dalton ");

   /* STATUS FRAMEWORK */
   textattr(YELLOW);  gotoxy (SCOREX,SCOREY);   cprintf ("SCORE:");
   textattr(CYAN);    gotoxy (LEVELX,LEVELY);   cprintf ("LEVEL:");
   textattr(GREEN);   gotoxy(CHARMSX,CHARMSY);  cprintf ("CHARMS");

   up_score(0);
   up_level(1);
   up_charms(0);

   /* HOW TO PLAY */
   out_instruct(FALSE);

   bonus = 10;

   timecheck = biostime(0,0L); // 921114 JLD CR002
}

/*************************************************/
/* INIT_USERF:                                   */
/* PUT A NEW SET OF CHARMS AT TOP OF SCREEN      */
/*************************************************/
void init_userf(void)
{
  int i;

  flist[USER].pos.x = STARTX;
  flist[USER].pos.y = STARTY;
  flist[USER].ccolor[3] = EMPTY;

  /* TRANSFER "ON DECK" FALLEE TO THE CURRENT FALLEE */
  for (i=0; i<=2; i++)
    flist[USER].ccolor[i] = flist[NEXT].ccolor[i];

  pick_next_fallee(FALSE);

  /* GAME PROGRESSIONS */
  turns++;
  if (!(turns%10))
  {
    /* NEXT LEVEL EVERY 10 FALLEES */
    up_level(level+1);
    bonus = (level>10) ? level : 10;

    /* MAGIC FALLEE AT START OF EVERY 7th LEVEL */
    if (level%7==0)
      pick_next_fallee(TRUE);
  }

}
/*************************************************/
/* LOOK ROUTINES                                 */
/* RETURNS COLOR AT,ABOVE,OR BELOW (X,Y)         */
/*************************************************/
BYTE look_at (BYTE x,BYTE y)
{
  unsigned int spos = xytoscr(x,y);

  return(peekb(SCRMEM,spos+1)&15);
}

BYTE look_above (BYTE x,BYTE y)
{
  return(look_at(x,y-1));
}

BYTE look_below (BYTE x,BYTE y)
{
  return(look_at(x,y+1));
}

/*************************************************/
/* OUT_FALLEE:                                   */
/* DRAW ALL CHARMS OF A FALLEE ON SCREEN         */
/* OR ERASE FALLEE IF clear IS SET               */
/*************************************************/
void out_fallee(int whichf,BOOLEAN clear)
{
  int   i = 0;
  int j;
  BYTE ox = flist[whichf].pos.x;
  BYTE oy = flist[whichf].pos.y;
  BYTE oc;
  BYTE ot;

  /* FOR EACH CHARM OF FALLEE */
  while ((oc=flist[whichf].ccolor[i++]) > EMPTY)
  {
      if (clear)
        oc = BLACK;

      ot = AIR;
      for (j=0; j<=6; j++)
        if (oc==_colors[j])
          ot = _charms[j];  /* shape of charm is dictated by the color */

      out_thing(ox,oy--,ot,oc);
  }
}
/*************************************************/
/* OUT_INSTRUCT:                                 */
/* SHOW USER HOW TO PLAY                         */
/* CALL WITH 'TRUE' AT END OF GAME TO TELL HOW   */
/* TO PLAY AGAIN                                 */
/*************************************************/
void out_instruct(BOOLEAN also_begin)
{

   textattr(LTRED);
   gotoxy (INSTRUCTX,INSTRUCTY+1);   cprintf ("  %c   Left   \n",27);
   gotoxy (INSTRUCTX,INSTRUCTY+2);   cprintf ("  %c   Right  \n",26);
   gotoxy (INSTRUCTX,INSTRUCTY+3);   cprintf ("[SPC] Rotate \n");
   gotoxy (INSTRUCTX,INSTRUCTY+4);   cprintf ("  %c   Drop   \n",25);
   gotoxy (INSTRUCTX,INSTRUCTY+5);   cprintf ("[ESC] Quit   \n");

   if (also_begin)
   {
     textattr(LTCYAN);
     gotoxy (INSTRUCTX,INSTRUCTY+7);     cprintf ("[ENTER] Begin");
   }

   hidecursor();

}
/*************************************************/
/* OUT_THING:                                    */
/* OUTPUT A CHARACTER OF CERTAIN COLOR AT X,Y    */
/*************************************************/
void out_thing(BYTE x,BYTE y, BYTE thing, BYTE cc)
{
  int cpos = xytoscr(x,y);

  pokeb(SCRMEM,cpos,thing);
  pokeb(SCRMEM,cpos+1,cc);
}
/*************************************************/
/* PICK_NEXT_FALLEE:                             */
/* DECIDE WHAT CHARMS ARE TO FALL NEXT TURN      */
/* CALL WITH 'TRUE' TO HAVE NEXT FALLEE BE MAGIC */
/*************************************************/
void pick_next_fallee(BOOLEAN magic)
{
  int i;

  for (i=0; i<=2; i++)
    flist[NEXT].ccolor[i] = (magic) ? WHITE : _colors[random(6)+1];

  flist[NEXT].pos.x = NEXTX;
  flist[NEXT].pos.y = NEXTY;

}
/*************************************************/
/* SEFFECT:                                      */
/* SOUND EFFECTS ROUTINE                         */
/*************************************************/
void seffect(int which)
{
  if (soundflag)
    switch(which)
    {
      case 0: sound(20); delay(20); nosound();break;
      case 1: return;
      case 2: return;
    }
}

/*************************************************/
/* UPDATEF_XY:                                   */
/* ERASE OLD FALLEE, RECORD NEW X-Y POSITION     */
/* DRAW FALLEE AT NEW LOCATION                   */
/*************************************************/
void updatef_xy(int whichf,BYTE nx,BYTE ny)
{
  out_fallee(whichf,ERASE);

  flist[whichf].pos.x = nx;
  flist[whichf].pos.y = ny;

  out_fallee(whichf,DRAW);
}
/*************************************************/
/* UPDATE STATUS INFORMATION ROUTINES            */
/*************************************************/
void up_charms(int amount)
{
  charms = amount;
  textattr(LTGRAY);
  gotoxy (CHARMSX,CHARMSY+1);
  cprintf ("%6d",charms);
  hidecursor();
}

void up_level(int amount)
{
  level = amount;
  textattr(LTGRAY);
  gotoxy (LEVELX,LEVELY+1);
  cprintf ("%6d",level);
  hidecursor();
}

void up_score(float amount)
{
  score = amount;
  gotoxy (SCOREX,SCOREY+1);
  textattr(LTGRAY);
  cprintf ("%6.0f",score);
  hidecursor();
}

/*************************************************/
/* ZAPMAGIC:                                     */
/* FIND ALL CHARMS SUCH AS THE ONE MAGIC FALLEE  */
/* LANDED ON                                     */
/* SEND COLOR OF CHARM TO SEARCH FOR             */
/* RETURN NUMBER OF SUCH CHARMS FOUND            */
/*************************************************/
int zapmagic(BYTE magicc)
{
  int zx,zy;
  int totalz=0;
  BYTE cc;

  /* SCAN ENTIRE BOARD */
  for (zy=23; zy >=STARTY-2; zy--)
    for (zx=LEFTX; zx <= RIGHTX; zx++)
      if ((cc=look_at(zx,zy))==magicc || (cc==WHITE))
      {
        /* CHANGE CHARM TO DOT SO WE KNOW TO GET RID OF IT */
        out_thing(zx,zy,DOT,cc);
        seffect(2);
        delay(5);
        totalz++;
      }

  return(totalz);
}
