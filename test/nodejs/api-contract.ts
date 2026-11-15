import assert from 'node:assert/strict'
import { mkdtemp, rm, writeFile, mkdir } from 'node:fs/promises'
import { readFileSync } from 'node:fs'
import { tmpdir } from 'node:os'
import { join, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'
import { spawnSync } from 'node:child_process'
import { setImmediate as nextTurn } from 'node:timers/promises'
import './maa-client.ts'

class RecordingActor implements maa.CustomControllerActor {
    calls: unknown[][] = []
    rejectInput = false
    rejectShell = false
    image = Uint8Array.from(readFileSync(new URL('./empty.png', import.meta.url))).buffer

    connect() {
        return true
    }
    request_uuid() {
        return 'node-contract-controller'
    }
    get_features() {
        return null
    }
    get_info(): maa.MaybePromise<string | null> {
        return JSON.stringify({ fixture: '接口测试', nested: { enabled: true } })
    }
    screencap() {
        return this.image
    }
    click(x: number, y: number) {
        this.calls.push(['click', x, y])
        return true
    }
    swipe(x1: number, y1: number, x2: number, y2: number, duration: number) {
        this.calls.push(['swipe', x1, y1, x2, y2, duration])
        return true
    }
    touch_down(contact: number, x: number, y: number, pressure: number) {
        this.calls.push(['touch_down', contact, x, y, pressure])
        return true
    }
    touch_move(contact: number, x: number, y: number, pressure: number) {
        this.calls.push(['touch_move', contact, x, y, pressure])
        return true
    }
    touch_up(contact: number) {
        this.calls.push(['touch_up', contact])
        return true
    }
    click_key(key: number) {
        this.calls.push(['click_key', key])
        return true
    }
    key_down(key: number) {
        this.calls.push(['key_down', key])
        return true
    }
    key_up(key: number) {
        this.calls.push(['key_up', key])
        return true
    }
    input_text(text: string) {
        this.calls.push(['input_text', text])
        return !this.rejectInput
    }
    start_app(intent: string) {
        this.calls.push(['start_app', intent])
        return true
    }
    stop_app(intent: string) {
        this.calls.push(['stop_app', intent])
        return true
    }
    scroll(dx: number, dy: number) {
        this.calls.push(['scroll', dx, dy])
        return true
    }
    relative_move(dx: number, dy: number) {
        this.calls.push(['relative_move', dx, dy])
        return true
    }
    shell(cmd: string, timeout: number) {
        this.calls.push(['shell', cmd, timeout])
        return this.rejectShell ? null : `stdout:${cmd}\n结果🐱`
    }
    inactive() {
        return true
    }
}

async function check(name: string, callback: () => Promise<void> | void) {
    await callback()
    console.log(`PASS: Node API ${name}`)
}

export async function runApiContractTests() {
    const actor = new RecordingActor()
    const controller = new maa.CustomController(actor)
    const resource = new maa.Resource()
    const tasker = new maa.Tasker()
    const directory = await mkdtemp(join(tmpdir(), 'maafw-node-contract-'))
    controller.screenshot_use_raw_size = true

    try {
        await check('Job status transitions and 64-bit string IDs', async () => {
            for (const terminal of [maa.Status.Succeeded, maa.Status.Failed]) {
                let state = maa.Status.Pending
                const source = {
                    status(id: string) {
                        assert.equal(id, '9007199254740993')
                        return state
                    },
                    async wait() {
                        return state
                    }
                }
                const job = new maa.Job(source, '9007199254740993')
                assert.equal(job.id, '9007199254740993')
                assert.equal(job.pending, true)
                assert.equal(job.done, false)
                state = maa.Status.Running
                assert.equal(job.running, true)
                assert.equal(job.pending, false)
                state = terminal
                const pending = job.wait()
                assert.equal(await pending, job)
                assert.equal(await pending.status, terminal)
                assert.equal(await pending.done, true)
                assert.equal(await pending.succeeded, terminal === maa.Status.Succeeded)
                assert.equal(await pending.failed, terminal === maa.Status.Failed)
            }
        })

        await check('synchronous custom getter callback', () => {
            const child = spawnSync(
                process.execPath,
                ['--expose-gc', fileURLToPath(import.meta.url), '--sync-controller-info'],
                {
                    timeout: 10000,
                    encoding: 'utf8'
                }
            )
            assert.equal(child.error, undefined, String(child.error) + child.stderr + child.stdout)
            assert.equal(child.status, 0, child.stderr + child.stdout)
        })

        await check('nullable lookups and binding', () => {
            assert.equal(tasker.inited, false)
            assert.equal(tasker.resource, null)
            assert.equal(tasker.controller, null)
            assert.equal(tasker.task_detail('0' as maa.TaskId), null)
            assert.equal(tasker.node_detail('0' as maa.NodeId), null)
            assert.equal(tasker.recognition_detail('0' as maa.RecoId), null)
            assert.equal(tasker.action_detail('0' as maa.ActId), null)
            assert.equal(tasker.latest_node('Missing'), null)
            assert.equal(resource.get_node_data('Missing'), null)
            assert.equal(resource.get_node_data_parsed('Missing'), null)
            assert.equal(resource.hash, null)
            assert.deepEqual(resource.node_list, [])
            tasker.resource = resource
            tasker.controller = controller
            assert.equal(tasker.resource, resource)
            assert.equal(tasker.controller, controller)
        })

        await check('connection job and controller info', async () => {
            const job = controller.post_connection()
            const pending = job.wait()
            assert.equal(await pending, job)
            assert.equal(await pending.succeeded, true)
            assert.equal(await pending.done, true)
            assert.equal(await pending.failed, false)
            assert.equal(job.source, controller)
            assert.equal(job.succeeded, true)
            assert.equal(job.pending, false)
            assert.equal(job.running, false)
            assert.equal(controller.connected, true)
            assert.equal(controller.uuid, 'node-contract-controller')
            assert.equal(tasker.inited, true)
        })

        await check('ArrayBuffer screenshot round trip', async () => {
            const image = await controller.post_screencap().wait().get()
            assert.ok(image instanceof ArrayBuffer)
            assert.ok(image.byteLength > 0)
            assert.ok(controller.cached_image instanceof ArrayBuffer)
            const png = Buffer.from(actor.image)
            assert.deepEqual(controller.resolution, [png.readUInt32BE(16), png.readUInt32BE(20)])
        })

        await check('input arguments and default parameters', async () => {
            const inputs: [() => maa.Job<maa.CtrlId, maa.Controller>, unknown[]][] = [
                [() => controller.post_click(3, 4), ['click', 3, 4]],
                [() => controller.post_swipe(1, 2, 5, 6, 10), ['swipe', 1, 2, 5, 6, 10]],
                [() => controller.post_touch_down(2, 3, 4, 5), ['touch_down', 2, 3, 4, 5]],
                [() => controller.post_touch_move(2, 4, 5, 6), ['touch_move', 2, 4, 5, 6]],
                [() => controller.post_touch_up(2), ['touch_up', 2]],
                [() => controller.post_click_key(65), ['click_key', 65]],
                [() => controller.post_key_down(66), ['key_down', 66]],
                [() => controller.post_key_up(66), ['key_up', 66]],
                [() => controller.post_input_text('中文 🐱'), ['input_text', '中文 🐱']],
                [
                    () => controller.post_start_app('test.package/.Main'),
                    ['start_app', 'test.package/.Main']
                ],
                [() => controller.post_stop_app('test.package'), ['stop_app', 'test.package']],
                [() => controller.post_scroll(-120, 240), ['scroll', -120, 240]],
                [() => controller.post_relative_move(-3, 4), ['relative_move', -3, 4]]
            ]
            for (const [post, expected] of inputs) {
                actor.calls.length = 0
                assert.equal(await post().wait().succeeded, true, String(expected[0]))
                assert.ok(
                    actor.calls.some(call => JSON.stringify(call) === JSON.stringify(expected)),
                    String(expected[0])
                )
            }
            actor.calls.length = 0
            const shell = await tasker
                .post_action('Shell', { cmd: 'echo 中文🐱', shell_timeout: 1357 }, [0, 0, 0, 0])
                .wait()
                .get()
            assert.ok(shell)
            assert.equal(shell.nodes.length, 1)
            const action = tasker.node_detail(shell.nodes[0])!.action!
            assert.equal(action.action, 'Shell')
            assert.equal(action.success, true)
            assert.deepEqual(action.detail, {
                cmd: 'echo 中文🐱',
                shell_timeout: 1357,
                success: true,
                output: 'stdout:echo 中文🐱\n结果🐱'
            })
            assert.deepEqual(actor.calls, [['shell', 'echo 中文🐱', 1357]])
        })

        await check('failed jobs and recovery', async () => {
            actor.rejectInput = true
            const failed = await controller.post_input_text('rejected').wait()
            assert.equal(failed.failed, true)
            assert.equal(failed.succeeded, false)
            assert.equal(failed.done, true)
            actor.rejectInput = false
            assert.equal(await controller.post_input_text('accepted').wait().succeeded, true)
            actor.rejectShell = true
            try {
                const failedShell = await tasker
                    .post_task('RejectedShell', {
                        RejectedShell: {
                            action: 'Shell',
                            cmd: 'rejected',
                            shell_timeout: 2468,
                            pre_delay: 0,
                            post_delay: 0
                        }
                    })
                    .wait()
                assert.equal(failedShell.failed, true)
                const detail = failedShell.get()!
                assert.equal(tasker.node_detail(detail.nodes[0])!.action!.success, false)
                assert.ok(
                    actor.calls.some(
                        call => JSON.stringify(call) === JSON.stringify(['shell', 'rejected', 2468])
                    )
                )
            } finally {
                actor.rejectShell = false
            }
            const recovered = await tasker
                .post_action('Shell', { cmd: 'accepted', shell_timeout: 3579 }, [0, 0, 0, 0])
                .wait()
                .get()
            assert.equal(tasker.node_detail(recovered!.nodes[0])!.action!.success, true)
        })

        await check('asynchronous custom info in controller events', async () => {
            const events: maa.ControllerNotify[] = []
            actor.get_info = async () => JSON.stringify({ fixture: 'async-info' })
            const sink = controller.add_sink((_controller, event) => {
                events.push(event)
            })
            try {
                assert.equal(await controller.post_input_text('async info').wait().succeeded, true)
                assert.equal(events.length, 2)
                assert.ok(events.every(event => event.info.fixture === 'async-info'))
            } finally {
                controller.remove_sink(sink)
                actor.get_info = () => JSON.stringify({ fixture: '接口测试' })
            }
        })

        await check('Resource file loading, dump, and overlay', async () => {
            await mkdir(join(directory, 'pipeline'))
            await writeFile(
                join(directory, 'pipeline', 'nodes.json'),
                JSON.stringify({
                    Entry: { next: ['Finish'], pre_delay: 0, post_delay: 0 },
                    Finish: { pre_delay: 0, post_delay: 0 }
                })
            )
            assert.equal(await resource.post_bundle(join(directory, 'missing')).wait().failed, true)
            assert.equal(resource.loaded, false)
            assert.equal(await resource.post_bundle(directory).wait().succeeded, true)
            assert.equal(resource.loaded, true)
            assert.deepEqual([...resource.node_list!].sort(), ['Entry', 'Finish'])
            assert.ok(resource.hash)
            resource.override_pipeline({ Entry: { post_delay: 17 } })
            const raw = resource.get_node_data('Entry')
            const parsed = resource.get_node_data_parsed('Entry')
            assert.ok(raw)
            assert.ok(parsed)
            assert.deepEqual(parsed, JSON.parse(raw))
            assert.equal(parsed.post_delay, 17)
            assert.ok(resource.get_default_recognition_param('DirectHit'))
            assert.ok(resource.get_default_action_param('Click'))
            resource.override_pipeline({ Entry: { post_delay: 0 } })
        })

        await check('task details, task override, and cache', async () => {
            const job = tasker.post_task('Entry')
            const detail = await job.wait().get()
            assert.ok(detail)
            assert.equal(detail.entry, 'Entry')
            assert.equal(job.succeeded, true)
            assert.deepEqual(
                detail.nodes.map(id => tasker.node_detail(id)!.name),
                ['Entry', 'Finish']
            )
            const nodeId = detail.nodes[0]
            const node = tasker.node_detail(nodeId)!
            assert.equal(node.completed, true)
            assert.equal(node.reco!.hit, true)
            assert.equal(node.reco!.algorithm, 'DirectHit')
            assert.equal(node.action!.success, true)
            assert.equal(node.action!.action, 'DoNothing')
            assert.equal(tasker.latest_node('Entry'), nodeId)
            const single = await tasker
                .post_task('Entry', { Entry: { next: [] } })
                .wait()
                .get()
            assert.equal(single!.nodes.length, 1)
            assert.equal((await tasker.post_task('Entry').wait().get())!.nodes.length, 2)
            tasker.clear_cache()
            assert.equal(tasker.task_detail(job.id), null)
            assert.equal(tasker.node_detail(nodeId), null)
            assert.equal(tasker.latest_node('Entry'), null)
            assert.equal(await tasker.post_task('Entry').wait().succeeded, true)
        })

        await check('custom callback JSON and Context clone', async () => {
            let recognitionCalls = 0
            let actionCalls = 0
            let callbackError: unknown = null
            resource.register_custom_recognition('ContractRecognition', self => {
                recognitionCalls++
                try {
                    assert.equal(self.name, 'ContractRecognition')
                    assert.deepEqual(self.param, { language: '中文', value: 7 })
                    assert.ok(self.image instanceof ArrayBuffer)
                    return [[3, 4, 5, 6], JSON.stringify({ text: '识别结果' })]
                } catch (error) {
                    callbackError = error
                    return null
                }
            })
            resource.register_custom_action('ContractAction', async self => {
                actionCalls++
                try {
                    assert.equal(self.name, 'ContractAction')
                    assert.deepEqual(self.param, { text: '动作参数' })
                    assert.deepEqual(self.box, [3, 4, 5, 6])
                    const clone = self.context.clone()
                    clone.override_pipeline({ Entry: { post_delay: 23 }, CloneOnly: {} })
                    assert.equal(clone.get_node_data_parsed('Entry')!.post_delay, 23)
                    assert.equal(self.context.get_node_data_parsed('Entry')!.post_delay, 0)
                    assert.equal(self.context.get_node_data('CloneOnly'), null)
                    assert.equal(self.context.get_anchor('checkpoint'), null)
                    clone.set_anchor('checkpoint', 'Finish')
                    assert.equal(self.context.get_anchor('checkpoint'), 'Finish')
                    self.context.set_anchor('checkpoint', '')
                    assert.equal(clone.get_anchor('checkpoint'), null)
                    assert.equal(await clone.wait_freezes('0'), false)
                    const direct = await clone.run_recognition_direct('DirectHit', {}, actor.image)
                    assert.equal(direct!.hit, true)
                    return true
                } catch (error) {
                    callbackError = error
                    return false
                }
            })
            const job = await tasker
                .post_task('Probe', {
                    Probe: {
                        recognition: 'Custom',
                        custom_recognition: 'ContractRecognition',
                        custom_recognition_param: { language: '中文', value: 7 },
                        action: 'Custom',
                        custom_action: 'ContractAction',
                        custom_action_param: { text: '动作参数' },
                        pre_delay: 0,
                        post_delay: 0
                    }
                })
                .wait()
            assert.equal(callbackError, null)
            assert.equal(recognitionCalls, 1)
            assert.equal(actionCalls, 1)
            assert.equal(job.succeeded, true)
            assert.equal(resource.get_node_data('CloneOnly'), null)
            resource.unregister_custom_recognition('ContractRecognition')
            resource.unregister_custom_action('ContractAction')
            assert.deepEqual(resource.custom_recognition_list, [])
            assert.deepEqual(resource.custom_action_list, [])
        })

        await check('sink removal and clearing', async () => {
            const events: maa.TaskerNotify[] = []
            const sink = tasker.add_sink((_tasker, event) => {
                events.push(event)
            })
            const job = await tasker.post_task('Entry').wait()
            assert.deepEqual(
                events.map(event => event.msg),
                ['Task.Starting', 'Task.Succeeded']
            )
            assert.ok(events.every(event => String(event.task_id) === job.id))
            tasker.remove_sink(sink)
            assert.equal(await tasker.post_task('Entry').wait().succeeded, true)
            assert.equal(events.length, 2)
            tasker.add_sink((_tasker, event) => {
                events.push(event)
            })
            tasker.clear_sinks()
            assert.equal(await tasker.post_task('Entry').wait().succeeded, true)
            assert.equal(events.length, 2)
        })

        await check('global capture options apply to every Tasker', async () => {
            const second = new maa.Tasker()
            second.resource = resource
            const events: maa.TaskerContextNotify[] = []
            const sink = tasker.add_context_sink((_context, event) => { events.push(event) })
            try {
                maa.Global.capture_config = true
                maa.Global.reco_image_raw = true
                maa.Global.reco_image_draws = false
                maa.Global.reco_image_cache_limit = '4096'
                maa.Global.reco_image_cache_bytes = '1048576'
                for (const current of [tasker, second]) {
                    const detail = await current.post_recognition('ColorMatch', {
                        lower: [0, 0, 0], upper: [255, 255, 255]
                    }, actor.image).wait().get()
                    const reco = current.node_detail(detail!.nodes[0])!.reco!
                    assert.ok(reco.raw.byteLength > 0)
                    assert.equal(reco.draws.length, 0)
                }
                const recognition = events.filter(event => event.msg === 'Recognition.Starting')
                assert.equal(recognition.length, 1)
                assert.ok('pipeline' in recognition[0] && recognition[0].pipeline)
                maa.Global.reco_image_draws = true
                const drawn = await second.post_recognition('ColorMatch', {
                    lower: [0, 0, 0], upper: [255, 255, 255]
                }, actor.image).wait().get()
                assert.ok(second.node_detail(drawn!.nodes[0])!.reco!.draws.length > 0)
                maa.Global.reco_image_cache_bytes = '1'
                const limited = await second.post_recognition('DirectHit', {}, actor.image).wait().get()
                assert.equal(second.node_detail(limited!.nodes[0])!.reco!.raw.byteLength, 0)
            } finally {
                tasker.remove_context_sink(sink)
                second.destroy()
                maa.Global.capture_config = false
                maa.Global.reco_image_raw = false
                maa.Global.reco_image_draws = false
                maa.Global.reco_image_cache_bytes = '67108864'
            }
        })

        await check('Resource clear and null hash', () => {
            resource.clear()
            assert.deepEqual(resource.node_list, [])
            assert.equal(resource.hash, null)
            assert.equal(resource.get_node_data('Entry'), null)
        })
    } finally {
        await tasker.post_stop().wait()
        tasker.destroy()
        controller.destroy()
        resource.destroy()
        await rm(directory, { recursive: true, force: true })
    }
}

async function runSynchronousInfoProbe() {
    const unhandled: unknown[] = []
    const onUnhandled = (reason: unknown) => {
        unhandled.push(reason)
    }
    process.on('unhandledRejection', onUnhandled)
    try {
        const actor = new RecordingActor()
        const controller = new maa.CustomController(actor)
        try {
            assert.equal(JSON.parse(controller.info!).fixture, '接口测试')
            actor.get_info = async () => JSON.stringify({ fixture: 'async-info' })
            assert.throws(() => controller.info, /synchronous API cannot wait/)
            actor.get_info = async () => {
                throw new Error('rejected custom info')
            }
            assert.throws(() => controller.info, /synchronous API cannot wait/)
            actor.get_info = async () => {
                await nextTurn()
                throw new Error('rejected after controller destruction')
            }
            assert.throws(() => controller.info, /synchronous API cannot wait/)
        } finally {
            actor.get_info = RecordingActor.prototype.get_info
            controller.destroy()
        }
        await nextTurn()
        await nextTurn()
        assert.deepEqual(unhandled, [])
    } finally {
        process.removeListener('unhandledRejection', onUnhandled)
    }
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
    if (process.argv.includes('--sync-controller-info')) {
        await runSynchronousInfoProbe()
        global.gc?.()
        await nextTurn()
    } else {
        await runApiContractTests()
        process.exit(0)
    }
}
