#pragma once

#include <string>
#include <map>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Command {
	std::string taskID;
	int commandID;
	json args;
	std::function<void(json)> callback;
};

void runCommand(Command);