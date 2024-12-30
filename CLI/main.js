const prompts = require('prompts');
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

const options = Object.keys(Commands).map(key => {
    return key
        .toLowerCase()
        .split('_')
        .map(word => word[0].charAt(0).toUpperCase() + word.slice(1)).join(' ')
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

const promptChains = {
    [Commands.KILL_PROCESS]: [
        {
            type: 'number',
            name: 'pid',
            message: 'Enter PID',
        },
    ],
    [Commands.LIST_FILES]: [
        {
            type: 'text',
            name: 'path',
            message: 'Enter remote directory path',
        },
    ],
    [Commands.READ_FILE]: [
        {
            type: 'text',
            name: 'path',
            message: 'Enter remote file path',
        },
    ],
    [Commands.UPLOAD_FILE]: [
        {
            type: 'text',
            name: 'url',
            message: 'Enter local file path or URL',
        },
        {
            type: 'text',
            name: 'path',
            message: 'Enter remote file path',
        },
    ],
    [Commands.DELETE_FILE]: [
        {
            type: 'text',
            name: 'path',
            message: 'Enter remote file path',
        },
    ],
    [Commands.SHELL]: [
        {
            type: 'text',
            name: 'ip',
            message: 'Enter IP address',
        },
        {
            type: 'number',
            name: 'port',
            message: 'Enter port',
        },
    ],
}


const main = async () => {
    const selection = await prompts({
        type: 'select',
        name: 'task',
        message: 'Select task',
        choices: options
    })

    const args = await prompts(promptChains[selection.task] || []);
    const task = Task(selection.task, args)
}

main();