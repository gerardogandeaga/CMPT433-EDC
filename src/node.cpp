#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "accelerometer.h"
#include "vibrationSensor.h"
#include "node.h"

#define TAG "NODE: "
#define SAMPLE_RATE 50000000
// accelerometer sensitivity
#define MAX_ACC_G 1
// vibration pulse sensitivity
#define VIBR_MAX_PULSE 90
#define VIBR_SMOOTHING_WEIGHT 0.8

// smoothing factor for the magnitude
#define MAG_SMOOTHING_WEIGHT 0.2

// range of quake mangitudes
#define MIN_MAGNITUDE 0.0
#define MAX_MAGNITUDE 9.0
#define NUM_SAMPLES 6

#define LERP(a, b, t) a + t * (b - a)

Node *Node::instance{nullptr};
std::mutex Node::mtx;

Node::Node() 
{
	// init the acceleromter
	Accelerometer::GetInstance();
	// init the acceleromter
	VibrationSensor::GetInstance();

	isNodeMaster = false;
	numberOfConnectedNodes = 0;
	nodeQuakeMagnitude = MIN_MAGNITUDE;
	consensusQuakeMagnitude = MIN_MAGNITUDE;
	nodeVibrationPulse = MIN_MAGNITUDE;

	// launch the worker thread
	stopWorker = false;
	workerThread = std::thread(&Node::worker, this);
}
 
Node::~Node() 
{
	// wait for the thread to end
	stopWorker = true;
	workerThread.join();
}

void Node::worker() 
{
	Accelerometer *accInst;
	VibrationSensor *vibsInst;
	// keep track of the previous sample to track the difference
	Vector prevSample = Accelerometer::GetInstance()->getAcceleration();
	while (!stopWorker) {
		// update reference to the instance as it could get destroyed during execution
		accInst = Accelerometer::GetInstance();
		vibsInst = VibrationSensor::GetInstance();

		// compute the change in acceleration
		Vector currSample = accInst->getAcceleration();

		computeNodeQuakeMagnitude(prevSample, currSample, vibsInst->getPulse());

		prevSample = currSample;

		std::cout << TAG
			<< "mag = " << nodeQuakeMagnitude 
			<< ", consensus = " << consensusQuakeMagnitude 
			<< ", # connected nodes = " << numberOfConnectedNodes
			<< std::endl;
		std::this_thread::sleep_for(std::chrono::nanoseconds(SAMPLE_RATE));
	}
}

void Node::computeNodeQuakeMagnitude(Vector prev, Vector curr, int vibrationPulse) 
{
	static int sampleIdx = 0;
	static int maxPulse = 0;
	static double maxMagnitude = 0.0;

	// collect vibration pulse data
	if (sampleIdx < NUM_SAMPLES) {
		// update the max vibration pulse
		if (vibrationPulse > maxPulse)
			maxPulse = vibrationPulse;

		if (maxPulse > VIBR_MAX_PULSE)
			maxPulse = VIBR_MAX_PULSE;

		// update the max magnitude
		// compute magnitude
		double accDiffMagnitude = curr.diff(prev).magnitude();
		double normalized = accDiffMagnitude / MAX_ACC_G;
		double magnitude = LERP(MIN_MAGNITUDE, MAX_MAGNITUDE, normalized);

		if (magnitude < MIN_MAGNITUDE) 
			magnitude = MIN_MAGNITUDE;
		if (magnitude > MAX_MAGNITUDE) 
			magnitude = MAX_MAGNITUDE;

		if (magnitude > maxMagnitude)
			maxMagnitude = magnitude;
	
		sampleIdx++;
	}
	// compute the best pulse the represents the current shake
	else {
		std::lock_guard<std::mutex> lock(nodeMagMtx);

		nodeVibrationPulse = maxPulse;
		nodeQuakeMagnitude = round(maxMagnitude + ((double)nodeVibrationPulse / VIBR_MAX_PULSE));

		maxPulse = 0;
		maxMagnitude = 0.0;
		sampleIdx = 0;
	}
}

void Node::computeConsensusQuakeMagnitude(std::vector<int> magnitudes)
{
	// only the master 
	if (isNodeMaster) {
		std::lock_guard<std::mutex> lock(consensusMtx);

		// average the magnitude
		int nNodes = magnitudes.size() + 1; // nodes + this one
		double sum = (double)nodeQuakeMagnitude;

		for (int i = 0; i < nNodes-1; i++) {
			sum += magnitudes[i];
		}

		// TODO: can we do something with standard deviation??
		consensusQuakeMagnitude = round(sum / (double)nNodes);
	}
	else {
		std::cerr << TAG << "Error! only master node can compute a consensus magnitude!" << std::endl;
		exit(EXIT_FAILURE);
	}
}

bool Node::isMaster(void)
{
	return isNodeMaster;
}

void Node::setNodeAsMaster(bool isMaster)
{
	isNodeMaster = isMaster;
}

int Node::getNumberOfConnectedNodes(void)
{
	return numberOfConnectedNodes;
}

void Node::setNumberOfConnectedNodes(int nNodes)
{
	numberOfConnectedNodes = nNodes;
}

int Node::getNodeQuakeMagnitude(void) 
{
	return nodeQuakeMagnitude;
}

int Node::getConsensusQuakeMagnitude(void)
{
	return consensusQuakeMagnitude;
}

void Node::setConsensusQuakeMagnitude(int magnitude)
{
	if (!isNodeMaster) {
		std::lock_guard<std::mutex> lock(consensusMtx);
		// set the value sent from the master node
		consensusQuakeMagnitude = magnitude;
	}
	else {
		std::cerr << "Error! this method should only be called from the slave nodes (i.e not the master) as they receive the consensus magnitude from the master!" << std::endl;
		exit(EXIT_FAILURE);
	}
}

Node *Node::GetInstance() 
{
	std::lock_guard<std::mutex> lock(mtx);

	if (instance == nullptr)
		instance = new Node();

	return instance;
}

void Node::DestroyInstance(void) 
{
	std::lock_guard<std::mutex> lock(mtx);
	delete instance;
	instance = nullptr;
}


