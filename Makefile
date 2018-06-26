all: 64bit 32bit
64bit:
	mkdir -p bin/
	g++ -Wall -Wextra -std=c++11 src/Receiver.cpp -o bin/receiver
	g++ -Wall -Wextra -std=c++11  src/Sender.cpp -o bin/sender
32bit:
	mkdir -p bin/
	g++ -Wall -Wextra -std=c++11  -m32 src/Receiver.cpp -o bin/receiver32
	g++ -Wall -Wextra -std=c++11  -m32 src/Sender.cpp -o bin/sender32

clean:
	rm -rf bin/
