const chalk = require("chalk");

const LOG = (s) => console.log(chalk.bold.yellow("[LOG]"), `${s}.`)
const ACTIVITY = (s) => console.log(chalk.bold.green("[ACTIVITY]"), `${s}`)
const ERROR = (s) => console.log(chalk.bold.red("[ERROR]"), `${s}.`)
const TASK = (s) => console.log(chalk.bold.blue("[TASK]"), `${s}.`)

module.exports = {
    LOG,
    ACTIVITY,
    ERROR,
    TASK
}