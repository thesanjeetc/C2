const short = require('short-uuid');

const Commands = Object.freeze({
    PING: 0,
    EXECUTE: 1,
    LIST_PROCESSES: 2,
    KILL_PROCESS: 3,
    SYSTEM_INFO: 4,
    LIST_FILES: 5,
    READ_FILE: 6,
    UPLOAD_FILE: 7,
    DELETE_FILE: 8,
    SHELL: 9,
});

const Task = (commandID, args = {}) => {
    const taskID = short.generate();
    const task = {
        taskID: taskID,
        commandID: commandID,
        args: args
    }

    console.log(task)
    return task
}

module.exports = {
    Commands,
    Task
};