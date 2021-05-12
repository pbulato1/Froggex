#include "HT16K33.h"
#include "sl_emlib_gpio_init_BUT_UP_config.h"
#include "sl_emlib_gpio_init_BUT_LEFT_config.h"
#include "sl_emlib_gpio_init_BUT_RIGHT_config.h"
#include "sl_emlib_gpio_init_BUT_DOWN_config.h"
#include "em_gpio.h"
#include "em_letimer.h"
#include "em_rtcc.h"
#include "em_cmu.h"

#define ON_THRESHOLD    0x3F
#define OFF_THRESHOLD   0xFC

static int8_t victoryX, dir, steps, level, random, enemyFire, ammoRandom, bossHealth;
static int8_t xFrog, yFrog, xAmmo, yAmmo, xBullet, yBullet;
static int8_t xEnemy1, xEnemy2, xEnemy3, xEnemy4, xEnemy5, xEnemy6, xEnemy7, xEnemy8, xEnemy9, xEnemy10;
static int8_t yEnemy1, yEnemy2, yEnemy3, yEnemy4, yEnemy5, yEnemy6, yEnemy7, yEnemy8, yEnemy9, yEnemy10;
static bool showLvl, ready, initialized, rise, coloring, won, skip, frogFired, enemyFired, ammoAvailable, loaded, gameComplete;
static uint8_t shiftRegUp = 0xFF;
static uint8_t shiftRegDn = 0xFF;         // shift registers for buttons debouncing
static uint8_t shiftRegR = 0xFF;
static uint8_t shiftRegL = 0xFF;
static uint8_t buttonStates = 0xFF;         //... and button states for debouncing
RTCC_Init_TypeDef initRTCC = RTCC_INIT_DEFAULT;
RTCC_CCChConf_TypeDef initCC0 = RTCC_CH_INIT_COMPARE_DEFAULT;

static void Letimer_setup(void)           // use for buttons debouncing
{
  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;
  CMU_ClockEnable(cmuClock_LETIMER0, true);
  letimerInit.comp0Top = true;            // Reload COMP0 on underflow
  letimerInit.topValue = CMU_ClockFreqGet(cmuClock_LETIMER0) / 100;
  LETIMER_Init(LETIMER0, &letimerInit);     // Initialize and enable LETIMER
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_UF);
  NVIC_EnableIRQ(LETIMER0_IRQn);
}

static void RTCC_setup(void)              // use for animation
{
  CMU_ClockEnable(cmuClock_RTCC, true);
  RTCC_CCChConf_TypeDef initCC0 = RTCC_CH_INIT_COMPARE_DEFAULT;
  initRTCC.enable = false;
  initRTCC.presc = rtccCntPresc_4096;       // clock period 4096/32768 sec
  initRTCC.prescMode = rtccCntTickPresc;
  RTCC_ChannelCompareValueSet(0, RTCC_CounterGet()+1);
  RTCC_ChannelInit(0, &initCC0);
  RTCC_Init(&initRTCC);
  RTCC_IntEnable(RTCC_IEN_CC0);
  NVIC_EnableIRQ(RTCC_IRQn);
  RTCC_Start();
}

/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
void app_init(void)
{
  //initialize game data
  HT16K33_init();
  Letimer_setup();
  RTCC_setup();
  showLvl = true;
  xFrog = 0;yFrog = 4; // init frog position
  coloring = true;
  level = 1;
  bool checkWalls(int x, int y);
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
}

//main loop
void RTCC_IRQHandler(void)                                          // RTC ISR
{
  if (RTCC_IntGet() & RTCC_IF_CC0) {                                  // check RTCC channel 0 event flag
    RTCC_ChannelCompareValueSet(0, RTCC_CounterGet() + 1);          // schedule next RTCC event
    RTCC_IntClear(RTCC_IF_CC0);                                   // clear RTCC flag

    //draw the number before level starts
     if(showLvl)drawNumber();
     //choose the correct level
     if (ready) {
         switch(level)
         {
           case 1:
             level1();
             break;
           case 2:
             level2();
             break;
           case 3:
             level3();
             break;
           case 4:
             level4();
             break;
           case 5:
             level5();
             break;
           case 6:
             level6();
             break;
           case 7:
             level7();
             break;
           case 8:
             level8();
             break;
           case 9:
             level9();
             break;
           case 10:
             finalBoss();
             break;
         }
     }
     //if level complete, show animation(green)
     else if(won)victoryAnimation();
     if(!gameComplete)checkCollisions();
  }
}

//green animation when level complete
void victoryAnimation()
{
    for(int i = 0; i < 8; i++){
       if(coloring) HT16K33_putPixel(victoryX, i, GREEN);
       else HT16K33_putPixel(victoryX, i, OFF);
    }

    if(victoryX == 7){
        victoryX = 0;
        if(!coloring){
            won = false;
            showLvl = true;
        }
        coloring = !coloring;
    }
    else victoryX++;
}

//check for wall collisions
bool wallHit(int x, int y)
{
   switch(level)
   {
     case 3:
       if((x == 0 && y != 0)
       || (x == 7 && y != 7))return true;
       break;
     case 4:
       if(y < 2 || y > 5)return true;
       break;
     case 5:
       if((x == 7 && y > 3)
       || (x > 2 && y == 4)
       || (x == 3 && y == 3)
       || (x == 4 && y == 3))return true;
       break;
     case 6:
       if((x == 0) && (y != 5 && y != 4 && y != 2))return true;
       if(x == 1 && y < 2)return true;
       if((x == 2 || x == 6) && (y != 7 && y != 5 && y != 2))return true;
       if((x == 4) && (y != 5 && y != 2 && y != 0))return true;
       if(x == 7 && y == 6)return true;
       break;
     case 8:
       if((x == 0) && (y > 4 || y < 2))return true;
       if (x == 2 && y != 1)return true;
       if((x == 4) && (y == 7 || y == 2 || y == 0))return true;
       if((x == 6) && (y != 6 && y != 3 && y != 2))return true;
       if(x == 7 && y != 6)return true;
       break;
     case 9:
       if((x == 0) && (y != 6 && y != 4 && y != 2))return true;
       if(x == 1 && y == 7)return true;
       if((x == 2 || x == 4) && (y < 6 && y != 1))return true;
       if((x == 3 || x == 5) && (y != 6 && y > 1))return true;
       if(x == 6 && y == 0)return true;
       if(x == 7 && y != 3 && y != 4)return true;
       break;
   }

   return false;
}

//switch off enemies light every time they move to a new cell
void turnEnemiesOff()
{
   HT16K33_putPixel(xEnemy1, yEnemy1, OFF);
   HT16K33_putPixel(xEnemy2, yEnemy2, OFF);
   HT16K33_putPixel(xEnemy3, yEnemy3, OFF);
   HT16K33_putPixel(xEnemy4, yEnemy4, OFF);
   HT16K33_putPixel(xEnemy5, yEnemy5, OFF);
   HT16K33_putPixel(xEnemy6, yEnemy6, OFF);
   HT16K33_putPixel(xEnemy7, yEnemy7, OFF);
   HT16K33_putPixel(xEnemy8, yEnemy8, OFF);
   HT16K33_putPixel(xEnemy9, yEnemy9, OFF);
   HT16K33_putPixel(xEnemy10, yEnemy10, OFF);
}

//draw number before level begins
void drawNumber()
{
  switch(level)
  {
    case 1:
      HT16K33_putPixel(4, 6, YELLOW);
      HT16K33_putPixel(4, 5, YELLOW);
      HT16K33_putPixel(4, 4, YELLOW);
      HT16K33_putPixel(4, 3, YELLOW);
      HT16K33_putPixel(4, 2, YELLOW);
      HT16K33_putPixel(4, 1, YELLOW);
      HT16K33_putPixel(3, 5, YELLOW);
      HT16K33_putPixel(3, 1, YELLOW);
      HT16K33_putPixel(4, 1, YELLOW);
      HT16K33_putPixel(5, 1, YELLOW);
     break;

    case 2:
      HT16K33_putPixel(2, 5, YELLOW);
      HT16K33_putPixel(3, 6, YELLOW);
      HT16K33_putPixel(4, 6, YELLOW);
      HT16K33_putPixel(5, 5, YELLOW);
      HT16K33_putPixel(5, 4, YELLOW);
      HT16K33_putPixel(4, 3, YELLOW);
      HT16K33_putPixel(3, 2, YELLOW);
      HT16K33_putPixel(2, 1, YELLOW);
      HT16K33_putPixel(3, 1, YELLOW);
      HT16K33_putPixel(4, 1, YELLOW);
      HT16K33_putPixel(5, 1, YELLOW);
      break;

    case 3:
      HT16K33_putPixel(2, 6, YELLOW);
      HT16K33_putPixel(3, 6, YELLOW);
      HT16K33_putPixel(4, 6, YELLOW);
      HT16K33_putPixel(5, 5, YELLOW);
      HT16K33_putPixel(3, 4, YELLOW);
      HT16K33_putPixel(4, 4, YELLOW);
      HT16K33_putPixel(5, 3, YELLOW);
      HT16K33_putPixel(5, 2, YELLOW);
      HT16K33_putPixel(2, 1, YELLOW);
      HT16K33_putPixel(3, 1, YELLOW);
      HT16K33_putPixel(4, 1, YELLOW);
      break;

    case 4:
      HT16K33_putPixel(2, 5, YELLOW);
      HT16K33_putPixel(2, 4, YELLOW);
      HT16K33_putPixel(2, 3, YELLOW);
      HT16K33_putPixel(3, 3, YELLOW);
      HT16K33_putPixel(4, 3, YELLOW);
      HT16K33_putPixel(5, 5, YELLOW);
      HT16K33_putPixel(5, 4, YELLOW);
      HT16K33_putPixel(5, 3, YELLOW);
      HT16K33_putPixel(5, 2, YELLOW);
      HT16K33_putPixel(5, 1, YELLOW);
      break;

    case 5:
      HT16K33_putPixel(5, 6, YELLOW);
      HT16K33_putPixel(4, 6, YELLOW);
      HT16K33_putPixel(3, 6, YELLOW);
      HT16K33_putPixel(2, 6, YELLOW);
      HT16K33_putPixel(2, 5, YELLOW);
      HT16K33_putPixel(2, 4, YELLOW);
      HT16K33_putPixel(3, 4, YELLOW);
      HT16K33_putPixel(4, 4, YELLOW);
      HT16K33_putPixel(5, 3, YELLOW);
      HT16K33_putPixel(5, 2, YELLOW);
      HT16K33_putPixel(4, 1, YELLOW);
      HT16K33_putPixel(3, 1, YELLOW);
      HT16K33_putPixel(2, 1, YELLOW);
      break;

    case 6:
      HT16K33_putPixel(2, 6, YELLOW);
      HT16K33_putPixel(2, 5, YELLOW);
      HT16K33_putPixel(2, 4, YELLOW);
      HT16K33_putPixel(2, 3, YELLOW);
      HT16K33_putPixel(2, 2, YELLOW);
      HT16K33_putPixel(2, 1, YELLOW);
      HT16K33_putPixel(3, 6, YELLOW);
      HT16K33_putPixel(3, 3, YELLOW);
      HT16K33_putPixel(3, 1, YELLOW);
      HT16K33_putPixel(4, 6, YELLOW);
      HT16K33_putPixel(4, 3, YELLOW);
      HT16K33_putPixel(4, 1, YELLOW);
      HT16K33_putPixel(5, 6, YELLOW);
      HT16K33_putPixel(5, 3, YELLOW);
      HT16K33_putPixel(5, 2, YELLOW);
      HT16K33_putPixel(5, 1, YELLOW);
      break;

    case 7:
      HT16K33_putPixel(2, 6, YELLOW);
      HT16K33_putPixel(3, 6, YELLOW);
      HT16K33_putPixel(4, 6, YELLOW);
      HT16K33_putPixel(5, 6, YELLOW);
      HT16K33_putPixel(5, 5, YELLOW);
      HT16K33_putPixel(4, 4, YELLOW);
      HT16K33_putPixel(3, 3, YELLOW);
      HT16K33_putPixel(3, 2, YELLOW);
      HT16K33_putPixel(3, 1, YELLOW);
      break;

    case 8:
      HT16K33_putPixel(2, 6, YELLOW);
      HT16K33_putPixel(3, 6, YELLOW);
      HT16K33_putPixel(4, 6, YELLOW);
      HT16K33_putPixel(5, 6, YELLOW);
      HT16K33_putPixel(2, 5, YELLOW);
      HT16K33_putPixel(5, 5, YELLOW);
      HT16K33_putPixel(2, 4, YELLOW);
      HT16K33_putPixel(3, 4, YELLOW);
      HT16K33_putPixel(4, 4, YELLOW);
      HT16K33_putPixel(5, 4, YELLOW);
      HT16K33_putPixel(2, 3, YELLOW);
      HT16K33_putPixel(3, 3, YELLOW);
      HT16K33_putPixel(4, 3, YELLOW);
      HT16K33_putPixel(5, 3, YELLOW);
      HT16K33_putPixel(2, 2, YELLOW);
      HT16K33_putPixel(5, 2, YELLOW);
      HT16K33_putPixel(2, 1, YELLOW);
      HT16K33_putPixel(3, 1, YELLOW);
      HT16K33_putPixel(4, 1, YELLOW);
      HT16K33_putPixel(5, 1, YELLOW);
      break;

    case 9:
      HT16K33_putPixel(2, 6, YELLOW);
      HT16K33_putPixel(3, 6, YELLOW);
      HT16K33_putPixel(4, 6, YELLOW);
      HT16K33_putPixel(5, 6, YELLOW);
      HT16K33_putPixel(2, 5, YELLOW);
      HT16K33_putPixel(5, 5, YELLOW);
      HT16K33_putPixel(2, 4, YELLOW);
      HT16K33_putPixel(3, 4, YELLOW);
      HT16K33_putPixel(4, 4, YELLOW);
      HT16K33_putPixel(5, 4, YELLOW);
      HT16K33_putPixel(5, 3, YELLOW);
      HT16K33_putPixel(5, 2, YELLOW);
      HT16K33_putPixel(5, 1, YELLOW);
      HT16K33_putPixel(4, 1, YELLOW);
      HT16K33_putPixel(3, 1, YELLOW);
      HT16K33_putPixel(2, 1, YELLOW);
      break;

    case 10:
      HT16K33_putPixel(1, 5, YELLOW);
      HT16K33_putPixel(2, 6, YELLOW);
      HT16K33_putPixel(2, 5, YELLOW);
      HT16K33_putPixel(2, 4, YELLOW);
      HT16K33_putPixel(2, 3, YELLOW);
      HT16K33_putPixel(2, 2, YELLOW);
      HT16K33_putPixel(2, 1, YELLOW);
      HT16K33_putPixel(4, 6, YELLOW);
      HT16K33_putPixel(5, 6, YELLOW);
      HT16K33_putPixel(6, 6, YELLOW);
      HT16K33_putPixel(4, 5, YELLOW);
      HT16K33_putPixel(4, 4, YELLOW);
      HT16K33_putPixel(4, 3, YELLOW);
      HT16K33_putPixel(4, 2, YELLOW);
      HT16K33_putPixel(4, 1, YELLOW);
      HT16K33_putPixel(6, 5, YELLOW);
      HT16K33_putPixel(6, 4, YELLOW);
      HT16K33_putPixel(6, 3, YELLOW);
      HT16K33_putPixel(6, 2, YELLOW);
      HT16K33_putPixel(6, 1, YELLOW);
      HT16K33_putPixel(5, 1, YELLOW);
      break;
  }

  showLvl = false;
}

//switch every light off
void switchOff()
{
  for(int i = 0; i < 8; i++)
    for(int j = 0; j < 8; j++)
       HT16K33_putPixel(i, j, OFF);

    ready = true;
}

void level1()
{
   turnEnemiesOff();
   //initialize level
  if(!initialized)
    {
        xEnemy1 = 1;
        yEnemy1 = 0;
        xEnemy2 = 2;
        yEnemy2 = 7;
        xEnemy3 = 3;
        yEnemy3 = 4;
        xEnemy4 = 4;
        yEnemy4 = 3;
        xEnemy5 = 5;
        yEnemy5 = 0;
        xEnemy6 = 6;
        yEnemy6 = 6;
        xEnemy7 = 7;
        yEnemy7 = 0;
        initialized = true;
    }

  //move enemies
  yEnemy1++;
  yEnemy2--;
  yEnemy3++;
  yEnemy4--;
  yEnemy5++;
  yEnemy6--;
  yEnemy7++;

  //boundary check for enemies
  if(yEnemy1 > 7)  yEnemy1 = 0;
  if(yEnemy2 < 0)  yEnemy2 = 7;
  if(yEnemy3 > 7)  yEnemy3 = 0;
  if(yEnemy4 < 0)  yEnemy4 = 7;
  if(yEnemy5 > 7)  yEnemy5 = 0;
  if(yEnemy6 < 0)  yEnemy6 = 7;
  if(yEnemy7 > 7)  yEnemy7 = 0;

  //draw enemies
  HT16K33_putPixel(xEnemy1, yEnemy1, RED);
  HT16K33_putPixel(xEnemy2, yEnemy2, RED);
  HT16K33_putPixel(xEnemy3, yEnemy3, RED);
  HT16K33_putPixel(xEnemy4, yEnemy4, RED);
  HT16K33_putPixel(xEnemy5, yEnemy5, RED);
  HT16K33_putPixel(xEnemy6, yEnemy6, RED);
  HT16K33_putPixel(xEnemy7, yEnemy7, RED);
}

void level2()
{
  //draw enemies
  HT16K33_putPixel(1, 7, RED);
  HT16K33_putPixel(3, 7, RED);
  HT16K33_putPixel(4, 7, RED);
  HT16K33_putPixel(6, 7, RED);
  HT16K33_putPixel(7, 7, RED);

  //initialize level
  if(!initialized)
      {
        xEnemy1 = 1;
        yEnemy1 = 7;
        xEnemy3 = 3;
        yEnemy3 = 7;
        xEnemy4 = 4;
        yEnemy4 = 7;
        xEnemy6 = 6;
        yEnemy6 = 7;
        xEnemy7 = 7;
        yEnemy7 = 7;
        initialized = true;
      }

  //elevate the enemy bars
    if(rise){
        HT16K33_putPixel(xEnemy1, yEnemy1, OFF);
        HT16K33_putPixel(xEnemy3, yEnemy3, OFF);
        HT16K33_putPixel(xEnemy4, yEnemy4, OFF);
        HT16K33_putPixel(xEnemy6, yEnemy6, OFF);
        HT16K33_putPixel(xEnemy7, yEnemy7, OFF);

        yEnemy1++;
        yEnemy3++;
        yEnemy4++;
        yEnemy6++;
        yEnemy7++;
    }
    //descend the enemy bars
    else{
        yEnemy1--;
        yEnemy3--;
        yEnemy4--;
        yEnemy6--;
        yEnemy7--;
    }
    //since all 3 bars are in sync, check only one of them to know when to switch direction
    if(yEnemy3 == 0)rise = true;
    else if(yEnemy3 == 7)rise = false;

    //draw enemy bars
    HT16K33_putPixel(xEnemy1, yEnemy1, RED);
    HT16K33_putPixel(xEnemy3, yEnemy3, RED);
    HT16K33_putPixel(xEnemy4, yEnemy4, RED);
    HT16K33_putPixel(xEnemy6, yEnemy6, RED);
    HT16K33_putPixel(xEnemy7, yEnemy7, RED);

    //increase difficulty at 3 stages depending on frog's position
    switch(xFrog) {
          case 0:
             initRTCC.presc = rtccCntPresc_4096;
             break;
          case 2:
             initRTCC.presc = rtccCntPresc_2048;
             break;
          case 5 :
            initRTCC.presc = rtccCntPresc_1024;
             break;
       }
    RTCC_Init(&initRTCC);
    RTCC_Start();
}

void level3()
{
  turnEnemiesOff();

  HT16K33_putPixel(0, 1, RED);
  HT16K33_putPixel(0, 2, RED);
  HT16K33_putPixel(0, 3, RED);
  HT16K33_putPixel(0, 4, RED);
  HT16K33_putPixel(0, 5, RED);
  HT16K33_putPixel(0, 6, RED);
  HT16K33_putPixel(0, 7, RED);
  HT16K33_putPixel(7, 6, RED);
  HT16K33_putPixel(7, 5, RED);
  HT16K33_putPixel(7, 4, RED);
  HT16K33_putPixel(7, 3, RED);
  HT16K33_putPixel(7, 2, RED);
  HT16K33_putPixel(7, 1, RED);
  HT16K33_putPixel(7, 0, RED);

  //initialize level
  if(!initialized)
    {
       xEnemy1 = -1;
       yEnemy1 = 1;
       xEnemy2 = -1;
       yEnemy2 = 3;
       xEnemy3 = -1;
       yEnemy3 = 5;
       xEnemy4 = 8;
       yEnemy4 = 2;
       xEnemy5 = 8;
       yEnemy5 = 4;
       xEnemy6 = 8;
       yEnemy6 = 6;
       xEnemy7 = -1;
       yEnemy7 = 7;

       rise = true;
       initRTCC.presc = rtccCntPresc_2048;       // clock period 4096/32768 sec
       RTCC_Init(&initRTCC);
       RTCC_Start();
       initialized = true;
    }

  if(rise){
      xEnemy1++;
      xEnemy2++;
      xEnemy3++;
      xEnemy7++;
      xEnemy4--;
      xEnemy5--;
      xEnemy6--;
  }
  else{
      xEnemy1--;
      xEnemy2--;
      xEnemy3--;
      xEnemy7--;
      xEnemy4++;
      xEnemy5++;
      xEnemy6++;
  }

  //all enemies are in sync, so check just one to know when to switch direction
  if(xEnemy1 == 6)rise = false;
  else if(xEnemy1 == 1)rise = true;

  HT16K33_putPixel(xEnemy1, yEnemy1, YELLOW);
  HT16K33_putPixel(xEnemy2, yEnemy2, YELLOW);
  HT16K33_putPixel(xEnemy3, yEnemy3, YELLOW);
  HT16K33_putPixel(xEnemy4, yEnemy4, YELLOW);
  HT16K33_putPixel(xEnemy5, yEnemy5, YELLOW);
  HT16K33_putPixel(xEnemy6, yEnemy6, YELLOW);
  HT16K33_putPixel(xEnemy7, yEnemy7, YELLOW);
}

void level4()
{
  HT16K33_putPixel(xEnemy1, yEnemy1, OFF);
  HT16K33_putPixel(xEnemy2, yEnemy2, OFF);

  //initialize level
  if(!initialized)
    {
      for(int i = 0; i < 8; i++)
        for(int j = 0; j < 8; j++)
          if(j < 2 || j > 5)HT16K33_putPixel(i, j, RED);

        xEnemy1 = 4;
        yEnemy1 = 2;
        xEnemy2 = 4;
        yEnemy2 = 5;

        dir = 1;

        initRTCC.presc = rtccCntPresc_1024;       // clock period 4096/32768 sec
        RTCC_Init(&initRTCC);
        RTCC_Start();

        initialized = true;
    }

  //move/circulate enemies
  switch(dir){
      case 1:
        yEnemy1++;
        yEnemy2--;
        break;
      case 2:
        xEnemy1--;
        xEnemy2++;
        break;
      case 3:
        yEnemy1--;
        yEnemy2++;
        break;
      case 4:
        xEnemy1++;
        xEnemy2--;
        break;
    }

   //change enemies' direction of movement when a corner is reached
    if(xEnemy1 == 4 && yEnemy1 == 2)dir = 1;
    else if(xEnemy1 == 4 && yEnemy1 == 5)dir = 2;
    else if(xEnemy1 == 1 && yEnemy1 == 5)dir = 3;
    else if(xEnemy1 == 1 && yEnemy1 == 2)dir = 4;

  HT16K33_putPixel(xEnemy1, yEnemy1, YELLOW);
  HT16K33_putPixel(xEnemy2, yEnemy2, YELLOW);
}

void level5()
{
  turnEnemiesOff();
  //draw enemies
  HT16K33_putPixel(3, 4, RED);
  HT16K33_putPixel(4, 3, RED);
  HT16K33_putPixel(4, 4, RED);
  HT16K33_putPixel(3, 3, RED);
  HT16K33_putPixel(5, 4, RED);
  HT16K33_putPixel(6, 4, RED);
  HT16K33_putPixel(7, 4, RED);
  HT16K33_putPixel(7, 5, RED);
  HT16K33_putPixel(7, 6, RED);
  HT16K33_putPixel(7, 7, RED);

  //initialize level
  if(!initialized)
      {
        xEnemy2 = 5;
        yEnemy2 = 2;
        xEnemy3 = 2;
        yEnemy3 = 2;
        xEnemy4 = 2;
        yEnemy4 = 5;
        steps = 0;
        initialized = true;
      }

  //move fireballs
  xEnemy2++;
  yEnemy2--;
  xEnemy3--;
  yEnemy3--;
  xEnemy4--;
  yEnemy4++;

  //draw fireballs
  HT16K33_putPixel(xEnemy2, yEnemy2, YELLOW);
  HT16K33_putPixel(xEnemy3, yEnemy3, YELLOW);
  HT16K33_putPixel(xEnemy4, yEnemy4, YELLOW);

  //reset fireball positions when out of bounds
  if(steps == 3){
            steps = 0;
            xEnemy2 = 4;
            yEnemy2 = 3;
            xEnemy3 = 3;
            yEnemy3 = 3;
            xEnemy4 = 3;
            yEnemy4 = 4;
        }
        else steps++;

  //increase speed midway through the level
  if(xFrog > 2 && yFrog < 3){
      initRTCC.presc = rtccCntPresc_2048;
      RTCC_Init(&initRTCC);
      RTCC_Start();
  }
}

void level6()
{
  //switch off fireballs
    HT16K33_putPixel(xEnemy1, yEnemy1, OFF);
    HT16K33_putPixel(xEnemy2, yEnemy2, OFF);

   //red barriers
    HT16K33_putPixel(0, 0, RED);
    HT16K33_putPixel(0, 1, RED);
    HT16K33_putPixel(0, 3, RED);
    HT16K33_putPixel(0, 6, RED);
    HT16K33_putPixel(0, 7, RED);
    HT16K33_putPixel(1, 0, RED);
    HT16K33_putPixel(1, 1, RED);
    HT16K33_putPixel(2, 0, RED);
    HT16K33_putPixel(2, 1, RED);
    HT16K33_putPixel(2, 3, RED);
    HT16K33_putPixel(2, 4, RED);
    HT16K33_putPixel(2, 6, RED);
    HT16K33_putPixel(4, 1, RED);
    HT16K33_putPixel(4, 3, RED);
    HT16K33_putPixel(4, 4, RED);
    HT16K33_putPixel(4, 6, RED);
    HT16K33_putPixel(4, 7, RED);
    HT16K33_putPixel(6, 0, RED);
    HT16K33_putPixel(6, 1, RED);
    HT16K33_putPixel(6, 3, RED);
    HT16K33_putPixel(6, 4, RED);
    HT16K33_putPixel(6, 6, RED);
    HT16K33_putPixel(7, 6, RED);

    //initialize level
    if(!initialized)
        {
          xEnemy1 = -1;
          yEnemy1 = 5;
          xEnemy2 = 8;
          yEnemy2 = 2;
          rise = true;
          initRTCC.presc = rtccCntPresc_1024;       // clock period 4096/32768 sec
          RTCC_Init(&initRTCC);
          RTCC_Start();
          initialized = true;
        }

    //move fireball
    if(rise){
        xEnemy1++;
        xEnemy2--;
    }
    else{
        xEnemy1--;
        xEnemy2++;
    }

    //draw fireballs
    HT16K33_putPixel(xEnemy1, yEnemy1, YELLOW);
    HT16K33_putPixel(xEnemy2, yEnemy2, YELLOW);

    //change fireball direction
    if(xEnemy1 == 7)rise = false;
    else if(xEnemy1 == 0)rise = true;
}

void level7()
{
  turnEnemiesOff();
  //initialize level
  if(!initialized)
    {
      xEnemy1 = 2;
      yEnemy1 = 8;
      xEnemy2 = 3;
      yEnemy2 = 8;
      xEnemy3 = 2;
      yEnemy3 = -1;
      xEnemy4 = 3;
      yEnemy4 = -1;
      xEnemy5 = 5;
      yEnemy5 = 7;
      xEnemy6 = 6;
      yEnemy6 = 7;
      xEnemy7 = 5;
      yEnemy7 = 0;
      xEnemy8 = 6;
      yEnemy8 = 0;
      xEnemy10 = 1;
      yEnemy10 = 0;

      dir = 1;
      rise = false;
      initialized = true;
    }

  //move enemies
   if(rise){
       yEnemy1++;
       yEnemy2++;
       yEnemy3--;
       yEnemy4--;
       yEnemy5++;
       yEnemy6++;
       yEnemy7--;
       yEnemy8--;
   }

   else{
       yEnemy1--;
       yEnemy2--;
       yEnemy3++;
       yEnemy4++;
       yEnemy5--;
       yEnemy6--;
       yEnemy7++;
       yEnemy8++;
   }

   switch(dir){
       case 1:
         yEnemy10++;
         break;
       case 2:
         xEnemy10++;
         break;
       case 3:
         yEnemy10--;
         break;
       case 4:
         xEnemy10--;
         break;
     }

     if(xEnemy10 == 1 && yEnemy10 == 0)dir = 1;
     else if(xEnemy10 == 1 && yEnemy10 == 7) dir = 2;
     else if(xEnemy10 == 7 && yEnemy10 == 7)dir = 3;
     else if(xEnemy10 == 7 && yEnemy10 == 0) dir = 4;

   if(xFrog == 4){
       initRTCC.presc = rtccCntPresc_2048;
       RTCC_Init(&initRTCC);
       RTCC_Start();
   }
   else if(xFrog == 0){
       initRTCC.presc = rtccCntPresc_2048;
       RTCC_Init(&initRTCC);
       RTCC_Start();
   }

   HT16K33_putPixel(xEnemy1, yEnemy1, RED);
   HT16K33_putPixel(xEnemy2, yEnemy2, RED);
   HT16K33_putPixel(xEnemy3, yEnemy3, RED);
   HT16K33_putPixel(xEnemy4, yEnemy4, RED);
   HT16K33_putPixel(xEnemy5, yEnemy5, RED);
   HT16K33_putPixel(xEnemy6, yEnemy6, RED);
   HT16K33_putPixel(xEnemy7, yEnemy7, RED);
   HT16K33_putPixel(xEnemy8, yEnemy8, RED);

   HT16K33_putPixel(xEnemy10, yEnemy10, YELLOW);

   if(yEnemy1 == 4)rise = true;
   else if(yEnemy1 == 7)rise = false;
}

void level8()
{
  //switch off fireballs
  HT16K33_putPixel(xEnemy1, yEnemy1, OFF);
  HT16K33_putPixel(xEnemy2, yEnemy2, OFF);
  HT16K33_putPixel(xEnemy3, yEnemy3, OFF);

  //draw fireballs
  HT16K33_putPixel(0, 7, RED);
  HT16K33_putPixel(0, 6, RED);
  HT16K33_putPixel(0, 5, RED);
  HT16K33_putPixel(0, 1, RED);
  HT16K33_putPixel(0, 0, RED);
  HT16K33_putPixel(2, 7, RED);
  HT16K33_putPixel(2, 6, RED);
  HT16K33_putPixel(2, 5, RED);
  HT16K33_putPixel(2, 4, RED);
  HT16K33_putPixel(2, 3, RED);
  HT16K33_putPixel(2, 2, RED);
  HT16K33_putPixel(2, 0, RED);
  HT16K33_putPixel(4, 0, RED);
  HT16K33_putPixel(4, 2, RED);
  HT16K33_putPixel(4, 7, RED);
  HT16K33_putPixel(6, 0, RED);
  HT16K33_putPixel(6, 1, RED);
  HT16K33_putPixel(6, 4, RED);
  HT16K33_putPixel(6, 5, RED);
  HT16K33_putPixel(6, 7, RED);
  HT16K33_putPixel(7, 0, RED);
  HT16K33_putPixel(7, 1, RED);
  HT16K33_putPixel(7, 2, RED);
  HT16K33_putPixel(7, 3, RED);
  HT16K33_putPixel(7, 4, RED);
  HT16K33_putPixel(7, 5, RED);
  HT16K33_putPixel(7, 7, RED);

  //initialize level
  if(!initialized)
    {
      xEnemy1 = 1;
      yEnemy1 = 8;
      xEnemy2 = 3;
      yEnemy2 = -1;
      xEnemy3 = 5;
      yEnemy3 = 8;
      rise = true;
      initRTCC.presc = rtccCntPresc_1024;       // clock period 4096/32768 sec
      RTCC_Init(&initRTCC);
      RTCC_Start();
      initialized = true;
    }

  //move fireballs
  if(rise){
       yEnemy1--;
       yEnemy2++;
       yEnemy3--;
    }
  else{
      yEnemy1++;
      yEnemy2--;
      yEnemy3++;
  }

    //draw fireballs
    HT16K33_putPixel(xEnemy1, yEnemy1, YELLOW);
    HT16K33_putPixel(xEnemy2, yEnemy2, YELLOW);
    HT16K33_putPixel(xEnemy3, yEnemy3, YELLOW);

    //all 3 fireballs are in sync so just check one of them if it is time to change direction
    if(yEnemy1 == 0)rise = false;
    else if(yEnemy1 == 7)rise = true;
}

void level9()
{
  //turn fireball off
  HT16K33_putPixel(xEnemy1, yEnemy1, OFF);

  //draw red barriers
  HT16K33_putPixel(0, 7, RED);
  HT16K33_putPixel(0, 5, RED);
  HT16K33_putPixel(0, 3, RED);
  HT16K33_putPixel(0, 1, RED);
  HT16K33_putPixel(0, 0, RED);
  HT16K33_putPixel(1, 7, RED);
  HT16K33_putPixel(2, 5, RED);
  HT16K33_putPixel(2, 4, RED);
  HT16K33_putPixel(2, 3, RED);
  HT16K33_putPixel(2, 2, RED);
  HT16K33_putPixel(2, 0, RED);
  HT16K33_putPixel(3, 7, RED);
  HT16K33_putPixel(3, 5, RED);
  HT16K33_putPixel(3, 4, RED);
  HT16K33_putPixel(3, 3, RED);
  HT16K33_putPixel(3, 2, RED);
  HT16K33_putPixel(4, 5, RED);
  HT16K33_putPixel(4, 4, RED);
  HT16K33_putPixel(4, 3, RED);
  HT16K33_putPixel(4, 2, RED);
  HT16K33_putPixel(4, 0, RED);
  HT16K33_putPixel(5, 7, RED);
  HT16K33_putPixel(5, 5, RED);
  HT16K33_putPixel(5, 4, RED);
  HT16K33_putPixel(5, 3, RED);
  HT16K33_putPixel(5, 2, RED);
  HT16K33_putPixel(6, 0, RED);
  HT16K33_putPixel(7, 7, RED);
  HT16K33_putPixel(7, 6, RED);
  HT16K33_putPixel(7, 5, RED);
  HT16K33_putPixel(7, 2, RED);
  HT16K33_putPixel(7, 0, RED);
  HT16K33_putPixel(7, 1, RED);

  //initialize level
  if(!initialized)
    {
      xEnemy1 = 1;
      yEnemy1 = 6;
      dir = 1;
      initRTCC.presc = rtccCntPresc_1024;       // clock period 4096/32768 sec
      RTCC_Init(&initRTCC);
      RTCC_Start();
      initialized = true;
      random = rand() % 2;
    }

  //based on the corner, choose random direction
  switch(dir){
    case 1:
      if(random == 1)xEnemy1++;
      else yEnemy1--;
      break;
    case 2:
      if(random == 1)xEnemy1++;
      else yEnemy1++;
      break;
    case 3:
      if(random == 1)xEnemy1--;
      else yEnemy1--;
      break;
    case 4:
      if(random == 1)xEnemy1--;
      else yEnemy1++;
      break;
  }

  //if corner reached, time to switch direction
  if(xEnemy1 == 1 && yEnemy1 == 6){
      dir = 1;
      random = rand() % 2;
  }
  else if(xEnemy1 == 1 && yEnemy1 == 1){
      dir = 2;
      random = rand() % 2;
  }
  else if(xEnemy1 == 6 && yEnemy1 == 6)  {
      dir = 3;
      random = rand() % 2;
    }
  else if(xEnemy1 == 6 && yEnemy1 == 1){
      dir = 4;
      random = rand() % 2;
  }
   //draw fireball
  HT16K33_putPixel(xEnemy1, yEnemy1, YELLOW);
}

void finalBoss()
{
  //turn enemies and bullet off
  turnEnemiesOff();
  HT16K33_putPixel(xBullet, yBullet, OFF);

  //draw the barriers
  HT16K33_putPixel(0, 3, RED);
  HT16K33_putPixel(1, 3, RED);
  HT16K33_putPixel(6, 3, RED);
  HT16K33_putPixel(7, 3, RED);

  //draw the boss green health bar
  drawBossHealth();

  //check if ammo collected
  if(xFrog == xAmmo){
      loaded = true;
      xAmmo = -1;
      yAmmo = -1;
  }

  //move bullet upwards when fired
  if(frogFired){
      if((xBullet > 1 && xBullet < 6 && yBullet < 6)
       ||((xBullet < 2 || xBullet > 5) && yBullet < 2))
        yBullet++;
      else {
        yBullet = 0;
        frogFired = false;
      }
  }

  //initialize level
  if(!initialized)
    {
       xEnemy1 = 2;
       yEnemy1 = 5;
       xEnemy2 = 3;
       yEnemy2 = 5;
       yEnemy3 = 5;
       xEnemy3 = 4;

       //enemy fire
       xEnemy4 = xEnemy2;
       yEnemy4 = yEnemy4;

       bossHealth = 8;

       initRTCC.presc = rtccCntPresc_2048;
       RTCC_Init(&initRTCC);
       RTCC_Start();

       initialized = true;
    }

  //check for collision of bullet and boss
  if(yBullet == yEnemy1){
        if((xBullet == xEnemy1)
        || (xBullet == xEnemy2)
        || (xBullet == xEnemy3)){
            yBullet = 0;
            frogFired = false;
            HT16K33_putPixel(bossHealth - 1, 7, OFF);
            bossHealth--;
            if(bossHealth == 0)gameComplete = true;
            //enemy hit
        }
    }

  if(gameComplete){
      if(xEnemy5 < -10)endLevel();
      else xEnemy5 --;
  }

  //if ammo already collected, generate another one at a random position and time
  if(!ammoAvailable){
      ammoRandom = rand() % 30;
      if(ammoRandom == 7){
          ammoAvailable = true;
          do{
              xAmmo = rand() % 7;
              yAmmo = 0;
          }
          while(xAmmo == xFrog);
      }
  }

  //if boss not firing currently (bullet not in the air), prepare next firing randomly
  if(!enemyFired && xEnemy4 > 1 && xEnemy4 < 6)
    {
      yEnemy4 = yEnemy2;
      enemyFire = rand() % 3;
      if(enemyFire == 1){
          enemyFired = true;
      }
    }

  //if boss fired, move bullet downwards
  if(enemyFired && yEnemy4 > -1)yEnemy4--;
  else {
      enemyFired = false;
      xEnemy4 = xEnemy2;
  }

  //makes sure that boss always moves 2 steps consecutively in the same direction(unless when it hits the wall)
 if(!skip)random = rand() % 2;

 //move boss in the selected direction
  if(random == 1 && xEnemy1 > 0){
     xEnemy1--;
     xEnemy2--;
     xEnemy3--;
  }
  else if(xEnemy3 < 7){
      xEnemy1++;
      xEnemy2++;
      xEnemy3++;
  }

  //draw frog,boss,ammo and bullets
  if(frogFired)HT16K33_putPixel(xBullet, yBullet, YELLOW);

  if(!gameComplete){
      HT16K33_putPixel(xEnemy4, yEnemy4, RED);
      HT16K33_putPixel(xEnemy1, yEnemy1, RED);
      HT16K33_putPixel(xEnemy2, yEnemy2, RED);
      HT16K33_putPixel(xEnemy3, yEnemy3, RED);
      if(ammoAvailable)HT16K33_putPixel(xAmmo, yAmmo, YELLOW);
      skip = !skip;
  }
  HT16K33_putPixel(xFrog, yFrog, GREEN);

}

//reset frog when hit
void resetFrog()
{
  HT16K33_putPixel(xFrog, yFrog, OFF);

  if(level == 10)bossHealth = 8;

  if(level == 5){
      xFrog = 7;
      yFrog = 5;
  }
  else if(level == 3 || level == 10){
      xFrog = 0;
      yFrog = 0;
  }
  else{
      xFrog = 0;
      yFrog = 4;
  }
  HT16K33_putPixel(xFrog, yFrog, GREEN);
}

//check if frog got hit, reset if so
void checkCollisions()
{
  if((xFrog == xEnemy1 && yFrog == yEnemy1)
  || (xFrog == xEnemy2 && yFrog == yEnemy2)
  || (xFrog == xEnemy3 && yFrog == yEnemy3)
  || (xFrog == xEnemy4 && yFrog == yEnemy4)
  || (xFrog == xEnemy5 && yFrog == yEnemy5)
  || (xFrog == xEnemy6 && yFrog == yEnemy6)
  || (xFrog == xEnemy7 && yFrog == yEnemy7)
  || (xFrog == xEnemy8 && yFrog == yEnemy8)
  || (xFrog == xEnemy9 && yFrog == yEnemy9)
  || (xFrog == xEnemy10 && yFrog == yEnemy10)
  )
    {
     resetFrog();
    }
}

//reset enemy coordinates/positions
void resetCoordinates()
{
  xBullet = -1;
  yBullet = -1;
  xAmmo = -1;
  yAmmo = -1;
  xEnemy1 = -1;
  xEnemy2 = -1;
  xEnemy3 = -1;
  xEnemy4 = -1;
  xEnemy5 = -1;
  xEnemy6 = -1;
  xEnemy7 = -1;
  xEnemy8 = -1;
  xEnemy9 = -1;
  xEnemy10 = -1;

  yEnemy1 = -1;
  yEnemy2 = -1;
  yEnemy3 = -1;
  yEnemy4 = -1;
  yEnemy5 = -1;
  yEnemy6 = -1;
  yEnemy7 = -1;
  yEnemy8 = -1;
  yEnemy9 = -1;
  yEnemy10 = -1;
}

void drawBossHealth(){
  for(int i = 0; i < bossHealth; i++)
      HT16K33_putPixel(i, 7, GREEN);
}

void endLevel()
{
   xFrog = 0;
   ready = false;
   won = true;
   resetCoordinates();
   initialized = false;
   level++;
   initRTCC.presc = rtccCntPresc_1024;
   RTCC_Init(&initRTCC);
   RTCC_Start();
}

void LETIMER0_IRQHandler(void)
{
  LETIMER_IntClear(LETIMER0, LETIMER_IEN_UF);
  //*** BUTTON_UP debouncing ***************************
  shiftRegUp = (shiftRegUp >> 1) | (GPIO_PinInGet(SL_EMLIB_GPIO_INIT_BUT_UP_PORT, SL_EMLIB_GPIO_INIT_BUT_UP_PIN) << 7); // update shift register
  if (buttonStates & 1)           // old button state = OFF
  {
    if (shiftRegUp <= ON_THRESHOLD && ready)    // button is pressed
    {
      if(loaded){
          xBullet = xFrog;
          frogFired = true;
          ammoAvailable = false;
          loaded = false;
      }
      else if(level != 10 && !wallHit(xFrog, yFrog + 1)) {
          buttonStates &= ~1;
          HT16K33_putPixel(xFrog, yFrog, OFF);    // button Up is just pressed
          yFrog++;
          if (yFrog > 7) yFrog = 7;
          HT16K33_putPixel(xFrog, yFrog, GREEN);
      }

    }
  }
  else if (shiftRegUp >= OFF_THRESHOLD) // button is released
    buttonStates |= 1;

  //*** BUTTON_DOWN debouncing ***************************
  shiftRegDn = (shiftRegDn >> 1) | (GPIO_PinInGet(SL_EMLIB_GPIO_INIT_BUT_DOWN_PORT, SL_EMLIB_GPIO_INIT_BUT_DOWN_PIN) << 7); // update shift register
  if (buttonStates & 2)           // old button state = OFF
  {
    if (shiftRegDn <= ON_THRESHOLD && ready && !wallHit(xFrog, yFrog - 1))    // button is pressed
    {
      buttonStates &= ~2;
      HT16K33_putPixel(xFrog, yFrog, OFF);
      yFrog--;
      if (yFrog < 0) yFrog = 0;
      HT16K33_putPixel(xFrog, yFrog, GREEN);
    }
  }
  else if (shiftRegDn >= OFF_THRESHOLD) // button is released
    buttonStates |= 2;

  //*** BUTTON_LEFT debouncing ***************************
  shiftRegL = (shiftRegL >> 1) | (GPIO_PinInGet(SL_EMLIB_GPIO_INIT_BUT_LEFT_PORT, SL_EMLIB_GPIO_INIT_BUT_LEFT_PIN) << 7); // update shift register
  if (buttonStates & 4)           // old button state = OFF
  {
    if (shiftRegL <= ON_THRESHOLD)    // button is pressed
    {
      //level starts
      if(!ready){
          switchOff();
          resetCoordinates();
          ready = true;
          initRTCC.presc = rtccCntPresc_4096;
          resetFrog();
          RTCC_Init(&initRTCC);
          RTCC_Start();
      }
         if(!wallHit(xFrog - 1, yFrog)){
          buttonStates &= ~4;
          HT16K33_putPixel(xFrog, yFrog, OFF);
          xFrog--;
          if (xFrog < 0) xFrog = 0;
          HT16K33_putPixel(xFrog, yFrog, GREEN);
      }

    }
  }
  else if (shiftRegL >= OFF_THRESHOLD)  // button is released
    buttonStates |= 4;

  //*** BUTTON_RIGHT debouncing ***************************
  shiftRegR = (shiftRegR >> 1) | (GPIO_PinInGet(SL_EMLIB_GPIO_INIT_BUT_RIGHT_PORT, SL_EMLIB_GPIO_INIT_BUT_RIGHT_PIN) << 7); // update shift register
  if (buttonStates & 8)           // old button state = OFF
  {
    if (shiftRegR <= ON_THRESHOLD && ready && !wallHit(xFrog + 1, yFrog))   // button is pressed
    {
      buttonStates &= ~8;
      HT16K33_putPixel(xFrog, yFrog, OFF);
      xFrog++;
      //if level complete(won)
      if (level != 10 && xFrog > 7){
          endLevel();
      }
      HT16K33_putPixel(xFrog, yFrog, GREEN);
    }
  }
  else if (shiftRegR >= OFF_THRESHOLD)  // button is released
    buttonStates |= 8;

  HT16K33_writeDisplay();
//
}

