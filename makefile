#
# Simple makefile for compiling and running .cpp files.
# Run makefile by calling "make" in the terminal once in 
# same directory as the desired cpp file. Note: NAME does
# not require that an extension be specified.
#
NAME = "snakeGame"

MAC_OPT = -I/opt/X11/include 

all:
	@echo "Compiling..."
	g++ -o $(NAME) $(NAME).cpp -L/opt/X11/lib -lX11 -lstdc++ $(MAC_OPT)

run: all
	@echo "Running..."
	./$(NAME)

clean:
	-rm *.o $(objects) 