#include <string>
#include <iostream>

#include "Signal.hpp"
using Zeeno::Signal;

class Emitter {
	public:
		void doSomething();
		
		Signal<std::string, int> signalLog;
		Signal<>                 signalFinished;
};

int main() {
	Emitter e;

	e.signalLog.connect(
	    [](std::string msg, int num) { std::cout << "Logging: " << msg << " with number " << num << std::endl; });
	e.signalFinished.connect([] { std::cout << "Finished" << std::endl; });

	e.doSomething();

	return 0;
}

void Emitter::doSomething() {
	for(int i{0}; i < 10; ++i)
		signalLog("Hello", i);
	
	signalFinished();
}