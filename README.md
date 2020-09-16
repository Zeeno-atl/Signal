# Signal
Header-only signal library, because every programmer should have one

# Usage

## Step 1

Declare some signals anywhere you want (class probably)

```
class Emitter {
	public:
		void doSomething();
		
		Signal<std::string, int> signalLog;
		Signal<>                 signalFinished;
};
```

## Step 2

Connect to them

```
Emitter e;

e.signalLog.connect([](std::string msg, int num){
	std::cout << "Logging: " << msg << " with number " << num << std::endl;
});
e.signalFinished.connect([]{
	std::cout << "Finished" << std::endl;
});
```

## Step 3

Emit some signals

```
void Emitter::doSomething() {
	for(int i{0}; i < 10; ++i)
		signalLog("Hello", i);
	
	signalFinished();
}
```

## Step 4

Buy me a beer. Or somebody else.

# Keywords

 - C++11
 - Header-only
 - Can connect, disconnect, block, ...
 - Simple
 - One of my first creations
  
