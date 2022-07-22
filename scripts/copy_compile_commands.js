import fs from 'fs';
import { default as pathTools } from 'path';

let cc = JSON.parse(fs.readFileSync('build/clang_debug/compile_commands.json'));
cc = cc.map(compileLine => {
    // assume msys2
    let command = compileLine.command;
    command = command.replace(/\/([a-zA-Z])\/([^\s]*)/g, (match, c1, c2, ...rest) => {
        return `\"${c1}:/${c2}\"`;
    });
    command = command.split(/(\s+)/).filter((e) => { return e.trim().length > 0; });
    // const isMsysSubsystem = pathTools.parse(command[0]).dir.match(/^(.*)\/(?:clang64)|(?:mingw64)|(?:mingw32)|(?:ucrt64)\//);
    // if (isMsysSubsystem) {
    //     let inclPath = pathTools.join(pathTools.dirname(pathTools.dirname(command[0])), 'include');
    //     inclPath = inclPath.split(pathTools.sep).join('/');
    //     command.splice(1, 0, '-I' + inclPath + "\"");
    // }
    return {
        ...compileLine,
        command: command.join(' '),
    };
})
fs.writeFileSync('compile_commands.json', JSON.stringify(cc, null, 2), { encoding: 'utf8' });
