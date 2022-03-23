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
	
	static Node *GetInstance(void);
	static void DestroyInstance(void); 

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
	// set the consensus reading on the slave nodes (set the value from the master)
	// this method should only be called on slaves
	void setConsensusQuakeMagnitude(int magnitude);
	// this method computes a consensus magnitude from a vector of values from the slave nodes
	// this method should only be called on the master! the vector should not inlcude the masters magnitude
	void computeConsensusQuakeMagnitude(std::vector<int> magnitudes);

private:
	static Node *instance;
	static std::mutex mtx;

	std::thread workerThread;

	// magnitude mutexes 
	std::mutex nodeMagMtx;
	std::mutex consensusMtx;

	bool isNodeMaster;			 // is this node the master?
	int numberOfConnectedNodes;  // number of connected nodes in network
	int nodeQuakeMagnitude;		 // the magnitude of this node
	int consensusQuakeMagnitude; // consensus, this is the average of all the nodes in the system (only computed by master)
	int nodeVibrationPulse;      // pulse from vibration sensor

	void computeNodeQuakeMagnitude(Vector prev, Vector curr, int vibrationPulse);

protected:
	Node();
	~Node();

	bool stopWorker;
	void worker();
};

#endif

