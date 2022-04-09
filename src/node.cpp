#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <cmath>

#include "accelerometer.h"
#include "vibrationSensor.h"
#include "segDisplay.h"
#include "network.hpp"
#include "node.h"
#include "lcdScreen.h"

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

Node::Node(const char* serverAddr, int serverPort) 
{
	nodeQuakeMagnitude = MIN_MAGNITUDE;
	nodeVibrationPulse = MIN_MAGNITUDE;
	host = serverAddr;
	port = serverPort;
	
	// init the modules ===============
	Accelerometer::GetInstance();
	VibrationSensor::GetInstance();
	SegDisplay::GetInstance();
	Network::GetInstance(host, port);
	LCDScreen::GetInstance();
	// ================================

	// launch the worker thread
	stopWorker = false;
	workerThread = std::thread(&Node::worker, this);
}
 
Node::~Node() 
{
	// wait for the thread to end
	stopWorker = true;
	workerThread.join();

	// end the modules ===============
	Accelerometer::DestroyInstance();
	VibrationSensor::DestroyInstance();
	SegDisplay::DestroyInstance();
	Network::DestroyInstance();
	LCDScreen::DestroyInstance();
	// ================================
}

void Node::worker() 
{
	Accelerometer *accInst;
	VibrationSensor *vibsInst;
	Network *netInst;
	LCDScreen *lcdInst;

	int consensusMagnitude, numNodes;
	// keep track of the previous sample to track the difference
	Vector prevSample = Accelerometer::GetInstance()->getAcceleration();
	while (!stopWorker) {
		// update reference to the instance as it could get destroyed during execution
		accInst = Accelerometer::GetInstance();
		vibsInst = VibrationSensor::GetInstance();
		netInst = Network::GetInstance(host, port);
		lcdInst = LCDScreen::GetInstance();

		// compute the change in acceleration
		Vector currSample = accInst->getAcceleration();

		computeNodeQuakeMagnitude(prevSample, currSample, vibsInst->getPulse());
		consensusMagnitude = netInst->getConsensusQuakeMagnitude();
		numNodes = netInst->getNumNodes();
		lcdInst->setStatus(numNodes, nodeQuakeMagnitude, consensusMagnitude);

		prevSample = currSample;

		std::cout << TAG
			<< "mag = " << nodeQuakeMagnitude 
			<< ", consensus = " << consensusMagnitude
			<< ", # connected nodes = " << numNodes
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

int Node::getNodeQuakeMagnitude(void) 
{
	return nodeQuakeMagnitude;
}

Node *Node::Initialize(const char* serverAddr, int serverPort) 
{
	std::lock_guard<std::mutex> lock(mtx);

	if (instance == nullptr)
		instance = new Node(serverAddr, serverPort);

	return instance;
}

Node *Node::GetInstanceIfExits() 
{
	std::lock_guard<std::mutex> lock(mtx);

	if (instance == nullptr)
		return nullptr;
	else
		return instance;
}

void Node::End(void) 
{
	std::lock_guard<std::mutex> lock(mtx);
	delete instance;
	instance = nullptr;
}


