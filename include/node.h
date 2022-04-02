// gets the inputs from the vibration sensor and accelerometer 
// and interprets intensities.

#ifndef _NODE_H_
#define _NODE_H_

#include <vector>
#include <mutex>
#include <thread>

struct Vector;

class Node 
{

public:
	Node(Node &other) = delete;
	void operator=(const Node &) = delete;
	
	static Node *Initialize(const char* serverAddr, int serverPort);
	static void End(void); 
	// returns an instance but does not initialize a new one if it doesnt exist.
	// this prevents the re-instantiation of the Node class if it shut down but
	// another module that hasnt shut down yet references it.
	static Node *GetInstanceIfExits(void); // return nullptr if it doesnt exist. See -> segDisplay.cpp::displayLevel() for reference

	bool isMaster(void);
	void setNodeAsMaster(bool isMaster);

	// for the LED
	int getNumberOfConnectedNodes(void);
	void setNumberOfConnectedNodes(int nNodes);

	// read values - For the 14-seg display
	// read the current nodes quake magnitude reading
	int getNodeQuakeMagnitude(void);

	// get the consensus reading - For the LED
	int getConsensusQuakeMagnitude(void);

private:
	static Node *instance;
	static std::mutex mtx;

	std::thread workerThread;

	// magnitude mutexes 
	std::mutex nodeMagMtx;
	std::mutex consensusMtx;

	int nodeQuakeMagnitude;		 // the magnitude of this node
	int nodeVibrationPulse;      // pulse from vibration sensor

	const char *host;
    int port;

	void computeNodeQuakeMagnitude(Vector prev, Vector curr, int vibrationPulse);

protected:
	Node(const char* serverAddr, int serverPort);
	~Node();

	bool stopWorker;
	void worker();
};

#endif

