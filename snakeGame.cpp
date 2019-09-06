/*
Running the command "make" in the command-line in the working directory
will build an executable file that can be run with the default game
parameters.

Command-line instructions to compile and run:

    g++ -o snake snake.cpp -L/usr/X11R6/lib -lX11 -lstdc++
	./snake

Note: the -L option and -lstdc++ may not be needed on some machines.
Execution of the above command-line inputs will build an executable
with the default game parameters.

Change difficulty by specifying an integer in the range [0-9], which
will specify the desired snake speed. This was primarily used for testing
purposes so some speeds may lead to white space between snake pixels.An 
error is displayed if any other argument format is given.
*/

// Import header files.
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

// STL library
#include <list>
#include <iterator>
#include <map>
#include <sstream>

// Header files for X functions.
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace std;

/*
 * Global game variables.
 */
// Screen parameters.
const int SCREEN_HEIGHT = 750;
const int SCREEN_WIDTH = 800;
const int INFO_OFFSET = 40;
const int WINDOW_CORNER_X = 10;
const int WINDOW_CORNER_Y = 10;
const int BORDER_SIZE = 5;
const int HORIZONTAL_INFO_OFFSET = 25;

// Snake parameters.
int PIXEL_WIDTH = 20;
const double INITIAL_CORNER1_X = 300;
const double INITIAL_CORNER1_Y = 300;
const double INITIAL_CORNER2_X = (300 + PIXEL_WIDTH);
const double INITIAL_CORNER2_Y = (300 + PIXEL_WIDTH);
double speedArrray[5] = {2, 4, 6, 8, 10}; 

// Other game parameters.
bool showSplash = true;
bool gamePaused = false;
bool alive = true;
int score = 0;
const double FPS = 30;
const int BUFFER_SIZE = 10;

/*
 * A struct containing the display, window, screenNumber,
 * graphic context, score, snakeSpeed, deltaTime step, 
 * and a key pressed boolean to incorporate a slight time 
 * delay in the application.
 */
struct XInfo {
    Display * display;
    Window window;
    int screenNumber;
    GC gc;
    int score;
    double snakeSpeed;
    unsigned long deltaTime;
    XPoint currentFruitLoc;
    bool keyPressedBool;
};
// Declare XInfo structure.
XInfo xInfo;

/*
 * Snake direction enum.
 */
enum Direction {NORTH, EAST, SOUTH, WEST};

/*
 * An abstract class for displayable objects.
 */
class Displayable {
    public:
        virtual void paint(XInfo &xInfo) = 0;
};

/*
 * A text displayable class derived from Displayable.
 */
class Text: public Displayable {
    public:
        virtual void paint(XInfo &xInfo) 
        {
            XDrawImageString(xInfo.display, xInfo.window, xInfo.gc,
                            this->x, this->y, this->s.c_str(), this->s.length());
        }

        Text(int x, int y, string s): x(x), y(y), s(s)
        {
            // Set member variables to input variables.
        }

    private:
        int x;
        int y;
        string s;
};
// Declare list of pointers to displayable objects.
list<Displayable *> dList;
list<Displayable *> dList1;
list<Displayable *> dList2;

/*
 * A scoreText displayable class derived from Displayable.
 */
class ScoreText: public Displayable {
    public:
        virtual void paint (XInfo &xInfo)
        {
            XDrawLine(xInfo.display, xInfo.window, xInfo.gc,
                            0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);

            XDrawImageString(xInfo.display, xInfo.window, xInfo.gc,
                            this->x, this->y,
                            this->scoreDisplay.c_str(), this->scoreDisplay.length());
        }

        void updateScore(XInfo &xInfo)
        {
            xInfo.score++;
            scoreDisplay = "Score: " + to_string(xInfo.score);
        }
        void reInitScore(XInfo &xInfo)
        {
            int scoreZero = 0;
            scoreDisplay = "Score: " + to_string(scoreZero);
            xInfo.score = 0;
        }

        // ScoreText(int x, int y, int score): x(x), y(y)
        ScoreText(int x, int y): x(x), y(y)
        {
            int scoreZero = 0;
            scoreDisplay = "Score: " + to_string(scoreZero);
        }

    private:
        int x;
        int y;
        string scoreDisplay;
};
// Declare ScoreText object.
ScoreText scoreText(HORIZONTAL_INFO_OFFSET , SCREEN_HEIGHT + 25);

/*
 * A snake displayable class derived from Displayable.
 */
class Snake : public Displayable {
    public:
        virtual void paint(XInfo &xInfo)
        {
            list<XPoint> :: iterator it1, it2;
            it1 = snakeListUL.begin();
            it2 = snakeListLR.begin();
            while (it1 != snakeListUL.end())
            {
                XFillRectangle(xInfo.display, xInfo.window, xInfo.gc,
                                it1->x, it1->y,
                                ((it2->x) - (it1->x)),
                                ((it2->y) - (it1->y)));
                it1++;
                it2++;
            }
        }

        // Move function.
        void move(XInfo &xInfo)
        {
            // Get the leading XPoint structures.
            XPoint leadPixelUL = snakeListUL.front();
            XPoint leadPixelLR = snakeListLR.front();
            
            // Set new lead pixels for tracking the length snake.
            XPoint newLeadPixelUL;
            XPoint newLeadPixelLR;

            // Set new lead corner pixels for tracking collisions.
            XPoint newLeadCorner1;
            XPoint newLeadCorner2;

            switch (direction)
            {
                // Set new lead pixel coordinates for movement in the North direction.
                // Set new lead conrer pixels for movement in the North direction.
                case NORTH:
                    newLeadPixelUL.x = leadPixelUL.x;
                    newLeadPixelUL.y = leadPixelUL.y - (xInfo.snakeSpeed*xInfo.deltaTime)/1000000;
                    newLeadPixelLR.x = leadPixelLR.x;
                    newLeadPixelLR.y = leadPixelLR.y - (xInfo.snakeSpeed*xInfo.deltaTime)/1000000;
                    
                    newLeadCorner1.x = newLeadPixelUL.x;
                    newLeadCorner1.y = newLeadPixelUL.y;
                    newLeadCorner2.x = newLeadPixelLR.x ;
                    newLeadCorner2.y = newLeadPixelLR.y - PIXEL_WIDTH;
                    break;

                // Set new lead pixel coordinates for movement in the East direction.
                // Set new lead conrer pixels for movement in the East direction.
                case EAST:
                    newLeadPixelUL.x = leadPixelUL.x + (xInfo.snakeSpeed*xInfo.deltaTime)/1000000;
                    newLeadPixelUL.y = leadPixelUL.y;
                    newLeadPixelLR.x = leadPixelLR.x + (xInfo.snakeSpeed*xInfo.deltaTime)/1000000;
                    newLeadPixelLR.y = leadPixelLR.y;

                    newLeadCorner1.x = newLeadPixelUL.x + PIXEL_WIDTH;
                    newLeadCorner1.y = newLeadPixelUL.y;
                    newLeadCorner2.x = newLeadPixelLR.x;
                    newLeadCorner2.y = newLeadPixelLR.y;
                    break;

                // Set new lead pixel coordinates for movement in the South direction.
                // Set new lead conrer pixels for movement in the South direction.
                case SOUTH:
                    newLeadPixelUL.x = leadPixelUL.x;
                    newLeadPixelUL.y = leadPixelUL.y + (xInfo.snakeSpeed*xInfo.deltaTime)/1000000;
                    newLeadPixelLR.x = leadPixelLR.x;
                    newLeadPixelLR.y = leadPixelLR.y + (xInfo.snakeSpeed*xInfo.deltaTime)/1000000;
                    
                    newLeadCorner1.x = newLeadPixelUL.x;
                    newLeadCorner1.y = newLeadPixelUL.y + PIXEL_WIDTH;
                    newLeadCorner2.x = newLeadPixelLR.x;
                    newLeadCorner2.y = newLeadPixelLR.y;
                    break;

                // Set new lead pixel coordinates for movement in the West direction.
                // Set new lead conrer pixels for movement in the West direction.
                case WEST:
                    newLeadPixelUL.x = leadPixelUL.x - (xInfo.snakeSpeed*xInfo.deltaTime)/1000000;
                    newLeadPixelUL.y = leadPixelUL.y;
                    newLeadPixelLR.x = leadPixelLR.x - (xInfo.snakeSpeed*xInfo.deltaTime)/1000000;
                    newLeadPixelLR.y = leadPixelLR.y;

                    newLeadCorner1.x = newLeadPixelUL.x;
                    newLeadCorner1.y = newLeadPixelUL.y;
                    newLeadCorner2.x = newLeadPixelLR.x - PIXEL_WIDTH;
                    newLeadCorner2.y = newLeadPixelLR.y;
                    break;
            }           
            
            // Check if snake is in contact with the wall.
            if (newLeadPixelUL.x < 0 || newLeadPixelUL.y < 0)
            {
                alive = false;
            }
            else if (newLeadPixelLR.x > SCREEN_WIDTH || newLeadPixelLR.y > SCREEN_HEIGHT)
            {
                alive = false;
            }    

            // Print out of snake pixel coordinates for debugging.

            // cout << "UL.x " << newLeadPixelUL.x << " & " << "UL.y" << newLeadPixelUL.y << endl;
            // cout << "LR.x" << newLeadPixelLR.x << " & " << "LR.y" << newLeadPixelLR.y << endl;
            // cout << " " << endl;
               
            /* 
             * Check if snake is in contact with itself.
             * This is achieved by checking if the lead
             * corner pixel coordinates occur in the list
             * of snake pixels.
             */ 
            list<XPoint> :: iterator iter1 = snakeListUL.begin();
            list<XPoint> :: iterator iter2 = snakeListLR.begin();;

            if (!xInfo.keyPressedBool)
            {
                while (iter1 != snakeListUL.end())
                {
                    if ( (iter1->x < newLeadCorner1.x)
                        && (iter2->x > newLeadCorner1.x)
                        && (iter1->y < newLeadCorner1.y)
                        && (iter2->y > newLeadCorner1.y) )
                    {
                        alive = false;
                    }

                    if ( (iter1->x < newLeadCorner2.x)
                        && (iter2->x > newLeadCorner2.x)
                        && (iter1->y < newLeadCorner2.y)
                        && (iter2->y > newLeadCorner2.y) )
                    {
                        alive = false;
                    }
                    iter1++;
                    iter2++;
                }
            }

            // Add new lead pixel to snakeList.
            snakeListUL.push_front(newLeadPixelUL);
            snakeListLR.push_front(newLeadPixelLR);

            // Check if snake is in contact with fruit.
            // If it is not then pop_back the trailing
            // pixel, if it is then keep the trailing 
            // pixel.
            double tempFruitX = xInfo.currentFruitLoc.x;
            double tempFruitY = xInfo.currentFruitLoc.y;

            if ( (newLeadCorner1.x > tempFruitX)
                && (newLeadCorner1.x < tempFruitX + PIXEL_WIDTH)
                && (newLeadCorner1.y > tempFruitY)
                && ( newLeadCorner1.y < tempFruitY + PIXEL_WIDTH) )
            {
                updateFruitLoc(xInfo);
                scoreText.updateScore(xInfo);
            }
            else if ( (newLeadCorner2.x > tempFruitX)
                && (newLeadCorner2.x < tempFruitX + PIXEL_WIDTH)
                && (newLeadCorner2.y > tempFruitY)
                && ( newLeadCorner2.y < tempFruitY + PIXEL_WIDTH) )
            {
                updateFruitLoc(xInfo);
                scoreText.updateScore(xInfo);
            }
            else
            {
                // Remove last set of XPoint structures that 
                // corresponds to the trailing pixel.
                snakeListUL.pop_back();
                snakeListLR.pop_back();
            }
        }

        // Update fruit to random location after snake eats it/
        void updateFruitLoc(XInfo &xInfo)
        {
            double randX;
            double randY;

            bool loop = true;

            list<XPoint> :: iterator iterate1 = snakeListUL.begin();
            list<XPoint> :: iterator iterate2 = snakeListLR.begin();;

            while(loop)
            {
                randX = rand() % (SCREEN_WIDTH - 2*PIXEL_WIDTH) + PIXEL_WIDTH;
                randY = rand() % (SCREEN_HEIGHT - 2*PIXEL_WIDTH - INFO_OFFSET) + PIXEL_WIDTH;
                xInfo.currentFruitLoc.x = randX;
                xInfo.currentFruitLoc.y = randY;

                while (iterate1 != snakeListUL.end())    
                {
                    loop = false;
                    if (xInfo.currentFruitLoc.x > iterate1->x
                        && xInfo.currentFruitLoc.x < iterate2->x
                        && xInfo.currentFruitLoc.y > iterate1->y
                        && xInfo.currentFruitLoc.y < iterate2->y)
                    {
                        loop = true;
                    }
                    iterate1++;
                    iterate2++;
                }             
            }
        }
        
        // Reinitialize snake on restart.
        void restart()
        {
            snakeListUL.clear();
            snakeListLR.clear();
            initSnake();
        }

        // Move North.
        void pressUpKey()
        {
            if ( direction != SOUTH)
            {
                direction = NORTH;
            }
        }

        // Move East.
        void pressRightKey()
        {
            if ( direction != WEST)
            {
                direction = EAST;
            }
        }

        // Move South.
        void pressDownKey()
        {
            if ( direction != NORTH)
            {
                direction = SOUTH;
            }
        }

        // Move West.
        void pressLeftKey()
        {
            if ( direction != EAST)
            {
                direction = WEST;
            }
        }

        list<XPoint> getSnakeListUL()
        {
            return snakeListUL;
        } 

        list<XPoint> getSnakeListLR()
        {
            return snakeListLR;
        }

        // Snake constructor.
		Snake()
        {
            /* 
             * Add corner1 to list of (x,y) coordinates for all
             * upper left pixel corners, and add corner2 to list
             * of (x,y) coordinates for all lower righ pixel corners.
             * Set East as the default direction of the snake.
             */
            corner1.x = INITIAL_CORNER1_X;
            corner1.y = INITIAL_CORNER1_Y;
            corner2.x = INITIAL_CORNER2_X;
            corner2.y = INITIAL_CORNER2_Y;
			snakeListUL.push_front(corner1);
            snakeListLR.push_front(corner2);
            direction = EAST;
		}

    private:
        XPoint corner1;
        XPoint corner2;
        Direction direction;
        list<XPoint> snakeListUL;
        list<XPoint> snakeListLR;

        void initSnake()
        {
            corner1.x = INITIAL_CORNER1_X;
            corner1.y = INITIAL_CORNER1_Y;
            corner2.x = INITIAL_CORNER2_X;
            corner2.y = INITIAL_CORNER2_X;
			snakeListUL.push_front(corner1);
            snakeListLR.push_front(corner2);
            direction = EAST;
        }
};
// Declare Snake object.
Snake snake;

/*
 * A fruit displayable class derived from Displayable.
 */
class Fruit: public Displayable {
    public:
        virtual void paint(XInfo &xInfo)
        {
            XFillRectangle(xInfo.display, xInfo.window, xInfo.gc,
                            xInfo.currentFruitLoc.x, 
                            xInfo.currentFruitLoc.y,
                            PIXEL_WIDTH, PIXEL_WIDTH);
        }

        void regenerateFruit(XInfo &xInfo)
        {
            list<XPoint> tempSnakeListUL = snake.getSnakeListUL();
            list<XPoint> tempSnakeListLR = snake.getSnakeListLR();

            list<XPoint> :: iterator iterate1, iterate2;
            iterate1 = tempSnakeListUL.begin();
            iterate2 = tempSnakeListLR.begin();

            bool loop = true;

            while(loop)
            {
                randX = rand() % (SCREEN_WIDTH - 2*PIXEL_WIDTH) + PIXEL_WIDTH;
                randY = rand() % (SCREEN_HEIGHT - 2*PIXEL_WIDTH - INFO_OFFSET) + PIXEL_WIDTH;
                xInfo.currentFruitLoc.x = randX;
                xInfo.currentFruitLoc.y = randY;

                while (iterate1 != tempSnakeListUL.end())    
                {
                    loop = false;
                    if (xInfo.currentFruitLoc.x > iterate1->x
                        && xInfo.currentFruitLoc.x < iterate2->x
                        && xInfo.currentFruitLoc.y > iterate1->y
                        && xInfo.currentFruitLoc.y < iterate2->y)
                    {
                        loop = true;
                    }
                    iterate1++;
                    iterate2++;
                }             
            }
        }

        Fruit(XInfo &xInfo)
        {
            // Initialize fruit in the drawable area that does
            // not intersect with the initial snake location.
            do
            {
                randX = rand() % (SCREEN_WIDTH - 2*PIXEL_WIDTH) + PIXEL_WIDTH;
                randY = rand() % (SCREEN_HEIGHT - 2*PIXEL_WIDTH - INFO_OFFSET) + PIXEL_WIDTH;
                xInfo.currentFruitLoc.x = randX;
                xInfo.currentFruitLoc.y = randY; 
            } while ((randX > INITIAL_CORNER1_X
                    && randX < INITIAL_CORNER2_X
                    && randY > INITIAL_CORNER1_Y
                    && randY < INITIAL_CORNER2_Y)
                    || (randX + PIXEL_WIDTH > INITIAL_CORNER1_X
                    && randX + PIXEL_WIDTH < INITIAL_CORNER2_X
                    && randY + PIXEL_WIDTH > INITIAL_CORNER1_Y
                    && randY + PIXEL_WIDTH < INITIAL_CORNER2_Y));
        }

    private:
        double randX;
        double randY;
        XPoint currentFruitLoc; 
};
// Declare fruit object
Fruit fruit(xInfo);

void error(string str)
{
    // Output error message.
    cerr << str << endl;

    // Exit application.
    exit(0);
}

// Get current time in microseconds.
unsigned long now()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long us = tv.tv_sec*1000000 + tv.tv_usec;

    return us;
}

/*
 * Create X window.
 */ 
void initXWindow(int argc, char * argv[], XInfo &xInfo)
{
    XSizeHints hints;
	unsigned long white, black;

    // Address of X display.
    char * display_name = getenv("Snake Game!");

    // Open connection with the X server.
    xInfo.display = XOpenDisplay(display_name);
    if (xInfo.display == NULL)
    {
        error("Cannot connect to X server and open display.");
    }

    // Number of screen to place the window on.
    xInfo.screenNumber = DefaultScreen(xInfo.display);

    white = WhitePixel(xInfo.display, xInfo.screenNumber);
    black = BlackPixel(xInfo.display, xInfo.screenNumber);

	hints.x = WINDOW_CORNER_X;
	hints.y = WINDOW_CORNER_Y;
	hints.width = SCREEN_WIDTH;
	hints.height = (SCREEN_HEIGHT + INFO_OFFSET);
	hints.flags = PPosition | PSize;

	xInfo.window = XCreateSimpleWindow(
                    xInfo.display,                          // Display where window appears.
                    DefaultRootWindow( xInfo.display ),     // Window's parent in window tree.
                    hints.x, hints.y,			            // Upper left corner location.
                    hints.width, hints.height,	            // Size of the window.
                    BORDER_SIZE,						    // Width of window's border.
                    black,						            // Window border colour.
                    white);					                // Window background colour.

	XSetStandardProperties(
		xInfo.display,		// display containing the window
		xInfo.window,		// window whose properties are set
		"Snake Game!",    	// window's title
		"Animate",			// icon's title
		None,				// pixmap for the icon
		argv, argc,			// applications command line args
		&hints );			// size hints for the window

    // Set event to monitor window.
	XSelectInput(xInfo.display, xInfo.window, 
		ButtonPressMask | KeyPressMask | KeyReleaseMask |
		PointerMotionMask | 
		EnterWindowMask | LeaveWindowMask |
		StructureNotifyMask);  // for resize events

	xInfo.gc = XCreateGC(xInfo.display, xInfo.window, 0, 0);
	XSetForeground(xInfo.display, xInfo.gc, BlackPixel(xInfo.display, xInfo.screenNumber));
	XSetBackground(xInfo.display, xInfo.gc, WhitePixel(xInfo.display, xInfo.screenNumber));
	XSetFillStyle(xInfo.display, xInfo.gc, FillSolid);
	XSetLineAttributes(xInfo.display, xInfo.gc, 2, LineSolid, CapButt, JoinRound);

    // Window name.
    XStoreName(xInfo.display, xInfo.window, "BREAKOUT!");

    // Make the window actually appear on the screen.
    XMapRaised(xInfo.display, xInfo.window);

    // Flush all pending requests to the X server. 
    XFlush(xInfo.display);

    // Allow server to setup before sending drawing commands.
    // sleep(1);
}

/*
 * Handle keyboard inputs.
 */
void keyPressInput(XInfo &xInfo, XEvent &event)
{
    KeySym key;
    char text[BUFFER_SIZE];
    /*
    * Arguments for XLookupString :
    *                 event - the keyboard event
    *                 text - buffer into which text will be written
    *                 BufferSize - size of text buffer
    *                 key - workstation independent key symbol
    *                 0 - pointer to a composeStatus structure
    */
    switch(event.type)
    {
        case KeyPress:
        {
            int i = XLookupString((XKeyEvent*)&event, text, 10, &key, 0);

            // Start game.
            if (i == 1 && text[0] == ' ' && showSplash)
            {
                dList.clear();
                dList.push_front(&snake);
                dList.push_front(&fruit);
                dList.push_front(&scoreText);
                showSplash = false;
            }

            // Restart game after losing.
            if (i == 1 && text[0] == ' ' && !alive)
            {
                // while(!dList.empty()) delete dList.front(), dList.pop_front();
                // showSplash = true;
                alive = true;
                gamePaused = false;
                dList.clear();
                // xInfo.score = 0;
                scoreText.reInitScore(xInfo);
                snake.restart();
                snake.updateFruitLoc(xInfo);
                dList.push_front(&snake);
                dList.push_front(&fruit);
                dList.push_front(&scoreText);
            }

            // Quit game.
            if (i == 1 && text[0] == 'q')
            {
                // while(!dList.empty()) delete dList.front(), dList.pop_front();
                error("Exit Snake appplication normally.");
                XCloseDisplay(xInfo.display);
            }

            // Pause game.
            if (i == 1 && text[0] == 'p' && !gamePaused)
            {
                gamePaused = true;
            }

            // Unpause game.
            if (i == 1 && text[0] == 'u' && gamePaused)
            {
                gamePaused = false;
            }
            // Snake action based on arrow key inputs.
            switch (key)
            {
                // Move North.
                case XK_Up:
                {
                    snake.pressUpKey();
                    xInfo.keyPressedBool = true;
                    // cout << "press" << endl;
                    break;
                }
                // Move South.
                case XK_Down:
                {
                    snake.pressDownKey();
                    xInfo.keyPressedBool = true;
                    // cout << "press" << endl;
                    break;
                }
                // Move East.
                case XK_Right:
                {
                    snake.pressRightKey();
                    xInfo.keyPressedBool = true;
                    // cout << "press" << endl;
                    break;
                }
                // Move West.
                case XK_Left:
                {
                    snake.pressLeftKey();
                    xInfo.keyPressedBool = true;
                    // cout << "press" << endl;
                    break;
                }
            }
            break;
        }
        case KeyRelease:
        {
            int i = XLookupString((XKeyEvent*)&event, text, 10, &key, 0);
            switch (key)
            {
                case XK_Up:
                {
                    xInfo.keyPressedBool = false;
                    // cout << "release" << endl;
                    break;
                }
                case XK_Down:
                {
                    xInfo.keyPressedBool = false;
                    // cout << "release" << endl;
                    break;
                }
                case XK_Right:
                {
                    xInfo.keyPressedBool = false;
                    // cout << "release" << endl;
                    break;
                }
                case XK_Left:
                {
                    xInfo.keyPressedBool = false;
                    // cout << "release" << endl;
                    break;
                }
            }
            break;
        }
    }
}

/*
 * Execute animation of snake.
 */
void handleAnimation(XInfo &xInfo)
{
    if (!gamePaused && alive)
    {
        snake.move(xInfo);
    }
}

/*
 * Function to draw displayables to the window.
 */
void repaint(XInfo &xInfo)
{
    // list<Displayable *> dList1;
    // list<Displayable *> dList2;

    string ss1 = "Game Paused";
    string ss2 = "Press p again to resume or q to quit.";

    string ss3 = "Game Over";
    string ss4 = "Press Spacebar to re-start or q to quit.";

    dList1.push_front(new Text(SCREEN_WIDTH/2 - 3*ss1.length(), SCREEN_HEIGHT/2 - 25, ss1));
    dList1.push_front(new Text(SCREEN_WIDTH/2 - 3*ss2.length(), SCREEN_HEIGHT/2, ss2));

    dList2.push_front(new Text(SCREEN_WIDTH/2 - 3*ss1.length(), SCREEN_HEIGHT/2 - 25, ss3));
    dList2.push_front(new Text(SCREEN_WIDTH/2 - 3*ss2.length(), SCREEN_HEIGHT/2, ss4));

    if (!gamePaused && alive)
    {
        list<Displayable *>::const_iterator begin = dList.begin();
        list<Displayable *>::const_iterator end = dList.end();

        XClearWindow(xInfo.display, xInfo.window);
        while(begin != end)
        {
            // Note: Here a pointer is set to the value pointed to by
            // begin, which is the address contained in dList.
            Displayable * d = *begin;
            d->paint(xInfo);
            begin++;
        }
        XFlush(xInfo.display);
    }
    else if (gamePaused)
    {
        list<Displayable *>::const_iterator begin1 = dList1.begin();
        list<Displayable *>::const_iterator end1 = dList1.end();
        
        XClearWindow(xInfo.display, xInfo.window);
        while(begin1 != end1)
        {
            Displayable * d1 = *begin1;
            d1->paint(xInfo);
            begin1++;
        }
        XFlush(xInfo.display);
    }
    else if (!alive)
    {
        list<Displayable *>::const_iterator begin2 = dList2.begin();
        list<Displayable *>::const_iterator end2 = dList2.end();

        XClearWindow(xInfo.display, xInfo.window);
        while(begin2 != end2)
        {
            Displayable * d2 = *begin2;
            d2->paint(xInfo);
            begin2++;
        }
        XFlush(xInfo.display);
    }
}

/*
 * Execute event loop.
 */ 
void eventLoop(XInfo &xInfo)
{
    // Show splash page initially.
    if (showSplash)
    {
        string s1 = "Snake!";
        string s2 = "Created by: Christopher Mannes";
        string s3 = "Press left, right, up, and down arrow keys to direct the snake.";
        string s4 = "Press p to pause, q to quit, and spacebar to start.";

        dList.push_front(new Text(SCREEN_WIDTH/2 - 3*s1.length(), SCREEN_HEIGHT/2 - 25, s1));
        dList.push_front(new Text(SCREEN_WIDTH/2 - 3*s2.length(), SCREEN_HEIGHT/2, s2));
        dList.push_front(new Text(SCREEN_WIDTH/2 - 3*s3.length(), SCREEN_HEIGHT/2 + 25, s3));
        dList.push_front(new Text(SCREEN_WIDTH/2 - 3*s4.length(), SCREEN_HEIGHT/2 + 50, s4));
    }

    XEvent event;
    xInfo.keyPressedBool = false;
    unsigned long lastUpdate = now();
    unsigned long lastRepaint = 0;

    while (true)
    {
        if (XPending(xInfo.display) > 0)
        {
            XNextEvent(xInfo.display, &event);

            // Decide action required based on keyPress input.
            keyPressInput(xInfo, event);
        }

        unsigned long end = now();
        xInfo.deltaTime = (end - lastUpdate);

        // Update snake state.
        if (!showSplash && !gamePaused)
        {
            handleAnimation(xInfo);
        }

        lastUpdate = now();
        if (end - lastRepaint > 1000000/FPS)
        {
            // handleAnimation(xInfo);
            repaint(xInfo);
            lastRepaint = now();
        }

        // IMPORTANT: sleep for a bit to let other processes work.
        if (XPending(xInfo.display) == 0)
        {
            usleep(1000000 / FPS - (now() - lastRepaint));
        }
    }
}

/*
 * Enter main program.
 *	 First initialize X window.
 *	 Then continuously loop responding to events.
 *	 Exit by closing window and returning int zero.
 */
int main(int argc, char * argv[]) {

    // Read command-line parameters.
    if (argc == 1)
    {
        xInfo.snakeSpeed = 50*speedArrray[2];
    }
    else if (argc == 2)
    {
        xInfo.snakeSpeed = speedArrray[stoi(argv[1])];
    }
    else if (argc > 2)
    {
        error("Invalid inputs. Only zero or one arguments allowed.");
    }

	initXWindow(argc, argv, xInfo);
	eventLoop(xInfo);
	XCloseDisplay(xInfo.display);

    return(0);
}
