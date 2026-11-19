// 一个 Client 占住端口后，第二个 Client 必须拿到带原因的 TypeError。
// 环境里连一个 Client 都建不出来时跳过。
// 注意：exit() 只记录结果、不会中断脚本，所以主体放在 run() 里，最后统一 exit(run())。

const candidates = ['38271', '38283', '38299', '38303', '38317'];

function occupyPort() {
    for (const candidate of candidates) {
        try {
            // 必须留引用，否则对象被回收后端口就释放了
            return { holder: new maa.Client(candidate), port: candidate };
        } catch (e) {
            // 端口被占或环境不支持，换下一个
        }
    }
    return null;
}

function run() {
    const occupied = occupyPort();
    if (occupied === null) {
        return 'skip: no usable TCP port in this environment';
    }

    try {
        new maa.Client(occupied.port);
    } catch (e) {
        if (e.name !== 'TypeError') {
            return 'fail: unexpected error type ' + e.name + ': ' + e.message;
        }
        if (!String(e.message).includes('Client create failed')) {
            return 'fail: error message lost its reason: ' + e.message;
        }
        return 'ok';
    }

    return 'fail: duplicate Client on port ' + occupied.port + ' was accepted';
}

exit(run());
